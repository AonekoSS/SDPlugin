/**
 * @file TriglavPlugIn.cpp
 * @author 青猫 (AonekoSS)
 * @brief クリスタ用フィルターSDKのラッパー
 */
#include "pch.h"

#include "SDPlugin.h"
#include "TriglavPlugIn.h"

namespace TriglavPlugIn {
	// バイト定義
	typedef uint8_t byte_t;
	typedef byte_t* pbyte_t;

	/// @brief  矩形が空かどうか
	bool isRectEmpty(const Rect& rect) {
		return rect.left >= rect.right || rect.top >= rect.bottom;
	}

	/// @brief 交差する矩形
	Rect intersectRects(const Rect& a, const Rect& b) {
		Rect result;
		result.left = std::max(a.left, b.left);
		result.top = std::max(a.top, b.top);
		result.right = std::min(a.right, b.right);
		result.bottom = std::min(a.bottom, b.bottom);

		// 交差していない場合は、空の矩形を返す
		if (isRectEmpty(result)) return { 0, 0, 0, 0 };

		return result;
	}

	/// @brief 転送アドレスのオフセット計算
	/// @param block 対象のブロック
	/// @param target ターゲットの矩形
	/// @return バイト単位でのオフセット値を返す
	Int adressOffset(const Block& block, const Rect& target) {
		const auto offsetX = target.left - block.rect.left;
		const auto offsetY = target.top - block.rect.top;
		return offsetX * block.pixelBytes + offsetY * block.rowBytes;
	}

	/// @brief ブロック転送
	/// @param dst 転送先のブロック
	/// @param src 転送元のブロック
	void Transfer(const Block& dst, const Block& src) {
		const auto rect = intersectRects(dst.rect, src.rect);
		if (isRectEmpty(rect)) return;

		const auto dstRowBytes = dst.rowBytes;
		const auto dstPixelBytes = dst.pixelBytes;
		const auto dr = dst.r;
		const auto dg = dst.g;
		const auto db = dst.b;

		const auto srcRowBytes = src.rowBytes;
		const auto srcPixelBytes = src.pixelBytes;
		const auto sr = src.r;
		const auto sg = src.g;
		const auto sb = src.b;

		const auto cols = rect.right - rect.left;
		const auto rows = rect.bottom - rect.top;
		pbyte_t pDstRow = static_cast<pbyte_t>(dst.address) + adressOffset(dst, rect);
		pbyte_t pSrcRow = static_cast<pbyte_t>(src.address) + adressOffset(src, rect);
		for (int y = 0; y < rows; ++y) {
			pbyte_t pSrc = pSrcRow;
			pbyte_t pDst = pDstRow;
			for (int x = 0; x < cols; ++x) {
				pDst[dr] = pSrc[sr];
				pDst[dg] = pSrc[sg];
				pDst[db] = pSrc[sb];
				pSrc += srcPixelBytes;
				pDst += dstPixelBytes;
			}
			pSrcRow += srcRowBytes;
			pDstRow += dstRowBytes;
		}
	}

	/// @brief ブロック転送（アルファ付き）
	/// @param dst 転送先のブロック
	/// @param src 転送元のブロック
	void Transfer(const Block& dst, const Block& src, const Block& alpha) {
		/// TODO: アルファチャンネルを考慮した転送の実装
	}

	/// @brief ブロック転送（アルファ＆選択マスク付き）
	/// @param dst 転送先のブロック
	/// @param src 転送元のブロック
	void Transfer(const Block& dst, const Block& src, const Block& alpha, const Block& select) {
		/// TODO: アルファチャンネルと選択領域を考慮した転送の実装
	}
}
