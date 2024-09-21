/**
 * @file SDPlugin.cpp
 * @author 青猫 (AonekoSS)
 * @brief クリスタ用の画像生成プラグイン：メインモジュール
 */
#include "pch.h"
#include <filesystem>

#include "SDPlugin.h"
#include "TriglavPlugIn.h"
#include "StableDiffusion.h"

using namespace TriglavPlugIn;
using namespace StableDiffusion;

/// このプラグインのモジュールID（GUID）
/// @note プラグイン毎に違う値にしないと駄目
constexpr auto kModuleIDString = "13E3F872-F4BD-4EFD-8F60-6B20BA7BB1B8";

/// このDLLのベースパス
std::string g_BasePath;

/// デバッグログの書き出し先
std::string g_DebugPath;

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
};

/// プロパティキー
enum PropertyKey {
	ITEM_FUNCTION = 1,
	ITEM_STEPS,
	ITEM_STRENGTH,
	ITEM_CONTROL_STRENGTH,
	ITEM_PROMPT,
	ITEM_NPROMPT,
};

/// プラグイン初期化
/// @return 正常終了ならtrue
/// @note ここでfalse返すとクリスタのバージョン上げろって言われる
static bool InitializeModule(TriglavPlugInServer* server, TriglavPlugInPtr* data) {
	// 初期化
	TriglavPlugIn::ModuleInitialize initialize(server);
	if (!initialize.Initialize(kModuleIDString)) return false;

	// 情報インスタンス
	auto info = new FilterInfo;
	info->server = server;
	*data = info;

	return true;
}

/// プラグイン終了
/// @return 正常終了ならtrue
static bool TerminateModule(TriglavPlugInServer* server, TriglavPlugInPtr* data) {
	// 情報インスタンス解放
	if (*data) {
		delete static_cast<FilterInfo*>(*data);
		*data = nullptr;
	}
	// StableDiffusionのDLL解放
	StableDiffusion::Terminate();
	return true;
}


/// プロパティの初期化
static void InitProperty(Property& p) {
	auto type = p.addEnumerationItem(ITEM_FUNCTION, "Function Type");
	type.addValue((int)Mode::TXT2IMG, "TXT2IMG");
	type.addValue((int)Mode::IMG2IMG, "IMG2IMG");
	type.addValue((int)Mode::CONTROL, "CONTROL");

	p.addIntegerItem(ITEM_STEPS, "Steps", 20, 1, 60);
	p.addDecimalItem(ITEM_STRENGTH, "Stremgth", 0.5, 0.0, 1.0);
	p.addDecimalItem(ITEM_CONTROL_STRENGTH, "Control Stremgth", 8.0, 1.0, 20.0);

	p.addStringItem(ITEM_PROMPT, "Prompt", 800);
	p.addStringItem(ITEM_NPROMPT, "Negative Prompt", 800);
}

/// @brief モードの切り替え
/// @param mode スイッチ先のモード
/// @param data フィルター情報
/// @param propertyObject 反映先プロパティ
static void SwitchToMode(StableDiffusion::Mode mode, StableDiffusion::Params& params, Property& property) {
	// コンフィグのロード
	auto iniPath = g_BasePath + "SDPlugin.ini";
	print("load: %s", iniPath.c_str());
	auto config = LoadParams(iniPath, "COMMON");
	switch (mode) {
	case Mode::TXT2IMG: config = LoadParams(iniPath, "TXT2IMG", config); break;
	case Mode::IMG2IMG: config = LoadParams(iniPath, "IMG2IMG", config); break;
	case Mode::CONTROL: config = LoadParams(iniPath, "CONTROL", config); break;
	}
	config.mode = mode;
	params = config;

	// プロパティへの反映
	property.setInteger(ITEM_STEPS, config.sample_steps);
	property.setDecimal(ITEM_STRENGTH, config.strength);
	property.setDecimal(ITEM_CONTROL_STRENGTH, config.control_strength);
	property.setString(ITEM_PROMPT, config.prompt);
	property.setString(ITEM_NPROMPT, config.negative_prompt);
}

/// プロパティ同期
static bool SyncProperty(Int itemKey, PropertyObject propertyObject, Ptr data) {
	auto& info = *static_cast<FilterInfo*>(data);
	auto& params = info.params;
	Property property(info.server, propertyObject);

	switch (itemKey) {
	case ITEM_FUNCTION:
	{
		// モード変更を検出してコンフィグを切り替える
		auto mode = Mode(property.getEnumeration(ITEM_FUNCTION));
		if (params.mode != mode) {
			SwitchToMode(mode, params, property);
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
static void TRIGLAV_PLUGIN_CALLBACK FilterPropertyCallBack(Int* result, PropertyObject propertyObject, const Int itemKey, const Int notify, Ptr data) {
	(*result) = kTriglavPlugInPropertyCallBackResultNoModify;
	if (notify != kTriglavPlugInPropertyCallBackNotifyValueChanged) return;
	if (SyncProperty(itemKey, propertyObject, data)) {
		(*result) = kTriglavPlugInPropertyCallBackResultModify;
	}
}


/// フィルタ初期化
/// @return 正常終了ならtrue
static bool InitializeFilter(TriglavPlugInServer* server, TriglavPlugInPtr* data) {
	TriglavPlugIn::Initialize initialize(server);
	auto info = static_cast<FilterInfo*>(*data);
	info->server = server;

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
	SwitchToMode(Mode::TXT2IMG, info->params, property);

	//	プロパティコールバック
	initialize.SetPropertyCallBack(FilterPropertyCallBack, *data);

	return true;
}

/// フィルタ終了
/// @return 正常終了ならtrue
static bool TerminateFilter(TriglavPlugInServer* server, TriglavPlugInPtr* data) {
	// 特に解放するリソースは無い
	return true;
}

// 画像からブロック
inline Block ImageToBlock(const Image& image) {
	return Block{
		.rect{ 0, 0, static_cast<Int>(image.width), static_cast<Int>(image.height)},
		.address{ image.data() },
		.rowBytes{ static_cast<Int>(image.width) * static_cast<Int>(image.channel)},
		.pixelBytes{ static_cast<Int>(image.channel) }, .r{0}, .g{1}, .b{2}
	};
}


/// フィルタ実行
/// @return 正常終了ならtrue
static bool RunFilter(TriglavPlugInServer* server, TriglavPlugInPtr* data) {
	Run run(server);
	auto info = static_cast<FilterInfo*>(*data);
	info->server = server;

	// 生成ライブラリの初期化
	StableDiffusion::Initialize(g_BasePath);

	// 選択範囲の取得
	auto selectAreaRect = run.GetSelectArea();
	uint32_t width = selectAreaRect.right - selectAreaRect.left;
	uint32_t height = selectAreaRect.bottom - selectAreaRect.top;

	// オフスクリーンの取得
	Offscreen offscreenSource(server), offscreenDestination(server), offscreenSelectArea(server);
	offscreenSource.GetSource();
	offscreenDestination.GetDestination();
	offscreenSelectArea.GetSelectArea();

	// メイン処理
	while (true) {
		if (run.Process(Run::States::Start) == Run::Results::Exit) break;
		run.Total(3); // 進捗コールバック取れるようになったら切り替え
		run.Progress(0);

		// パラメータの取得
		auto params = info->params;
		if (params.prompt.empty()) { print("empty prompt!"); return false; }
		if (params.model_path.empty()) { print("empty model_path!"); return false; }

		// 入力画像の取得
		Image inputImage = Image{ width, height, 3 };
		Block inputBlock = ImageToBlock(inputImage);
		auto sourceRects = offscreenSource.GetBlockRects(selectAreaRect);
		for (const auto& rect : sourceRects) {
			if (run.Process(Run::States::Continue) != Run::Results::Continue) break;
			Block srcBlock;
			offscreenSource.GetBlockImage(rect, srcBlock);
			Transfer(inputBlock, srcBlock);
		}
		if (run.Result() == Run::Results::Restart) continue;
		if (run.Result() == Run::Results::Exit) break;

		run.Progress(1);

		// 生成
		params.width = width;
		params.height = height;

		print("generate by prompt: %s", params.prompt.c_str());
		print("input image: %d * %d", width, height);
		auto result = StableDiffusion::Generate(params, inputImage);
		print("generated: %d * %d", result.width, result.height);
		Block outputBlock = ImageToBlock(result);

		run.Progress(2);

		// ブロック転送
		auto destRects = offscreenDestination.GetBlockRects(selectAreaRect);
		for (const auto& rect : destRects) {
			if (run.Process(Run::States::Continue) != Run::Results::Continue) break;

			// 転送先ブロック
			Block imageBlock, alphaBlock;
			offscreenDestination.GetBlockImage(rect, imageBlock);
			offscreenDestination.GetBlockAlpha(rect, alphaBlock);

			if (offscreenSelectArea) {
				// 選択範囲ブロック
				Block selectBlock;
				offscreenSelectArea.GetBlockSelectArea(rect, selectBlock);

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
		run.Progress(3);

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
void TRIGLAV_PLUGIN_API TriglavPluginCall(TriglavPlugInInt* result, TriglavPlugInPtr* data, TriglavPlugInInt selector, TriglavPlugInServer* server, TriglavPlugInPtr reserved) {
	*result = kTriglavPlugInCallResultFailed;

	// 生きてないと困るものをチェック
	if (!server) return;
	if (!server->serviceSuite.stringService) return;
	if (!server->serviceSuite.propertyService) return;
	if (!server->serviceSuite.propertyService2) return;
	if (!server->serviceSuite.offscreenService) return;

	// 処理の振り分け
	switch (selector) {
	case kTriglavPlugInSelectorModuleInitialize:
		if (!server->recordSuite.moduleInitializeRecord) return;
		if (!InitializeModule(server, data)) return;
		break;
	case kTriglavPlugInSelectorModuleTerminate:
		if (!TerminateModule(server, data)) return;
		break;
	case kTriglavPlugInSelectorFilterInitialize:
		print("InitializeFilter {");
		if (!server->recordSuite.filterInitializeRecord) return;
		if (!InitializeFilter(server, data)) return;
		print("InitializeFilter }");
		break;
	case kTriglavPlugInSelectorFilterTerminate:
		if (!TerminateFilter(server, data)) return;
		break;
	case kTriglavPlugInSelectorFilterRun:
		print("RunFilter {");
		if (!server->recordSuite.filterRunRecord) return;
		if (!RunFilter(server, data)) return;
		print("RunFilter }");
		break;
	}
	*result = kTriglavPlugInCallResultSuccess;
}
