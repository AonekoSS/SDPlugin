/**
 * @file StableDiffusion.h
 * @author 青猫 (AonekoSS)
 * @brief stable-diffusion.cppのDLLを呼ぶためのラッパー
 */
#pragma once

#define SD_BUILD_SHARED_LIB
#include "stable-diffusion.cpp/stable-diffusion.h"

namespace StableDiffusion {
	// 生成モード
	enum Mode {
		TXT2IMG,
		IMG2IMG,
		CONTROL,
	};

	// 生成パラメータ
	struct Params {
		// 動作オプション
		Mode mode{ TXT2IMG };
		bool verbose{ false };

		// 基本設定
		std::string model_path{};
		std::string clip_l_path{};
		std::string t5xxl_path{};
		std::string diffusion_model_path{};
		std::string vae_path{};
		std::string taesd_path{};
		std::string controlnet_path{};
		std::string lora_model_dir{};
		std::string embeddings_path{};
		std::string stacked_id_embeddings_path{};
		bool vae_decode_only{ true };
		bool vae_tiling{ false };
		bool free_params_immediately{ true };
		int n_threads{ -1 };
		sd_type_t wtype{ SD_TYPE_COUNT };
		rng_type_t rng_type{ CUDA_RNG };
		schedule_t schedule{ DEFAULT };
		bool clip_on_cpu{ false };
		bool control_net_cpu{ false };
		bool vae_on_cpu{ false };

		// 生成パラメータ
		std::string prompt{};
		std::string negative_prompt{};
		int clip_skip{ -1 };
		float cfg_scale{ 7.0f };
		float guidance{ 3.5f };
		int width{ 1024 };
		int height{ 1024 };
		sample_method_t sample_method{ EULER_A };
		int sample_steps{ 20 };
		float strength{ 0.75f };
		int64_t seed{ -1 };
		float control_strength{ 0.9f };
		float style_ratio{ 20.f };
		bool normalize_input{ false };
		std::string input_id_images_path{};
	};

	// イメージ
	class Image {
		std::shared_ptr<void> data_;
	public:
		const uint32_t width;
		const uint32_t height;
		const uint32_t channel;
		uint8_t* data() const { return static_cast<uint8_t*>(data_.get()); }
		Image() noexcept : width{ 0 }, height{ 0 }, channel{ 0 } {}
		Image(uint32_t w, uint32_t h, uint32_t c) : width{ w }, height{ h }, channel{ c }, data_{ malloc(w * h * c), free } {}
		Image(int w, int h, int c) : Image{ static_cast<uint32_t>(w), static_cast<uint32_t>(h), static_cast<uint32_t>(c) } {}
		Image(sd_image_t const& image) : width{ image.width }, height{ image.height }, channel{ image.channel }, data_{ image.data, free } {}
	};

	/// ライブラリ初期化
	extern void Initialize(const std::string& base_path);

	/// ライブラリ初期化
	extern void Terminate();

	/// 設定のロード
	/// @param filePath 設定ファイルのパス
	/// @param section セクション
	/// @return 設定データ
	extern Params LoadParams(const std::string& filePath, const std::string& section, const Params& defaultParams = Params());

	/// 画像生成
	/// @param params 生成パラメータ
	/// @param input 入力画像（t2iのコントロールかi2iのベースに使われる）
	/// @param progressCallback 進捗コールバック void(int step, int steps)
	/// @return 生成された画像データ
	extern Image Generate(const Params& params, const Image& input, std::function<void(int,int)> progressCallback);
}
