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

	DECL_FUNCTION(new_sd_ctx);
	DECL_FUNCTION(free_sd_ctx);
	DECL_FUNCTION(img2img);
	DECL_FUNCTION(txt2img);
	DECL_FUNCTION(get_num_physical_cores);
	DECL_FUNCTION(sd_type_name);
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
		BIND_FUNCTION(new_sd_ctx);
		BIND_FUNCTION(free_sd_ctx);
		BIND_FUNCTION(img2img);
		BIND_FUNCTION(txt2img);
		BIND_FUNCTION(get_num_physical_cores);
		BIND_FUNCTION(sd_type_name);
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
	static std::string iniGetString(const std::string& filePath, const std::string& section, const std::string& key) {
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
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, std::string& val) {
		auto s = iniGetString(filePath, section, key);
		if (!s.empty()) val = s;
	}
	// iniファイル読み込み：整数
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, int& val) {
		auto s = iniGetString(filePath, section, key);
		if (!s.empty()) val = std::stoi(s);
	}
	// iniファイル読み込み：小数
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, float& val) {
		auto s = iniGetString(filePath, section, key);
		if (!s.empty()) val = std::stof(s);
	}
	// iniファイル読み込み：int64_t（seed用）
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, int64_t& val) {
		auto s = iniGetString(filePath, section, key);
		if (!s.empty()) val = std::stoll(s);
	}
	// iniファイル読み込み：bool
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, bool& val) {
		auto s = iniGetString(filePath, section, key);
		if (s == "true") val = true; else if (s == "false") val = false;
	}
	// iniファイル読み込み：Mode
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, Mode& val) {
		auto s = iniGetString(filePath, section, key);
		if (s.empty()) return;
		else if (s == "TXT2IMG") val = TXT2IMG;
		else if (s == "IMG2IMG") val = IMG2IMG;
		else if (s == "CONTROL") val = CONTROL;
	}
	// iniファイル読み込み：sample_method_t
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, sample_method_t& val) {
		auto s = iniGetString(filePath, section, key);
		if (s.empty()) return;
		else if (s == "euler_a") val = EULER_A;
		else if (s == "euler") val = EULER;
		else if (s == "heun") val = HEUN;
		else if (s == "dpm2") val = DPM2;
		else if (s == "dpm++2s_a") val = DPMPP2S_A;
		else if (s == "dpm++2m") val = DPMPP2M;
		else if (s == "dpm++2mv2") val = DPMPP2Mv2;
		else if (s == "ipndm") val = IPNDM;
		else if (s == "ipndm_v") val = IPNDM_V;
		else if (s == "lcm") val = LCM;
	}
	// iniファイル読み込み：schedule_t
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, schedule_t& val) {
		auto s = iniGetString(filePath, section, key);
		if (s.empty()) return;
		else if (s == "default") val = DEFAULT;
		else if (s == "discrete") val = DISCRETE;
		else if (s == "karras") val = KARRAS;
		else if (s == "exponential") val = EXPONENTIAL;
		else if (s == "ays") val = AYS;
		else if (s == "gits") val = GITS;
	}
	// iniファイル読み込み：整数配列
	static void ini(const std::string& filePath, const std::string& section, const std::string& key, std::vector<int>& val) {
		auto s = iniGetString(filePath, section, key);
		if (!s.empty()) {
			val.clear();
			std::istringstream iss(s);
			std::string token;
			while (std::getline(iss, token, ',')) {
				if (!token.empty()) val.push_back(std::stoi(token));
			}
		}
	}

	/// 設定のロード
	/// @param file 設定ファイルのパス
	/// @param section セクション
	/// @return 設定データ
	Params LoadParams(const std::string& filePath, const std::string& section, const Params& defaultParams) {
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
		ini(filePath, section, "embeddings_path", p.embeddings_path);
		ini(filePath, section, "stacked_id_embeddings_path", p.stacked_id_embeddings_path);
		ini(filePath, section, "vae_decode_only", p.vae_decode_only);
		ini(filePath, section, "vae_tiling", p.vae_tiling);
		ini(filePath, section, "free_params_immediately", p.free_params_immediately);
		ini(filePath, section, "n_threads", p.n_threads);
		// wtype
		// rng_type
		ini(filePath, section, "schedule", p.schedule);
		ini(filePath, section, "clip_on_cpu", p.clip_on_cpu);
		ini(filePath, section, "control_net_cpu", p.control_net_cpu);
		ini(filePath, section, "vae_on_cpu", p.vae_on_cpu);

		ini(filePath, section, "prompt", p.prompt);
		ini(filePath, section, "negative_prompt", p.negative_prompt);
		ini(filePath, section, "clip_skip", p.clip_skip);
		ini(filePath, section, "cfg_scale", p.cfg_scale);
		ini(filePath, section, "guidance", p.guidance);
		ini(filePath, section, "sample_method", p.sample_method);
		ini(filePath, section, "sample_steps", p.sample_steps);
		ini(filePath, section, "strength", p.strength);
		ini(filePath, section, "seed", p.seed);
		ini(filePath, section, "control_strength", p.control_strength);
		ini(filePath, section, "style_ratio", p.style_ratio);
		ini(filePath, section, "normalize_input", p.normalize_input);
		ini(filePath, section, "input_id_images_path", p.input_id_images_path);
		ini(filePath, section, "skip_layers", p.skip_layers);
		ini(filePath, section, "slg_scale", p.slg_scale);
		ini(filePath, section, "skip_layer_start", p.skip_layer_start);
		ini(filePath, section, "skip_layer_end", p.skip_layer_end);
		return p;
	}

	/// 画像生成
	/// @param params 生成パラメータ
	/// @param input 入力画像（t2iのコントロールかi2iのベースに使われる）
	/// @param progressCallback 進捗コールバック void(int step, int steps)
	/// @return 生成された画像データ
	Image Generate(const Params& params, const Image& input, std::function<void(int, int)> progressCallback) {
		sd_set_log_callback(log_callback, (void*)&params);
		const int batch_count = 1;

		sd_set_progress_callback([](int step, int steps, float time, void* data) {
			auto callback = reinterpret_cast<decltype(progressCallback)*>(data);
			(*callback)(step, steps);
		}, &progressCallback);

		// パラメータの調整
		auto mode = params.mode;
		auto width = params.width;
		auto height = params.height;
		auto n_threads = params.n_threads;
		auto seed = params.seed;

		// 入力無しならt2iに
		if (input.channel == 0) mode = TXT2IMG;

		// スレッド数
		if (n_threads <= 0) {
			n_threads = get_num_physical_cores();
		}

		// ランダムシード
		if (seed < 0) {
			srand((int)time(NULL));
			seed = rand();
		}

		// 64の倍数サイズに切り上げ
		width = (width + 0b000000111111) & 0b111111000000;
		height = (height + 0b000000111111) & 0b111111000000;

		// コントロール画像
		auto control = sd_image_t{ input.width, input.height, input.channel, input.data() };
		sd_image_t* control_image = nullptr;
		if (mode == CONTROL && input.channel && params.controlnet_path.size()) {
			control_image = &control;
		}

		// コンテキスト生成
		auto sd_ctx = new_sd_ctx(
			params.model_path.c_str(),
			params.clip_l_path.c_str(),
			params.clip_g_path.c_str(),
			params.t5xxl_path.c_str(),
			params.diffusion_model_path.c_str(),
			params.vae_path.c_str(),
			params.taesd_path.c_str(),
			params.controlnet_path.c_str(),
			params.lora_model_dir.c_str(),
			params.embeddings_path.c_str(),
			params.stacked_id_embeddings_path.c_str(),
			params.vae_decode_only,
			params.vae_tiling,
			params.free_params_immediately,
			n_threads,
			params.wtype,
			params.rng_type,
			params.schedule,
			params.clip_on_cpu,
			params.control_net_cpu,
			params.vae_on_cpu,
			params.diffusion_flash_attn);

		if (!sd_ctx) {
			print("sd::new_sd_ctx: initialize error!");
			return Image();
		}

		// 生成
		sd_image_t* results = nullptr;
		switch (params.mode) {
		case TXT2IMG:
		case CONTROL:
			results = txt2img(sd_ctx,
				params.prompt.c_str(),
				params.negative_prompt.c_str(),
				params.clip_skip,
				params.cfg_scale,
				params.guidance,
				width,
				height,
				params.sample_method,
				params.sample_steps,
				params.seed,
				batch_count,
				control_image,
				params.control_strength,
				params.style_ratio,
				params.normalize_input,
				params.input_id_images_path.c_str(),
				const_cast<int*>(params.skip_layers.data()),
				params.skip_layers.size(),
				params.slg_scale,
				params.skip_layer_start,
				params.skip_layer_end);
			break;
		case IMG2IMG:
			results = img2img(sd_ctx,
				sd_image_t{ input.width, input.height, input.channel, input.data() },
				params.prompt.c_str(),
				params.negative_prompt.c_str(),
				params.clip_skip,
				params.cfg_scale,
				params.guidance,
				width,
				height,
				params.sample_method,
				params.sample_steps,
				params.strength,
				params.seed,
				batch_count,
				control_image,
				params.control_strength,
				params.style_ratio,
				params.normalize_input,
				params.input_id_images_path.c_str(),
				const_cast<int*>(params.skip_layers.data()),
				params.skip_layers.size(),
				params.slg_scale,
				params.skip_layer_start,
				params.skip_layer_end);
			break;
		}
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
