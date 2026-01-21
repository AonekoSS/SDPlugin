/**
 * @file StableDiffusion.cpp
 * @author 青猫 (AonekoSS)
 * @brief stable-diffusion.cppのDLLを呼ぶためのラッパー
 */
#include "pch.h"

#include "SDPlugin.h"
#include "StableDiffusion.h"

namespace StableDiffusion {
	/// @brief DLLのモジュールハンドル
	static HMODULE hModule;

	// DLL各関数のポインタ
#define DECL_FUNCTION(function) static decltype(::function)* function

	DECL_FUNCTION(sd_ctx_params_init);
	DECL_FUNCTION(sd_ctx_params_to_str);
	DECL_FUNCTION(new_sd_ctx);
	DECL_FUNCTION(free_sd_ctx);
	DECL_FUNCTION(sd_img_gen_params_init);
	DECL_FUNCTION(sd_img_gen_params_to_str);
	DECL_FUNCTION(generate_image);
	DECL_FUNCTION(sd_type_name);
	DECL_FUNCTION(str_to_rng_type);
	DECL_FUNCTION(str_to_sample_method);
	DECL_FUNCTION(str_to_scheduler);
	DECL_FUNCTION(sd_set_log_callback);
	DECL_FUNCTION(sd_set_progress_callback);

#define BIND_FUNCTION(function)  function=reinterpret_cast<decltype(function)>(GetProcAddress(hModule, #function))

	/// ライブラリ初期化
	/// @param base_path DLLを探しに行くベースパス
	void Initialize(std::string const& base_path) {
		if (hModule != NULL) return;

		// DLLのロード
		auto dll_path = base_path + "stable-diffusion.dll";
		print("LoadLibrary: %s", dll_path.c_str());
		hModule = LoadLibraryA(dll_path.c_str());
		if (hModule == NULL) {
			print("LoadLibrary: error");
			return;
		}

		// 各関数のバインディング
		BIND_FUNCTION(sd_ctx_params_init);
		BIND_FUNCTION(sd_ctx_params_to_str);
		BIND_FUNCTION(new_sd_ctx);
		BIND_FUNCTION(free_sd_ctx);
		BIND_FUNCTION(sd_img_gen_params_init);
		BIND_FUNCTION(sd_img_gen_params_to_str);
		BIND_FUNCTION(generate_image);
		BIND_FUNCTION(sd_type_name);
		BIND_FUNCTION(str_to_rng_type);
		BIND_FUNCTION(str_to_sample_method);
		BIND_FUNCTION(str_to_scheduler);
		BIND_FUNCTION(sd_set_log_callback);
		BIND_FUNCTION(sd_set_progress_callback);
	}

	/// ライブラリ解放
	void Terminate() {
		if (hModule != NULL) FreeLibrary(hModule);
		hModule = NULL;
	}

	/// ログ用コールバック
	static void log_callback(enum sd_log_level_t level, const char* log, void* data) {
		Params* params = (Params*)data;
		if (!log || !params || (!params->verbose && level <= SD_LOG_DEBUG)) return;
		const char* level_name = "????";
		switch (level) {
		case SD_LOG_DEBUG: level_name = "DEBUG"; break;
		case SD_LOG_INFO:  level_name = "INFO";  break;
		case SD_LOG_WARN:  level_name = "WARN";  break;
		case SD_LOG_ERROR: level_name = "ERROR"; break;
		}
		print("[%-5s] %s", level_name, log);
	}

	// iniファイル読み込みヘルパー（文字列用）
	static std::string iniGetString(const std::string& filePath, const std::string& section, const std::string& key){
		char buf[MAX_PATH] = {};
		GetPrivateProfileStringA(section.c_str(), key.c_str(), "", buf, MAX_PATH, filePath.c_str());
		auto text = std::string(buf);

		// コメント削除
		auto commentPos = text.find_first_of("#;");
		if (commentPos != std::string::npos) {
			text.erase(commentPos);
		}

		// アンクォート
		if (text.length() >= 2 && text.front() == '"' && text.back() == '"') {
			text = text.substr(1, text.length() - 2);
		}
		return text;
	}

	// iniファイル読み込み：文字列
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, std::string& val){
		auto s = iniGetString(filePath, section, key);
		if (!s.empty()) val = s;
	}
	// iniファイル読み込み：整数
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, int& val){
		auto s = iniGetString(filePath, section, key);
		if (!s.empty()) val = std::stoi(s);
	}
	// iniファイル読み込み：小数
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, float& val){
		auto s = iniGetString(filePath, section, key);
		if (!s.empty()) val = std::stof(s);
	}
	// iniファイル読み込み：int64_t（seed用）
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, int64_t& val){
		auto s = iniGetString(filePath, section, key);
		if (!s.empty()) val = std::stoll(s);
	}
	// iniファイル読み込み：bool
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, bool& val){
		auto s = iniGetString(filePath, section, key);
		if (s == "true") val = true; else if (s == "false") val = false;
	}
	// iniファイル読み込み：Mode
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, Mode& val){
		auto s = iniGetString(filePath, section, key);
		if (s.empty()) return;
		else if (s == "TXT2IMG") val = TXT2IMG;
		else if (s == "IMG2IMG") val = IMG2IMG;
		else if (s == "CONTROL") val = CONTROL;
	}

	/// 設定のロード
	/// @param file 設定ファイルのパス
	/// @param section セクション
	/// @return 設定データ
	Params LoadParams(const std::string& filePath, const std::string& section, const Params& defaultParams){
		Params p = defaultParams;
	    ini(filePath, section, "mode", p.mode);
	    ini(filePath, section, "model_path", p.model_path);
	    ini(filePath, section, "clip_l_path", p.clip_l_path);
	    ini(filePath, section, "clip_g_path", p.clip_g_path);
	    ini(filePath, section, "t5xxl_path", p.t5xxl_path);
	    ini(filePath, section, "diffusion_model_path", p.diffusion_model_path);
	    ini(filePath, section, "vae_path", p.vae_path);
	    ini(filePath, section, "taesd_path", p.taesd_path);
	    ini(filePath, section, "controlnet_path", p.controlnet_path);
	    ini(filePath, section, "lora_model_dir", p.lora_model_dir);

	    ini(filePath, section, "prompt", p.prompt);
	    ini(filePath, section, "negative_prompt", p.negative_prompt);
	    ini(filePath, section, "clip_skip", p.clip_skip);
	    ini(filePath, section, "cfg_scale", p.cfg_scale);
	    ini(filePath, section, "guidance", p.guidance);
		ini(filePath, section, "scheduler", p.scheduler);
	    ini(filePath, section, "sample_method", p.sample_method);
	    ini(filePath, section, "sample_steps", p.sample_steps);
	    ini(filePath, section, "strength", p.strength);
	    ini(filePath, section, "seed", p.seed);
	    ini(filePath, section, "control_strength", p.control_strength);
	    ini(filePath, section, "style_ratio", p.style_ratio);
	    ini(filePath, section, "normalize_input", p.normalize_input);
	    ini(filePath, section, "input_id_images_path", p.input_id_images_path);
		return p;
	}

	/// 画像生成
	/// @param params 生成パラメータ
	/// @param input 入力画像（t2iのコントロールかi2iのベースに使われる）
	/// @param progressCallback 進捗コールバック void(int step, int steps)
	/// @return 生成された画像データ
	Image Generate(const Params& params, const Image& input, std::function<void(int,int)> progressCallback) {
		sd_set_log_callback(log_callback, (void*)&params);
		const int batch_count = 1;

		sd_set_progress_callback([](int step, int steps, float time, void* data){
			print("progress: %d/%d, time: %f", step, steps, time);
			auto callback = reinterpret_cast<decltype(progressCallback)*>(data);
			(*callback)(step, steps);
		}, &progressCallback);

		// パラメータの調整
		auto mode = params.mode;
		auto width = params.width;
		auto height = params.height;
		auto seed = params.seed;

		// 入力無しならt2iに
		if (input.channel == 0) mode = TXT2IMG;

		// ランダムシード
		if (seed < 0) {
			srand((int)time(NULL));
			seed = rand();
		}

		// 64の倍数サイズに切り上げ
		width = (width + 0b000000111111) & 0b111111000000;
		height = (height + 0b000000111111) & 0b111111000000;

		// コンテキスト初期化
		sd_ctx_params_t ctx_params;
		sd_ctx_params_init(&ctx_params);
		ctx_params.model_path = params.model_path.c_str();
		ctx_params.clip_l_path = params.clip_l_path.c_str();
		ctx_params.clip_g_path = params.clip_g_path.c_str();
		ctx_params.t5xxl_path = params.t5xxl_path.c_str();
		ctx_params.diffusion_model_path = params.diffusion_model_path.c_str();
		ctx_params.vae_path = params.vae_path.c_str();
		ctx_params.taesd_path = params.taesd_path.c_str();
		ctx_params.control_net_path = params.controlnet_path.c_str();
		ctx_params.vae_decode_only = (params.mode == IMG2IMG);
		if (params.verbose) print("new_sd_ctx(\n%s)", sd_ctx_params_to_str(&ctx_params));

		auto sd_ctx = new_sd_ctx(&ctx_params);
		if (!sd_ctx) {
			print("sd::new_sd_ctx: initialize error!");
			return Image();
		}

		sd_img_gen_params_t gen_params;
		sd_img_gen_params_init(&gen_params);
		gen_params.prompt = params.prompt.c_str();
		gen_params.negative_prompt = params.negative_prompt.c_str();
		gen_params.clip_skip = params.clip_skip;
		gen_params.width = width;
		gen_params.height = height;
		gen_params.sample_params.guidance.txt_cfg = params.cfg_scale;
		gen_params.sample_params.scheduler = str_to_scheduler(params.scheduler.c_str());
		gen_params.sample_params.sample_method = str_to_sample_method(params.sample_method.c_str());
		gen_params.sample_params.sample_steps = params.sample_steps;
		gen_params.strength = params.strength;
		gen_params.seed = params.seed;
		gen_params.batch_count = batch_count;
		gen_params.control_strength = params.control_strength;

		// 入力画像
		auto input_image = sd_image_t{ input.width, input.height, input.channel, input.data() };
		switch (params.mode) {
		case TXT2IMG:
			break;
		case IMG2IMG:
			gen_params.init_image = input_image;
			break;
		case CONTROL:
			gen_params.control_image = input_image;
			break;
		}

		// 生成
		if (params.verbose) print("gen_params(\n%s)", sd_img_gen_params_to_str(&gen_params));
		sd_image_t* results = generate_image(sd_ctx, &gen_params);
		free_sd_ctx(sd_ctx);

		if (!results) {
			print("sd::generate error!");
			return Image();
		}

		auto result = Image(results[0]);
		free(results);
		return result;
	}
}
