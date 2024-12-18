/**
 * @file SDPlugin.cpp
 * @author 青猫 (AonekoSS)
 * @brief クリスタ用の画像生成プラグイン：メインモジュール
 */
#include "pch.h"
#include <filesystem>

#include "SDPlugin.h"
#include "FilterPlugIn.h"
#include "StableDiffusion.h"

using namespace FilterPlugIn;
using namespace StableDiffusion;

/// このプラグインのモジュールID（GUID）
/// @note プラグイン毎に違う値にしないと駄目
constexpr auto kModuleIDString = "13E3F872-F4BD-4EFD-8F60-6B20BA7BB1B8";

/// このDLLのベースパス
std::string g_BasePath;

/// デバッグログの書き出し先
std::string g_DebugPath;

/// 設定リスト
std::vector<std::string> g_Settings;


/// デバッグ出力の開始
void InitDebugOutput(const std::string& basePath) {
	g_DebugPath = basePath + "debuglog.txt";
	FILE* fp = nullptr;
	fopen_s(&fp, g_DebugPath.c_str(), "w");
	if (fp) fclose(fp);
}

/// デバッグ出力
/// @note ホストアプリがデバッガを嫌うから原始的なファイル出力で
void print(const char* format, ...) {
	if (g_DebugPath.empty()) return;
	FILE* fp = nullptr;
	fopen_s(&fp, g_DebugPath.c_str(), "a");
	if (fp) {
		va_list arg;
		va_start(arg, format);
		vfprintf(fp, format, arg);
		va_end(arg);
		fputs("\n", fp);
		fclose(fp);
	}
}

/// @brief ベースパス取得
/// @param hModule モジュールハンドル
/// @return このモジュールのベースパス
/// @note ロングパス対策なしの簡易版だけど用途的には大丈夫な筈
std::string GetBasePath(HMODULE hModule) {
	std::vector<char> buf(MAX_PATH);
	GetModuleFileNameA(hModule, &buf[0], MAX_PATH);
	std::filesystem::path path(&buf[0]);
	return path.parent_path().string() + "\\";
}

/// @brief DLLのエントリーポイント
/// @param hModule DLL（自分）のモジュールハンドル
/// @param fdwReason 呼ばれた理由（プロセスorスレッドのアタッチorデタッチ）
/// @param lpReserved 予約済み
/// @return 基本的にTRUE（ロード相手にNULL返すならFALSE）
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  fdwReason, LPVOID lpReserved) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		// ベースパスの取得＆デバッグ出力の開始
		g_BasePath = GetBasePath(hModule);
		InitDebugOutput(g_BasePath);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

/// フィルター情報
struct FilterInfo {
	Server const* server;
	StableDiffusion::Params params;
	int setting;
};

/// プロパティキー
enum PropertyKey {
	ITEM_SETTING = 1,
	ITEM_STEPS,
	ITEM_STRENGTH,
	ITEM_CONTROL_STRENGTH,
	ITEM_PROMPT,
	ITEM_NPROMPT,
};

/// プラグイン初期化
/// @return 正常終了ならtrue
/// @note ここでfalse返すとクリスタのバージョン上げろって言われる
static bool InitializeModule(FilterPlugIn::Server* server, FilterPlugIn::Ptr* data) {
	// 初期化
	FilterPlugIn::ModuleInitialize initialize(server);
	if (!initialize.Initialize(kModuleIDString)) return false;

	// 情報インスタンス
	auto info = new FilterInfo;
	info->server = server;
	*data = info;

	return true;
}

/// プラグイン終了
/// @return 正常終了ならtrue
static bool TerminateModule(FilterPlugIn::Server* server, FilterPlugIn::Ptr* data) {
	// 情報インスタンス解放
	if (*data) {
		delete static_cast<FilterInfo*>(*data);
		*data = nullptr;
	}
	// StableDiffusionのDLL解放
	StableDiffusion::Terminate();
	return true;
}


/// @brief 設定ファイルのパス
/// @return iniファイルのフルパスを返す
static std::string GetIniPath()
{
	return g_BasePath + "SDPlugin.ini";
}

/// @brief 設定ファイルのセクション
/// @return COMMON以外のセクションを返す
static std::vector<std::string> GetIniSections(const std::string& iniPath)
{
		char sections[4096];
		GetPrivateProfileSectionNamesA(sections, sizeof(sections), iniPath.c_str());
		char *section = sections;
		std::vector<std::string> result;
		while (*section != NULL)
		{
			std::string name = section;
			if (name != "COMMON") result.push_back(name);
			section += strlen(section) + 1;
		}
		return result;
}

/// @brief 文字列のUNICODE化
/// @param str ShiftJIS文字列
/// @return UNICODE文字列
std::wstring ShiftJIS_to_UTF16(const std::string& str)
{
    static_assert(sizeof(wchar_t) == 2, "for windows only");
    const int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
    std::vector<wchar_t> result(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &result[0], len);
    return &result[0];
}

/// プロパティの初期化
static void InitProperty(Property& p) {
	auto setting = p.addEnumerationItem(ITEM_SETTING, "Setting");
	for(int i = 0; i < g_Settings.size(); ++i) {
		setting.addValue(i, ShiftJIS_to_UTF16(g_Settings[i])); // UI側はUNICODEが良い
	}

	p.addIntegerItem(ITEM_STEPS, "Steps", 20, 1, 60);
	p.addDecimalItem(ITEM_STRENGTH, "Strength", 0.5, 0.0, 1.0);
	p.addDecimalItem(ITEM_CONTROL_STRENGTH, "Control Strength", 8.0, 1.0, 20.0);

	p.addStringItem(ITEM_PROMPT, "Prompt", 800);
	p.addStringItem(ITEM_NPROMPT, "Negative Prompt", 800);
}

/// @brief 設定の切り替え
/// @param index スイッチ先の設定インデックス
/// @param data フィルター情報
/// @param propertyObject 反映先プロパティ
static void SwitchToSetting(int index, StableDiffusion::Params& params, Property& property) {
	if (index < 0 || g_Settings.size() <= index) return;
	const auto setting = g_Settings[index];

	// コンフィグのロード
	auto iniPath = GetIniPath();
	auto common = LoadParams(iniPath, "COMMON");
	params = LoadParams(iniPath, setting, common);

	// プロパティへの反映
	property.setEnumeration(ITEM_SETTING, index);
	property.setInteger(ITEM_STEPS, params.sample_steps);
	property.setDecimal(ITEM_STRENGTH, params.strength);
	property.setDecimal(ITEM_CONTROL_STRENGTH, params.control_strength);
	property.setStringDefault(ITEM_PROMPT, params.prompt);
	property.setStringDefault(ITEM_NPROMPT, params.negative_prompt);
}

/// プロパティ同期
static bool SyncProperty(Int itemKey, PropertyObject propertyObject, Ptr data) {
	auto& info = *static_cast<FilterInfo*>(data);
	auto& params = info.params;
	Property property(info.server, propertyObject);

	switch (itemKey) {
	case ITEM_SETTING:
	{
		// 設定変更を検出してコンフィグを切り替える
		auto setting = Mode(property.getEnumeration(ITEM_SETTING));
		if (info.setting != setting) {
			SwitchToSetting(setting, params, property);
			info.setting = setting;
			return true;
		}
	}
	break;
	case ITEM_STEPS:
		return property.sync(ITEM_STEPS, params.sample_steps);
	case ITEM_STRENGTH:
		return property.sync(ITEM_STRENGTH, params.strength);
	case ITEM_CONTROL_STRENGTH:
		return property.sync(ITEM_CONTROL_STRENGTH, params.control_strength);
	case ITEM_PROMPT:
		return property.sync(ITEM_PROMPT, params.prompt);
	case ITEM_NPROMPT:
		return property.sync(ITEM_NPROMPT, params.negative_prompt);
	}
	return false;
}

/// プロパティコールバック
static void FilterPropertyCallBack(PropertyCallBackResult* result, PropertyObject propertyObject, const Int itemKey, const PropertyCallBackNotify notify, Ptr data) {
	(*result) = PropertyCallBackResult::NoModify;
	if (notify != PropertyCallBackNotify::ValueChanged) return;
	if (SyncProperty(itemKey, propertyObject, data)) {
		(*result) = PropertyCallBackResult::Modify;
	}
}


/// フィルタ初期化
/// @return 正常終了ならtrue
static bool InitializeFilter(FilterPlugIn::Server* server, FilterPlugIn::Ptr* data) {
	FilterPlugIn::Initialize initialize(server);
	auto info = static_cast<FilterInfo*>(*data);
	info->server = server;

	// 設定リストの初期化
	g_Settings = GetIniSections(GetIniPath());

	//	フィルタカテゴリ名とフィルタ名の設定
	initialize.SetCategoryName("Stable Diffusion", 'x');
	initialize.SetFilterName("Generate", 'x');

	//	プレビュー無し（LCMとかで高速化出来たらONにしてもいいか？）
	initialize.SetCanPreview(false);

	// ブランク画像でもOK
	initialize.SetUseBlankImage(true);

	//	ターゲット
	initialize.SetTargetKinds({ Initialize::Target::RGBAlpha });

	//	プロパティの作成
	auto property = Property(server);
	InitProperty(property);
	initialize.SetProperty(property);

	// 初回は0番設定に
	SwitchToSetting(0, info->params, property);
	info->setting = 0;

	//	プロパティコールバック
	initialize.SetPropertyCallBack(FilterPropertyCallBack, *data);

	return true;
}

/// フィルタ終了
/// @return 正常終了ならtrue
static bool TerminateFilter(Server* server, Ptr* data) {
	// 特に解放するリソースは無い
	return true;
}

// 画像からブロック
inline Block ImageToBlock(const Image& image, int x, int y) {
	return Block{
		.rect{ x, y, static_cast<Int>(image.width) + x, static_cast<Int>(image.height) + y},
		.address{ image.data() },
		.rowBytes{ static_cast<Int>(image.width) * static_cast<Int>(image.channel)},
		.pixelBytes{ static_cast<Int>(image.channel) }, .r{0}, .g{1}, .b{2},
		.needOffset{true}
	};
}


/// フィルタ実行
/// @return 正常終了ならtrue
static bool RunFilter(Server* server, Ptr* data) {
	Run run(server);
	auto info = static_cast<FilterInfo*>(*data);
	info->server = server;

	// 前回の設定で開く
	Property property(server, run.GetProperty());
	SwitchToSetting(info->setting, info->params, property);

	// 生成ライブラリの初期化
	StableDiffusion::Initialize(g_BasePath);

	// 選択範囲の取得
	const auto selectAreaRect = run.GetSelectArea();
	const auto width = selectAreaRect.right - selectAreaRect.left;
	const auto height = selectAreaRect.bottom - selectAreaRect.top;
	const auto offsetX = selectAreaRect.left;
	const auto offsetY = selectAreaRect.top;

	// オフスクリーンの取得
	Offscreen offscreenSource(server), offscreenDestination(server), offscreenSelectArea(server);
	offscreenSource.GetSource();
	offscreenDestination.GetDestination();
	offscreenSelectArea.GetSelectArea();

	// メイン処理
	while (true) {
		if (run.Process(Run::States::Start) == Run::Results::Exit) break;

		// パラメータの取得
		auto params = info->params;
		if (params.prompt.empty()) { print("empty prompt!"); return false; }
		if (params.model_path.empty()) { print("empty model_path!"); return false; }
		run.Total(params.sample_steps);

		// 入力画像の取得
		Image inputImage = Image{ width, height, 3 };
		Block inputBlock = ImageToBlock(inputImage, offsetX, offsetY);
		auto sourceRects = offscreenSource.GetBlockRects(selectAreaRect);
		for (const auto& rect : sourceRects) {
			if (run.Process(Run::States::Continue) != Run::Results::Continue) break;
			Block srcBlock = offscreenSource.GetBlockImage(rect);
			Transfer(inputBlock, srcBlock);
		}
		if (run.Result() == Run::Results::Restart) continue;
		if (run.Result() == Run::Results::Exit) break;

		// 生成
		params.width = width;
		params.height = height;

		print("generate by prompt: %s", params.prompt.c_str());
		print("input image: %d * %d", width, height);
		auto result = StableDiffusion::Generate(params, inputImage,
			[&run](int step, int steps) { // 進捗コールバック
				run.Progress(step);
				print("Progress %d / %d", step, steps);
			});

		print("generated: %d * %d", result.width, result.height);
		Block outputBlock = ImageToBlock(result, offsetX, offsetY);

		// ブロック転送
		auto destRects = offscreenDestination.GetBlockRects(selectAreaRect);
		for (const auto& rect : destRects) {
			if (run.Process(Run::States::Continue) != Run::Results::Continue) break;

			// 転送先ブロック
			Block imageBlock = offscreenDestination.GetBlockImage(rect);
			Block alphaBlock = offscreenDestination.GetBlockAlpha(rect);

			if (offscreenSelectArea) {
				// 選択範囲ブロック
				Block selectBlock = offscreenSelectArea.GetBlockSelectArea(rect);

				// 選択範囲（マスク）付きで描画
				Transfer(imageBlock, outputBlock, alphaBlock, selectBlock);
			} else {
				// 選択範囲なしで描画（透明ピクセルは埋めない）
				Transfer(imageBlock, outputBlock, alphaBlock);
			}

			run.UpdateRect(rect);
		}
		if (run.Result() == Run::Results::Restart) continue;
		if (run.Result() == Run::Results::Exit) break;

		// 継続確認
		if (run.Process(Run::States::End) != Run::Results::Restart) break;
	}

	return true;
}



/// @brief プラグインのエントリーポイント
/// @param result ここに成否の結果をつっこむ
/// @param data 共有データ
/// @param selector 処理のセレクタ（Run以外は初期化/解放１回ずつ来る）
/// @param server 処理サーバー（ホスト側アクセスが全部入ってる）
/// @param reserved 予約済みかな？
/// @return 無し
extern "C" __declspec(dllexport) void TriglavPluginCall(CallResult* result, Ptr* data, Selector selector, Server* server, void* reserved) {
	*result = CallResult::Failed;

	// 生きてないと困るものをチェック
	if (!server) return;
	if (!server->serviceSuite.stringService) return;
	if (!server->serviceSuite.propertyService) return;
	if (!server->serviceSuite.propertyService2) return;
	if (!server->serviceSuite.offscreenService) return;

	// 処理の振り分け
	switch (selector) {
	case Selector::ModuleInitialize:
		if (!server->recordSuite.moduleInitializeRecord) return;
		if (!InitializeModule(server, data)) return;
		break;
	case Selector::ModuleTerminate:
		if (!TerminateModule(server, data)) return;
		break;
	case Selector::FilterInitialize:
		print("InitializeFilter {");
		if (!server->recordSuite.filterInitializeRecord) return;
		if (!InitializeFilter(server, data)) return;
		print("InitializeFilter }");
		break;
	case Selector::FilterTerminate:
		if (!TerminateFilter(server, data)) return;
		break;
	case Selector::FilterRun:
		print("RunFilter {");
		if (!server->recordSuite.filterRunRecord) return;
		if (!RunFilter(server, data)) return;
		print("RunFilter }");
		break;
	}
	*result = CallResult::Success;
}

