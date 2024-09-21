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
	Int addressOffset(const Block& block, const Rect& target) {
		if (!block.needOffset) return 0;
		const auto offsetX = target.left - block.rect.left;
		const auto offsetY = target.top - block.rect.top;
		return offsetY * block.rowBytes + offsetX * block.pixelBytes;
	}

	/// @brief ブロック転送
	/// @param dst 転送先のブロック
	/// @param src 転送元のブロック
	void Transfer(const Block& dst, const Block& src) {
		const auto rect = intersectRects(dst.rect, src.rect);
		if (isRectEmpty(rect)) return;

		const auto dstRowBytes = dst.rowBytes;
		const auto dstPixelBytes = dst.pixelBytes;
		const auto dstR = dst.r, dstG = dst.g, dstB = dst.b;

		const auto srcRowBytes = src.rowBytes;
		const auto srcPixelBytes = src.pixelBytes;
		const auto srcR = src.r, srcG = src.g, srcB = src.b;

		const auto cols = rect.right - rect.left;
		const auto rows = rect.bottom - rect.top;
		pbyte_t pDstRow = static_cast<pbyte_t>(dst.address) + addressOffset(dst, rect);
		pbyte_t pSrcRow = static_cast<pbyte_t>(src.address) + addressOffset(src, rect);
		for (int y = 0; y < rows; ++y) {
			pbyte_t pSrc = pSrcRow;
			pbyte_t pDst = pDstRow;
			for (int x = 0; x < cols; ++x) {
				pDst[dstR] = pSrc[srcR];
				pDst[dstG] = pSrc[srcG];
				pDst[dstB] = pSrc[srcB];
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
	/// @param alpha 転送先のアルファチャンネル
	void Transfer(const Block& dst, const Block& src, const Block& alpha) {
		const auto rect = intersectRects(dst.rect, src.rect);
		if (isRectEmpty(rect)) return;

		const auto dstRowBytes = dst.rowBytes;
		const auto dstPixelBytes = dst.pixelBytes;
		const auto dstR = dst.r, dstG = dst.g, dstB = dst.b;

		const auto srcRowBytes = src.rowBytes;
		const auto srcPixelBytes = src.pixelBytes;
		const auto srcR = src.r, srcG = src.g, srcB = src.b;

		const auto alpRowBytes = alpha.rowBytes;
		const auto alpPixelBytes = alpha.pixelBytes;

		const auto cols = rect.right - rect.left;
		const auto rows = rect.bottom - rect.top;
		pbyte_t pDstRow = static_cast<pbyte_t>(dst.address) + addressOffset(dst, rect);
		pbyte_t pSrcRow = static_cast<pbyte_t>(src.address) + addressOffset(src, rect);
		pbyte_t pAlpRow = static_cast<pbyte_t>(alpha.address) + addressOffset(alpha, rect);
		for (int y = 0; y < rows; ++y) {
			pbyte_t pSrc = pSrcRow;
			pbyte_t pDst = pDstRow;
			pbyte_t pAlp = pAlpRow;
			for (int x = 0; x < cols; ++x) {
				if (*pAlp > 0) {
					pDst[dstR] = pSrc[srcR];
					pDst[dstG] = pSrc[srcG];
					pDst[dstB] = pSrc[srcB];
				}
				pSrc += srcPixelBytes;
				pDst += dstPixelBytes;
				pAlp += alpPixelBytes;
			}
			pSrcRow += srcRowBytes;
			pDstRow += dstRowBytes;
			pAlpRow += alpRowBytes;
		}
	}

	// アルファブレンド
	inline int BlendFunction(const int dst, const int src, const int alpha) {
		return (((src - dst) * alpha) / 255) + dst;
	}

	/// @brief ブロック転送（アルファ＆選択マスク付き）
	/// @param dst 転送先のブロック
	/// @param src 転送元のブロック
	/// @param alpha 転送先のアルファチャンネル
	/// @param select 転送元のアルファチャンネル（選択領域用）
	void Transfer(const Block& dst, const Block& src, const Block& alpha, const Block& select) {
		const auto rect = intersectRects(dst.rect, src.rect);
		if (isRectEmpty(rect)) return;

		const auto dstRowBytes = dst.rowBytes;
		const auto dstPixelBytes = dst.pixelBytes;
		const auto dstR = dst.r, dstG = dst.g, dstB = dst.b;

		const auto srcRowBytes = src.rowBytes;
		const auto srcPixelBytes = src.pixelBytes;
		const auto srcR = src.r, srcG = src.g, srcB = src.b;

		const auto alpRowBytes = alpha.rowBytes;
		const auto alpPixelBytes = alpha.pixelBytes;

		const auto selRowBytes = select.rowBytes;
		const auto selPixelBytes = select.pixelBytes;

		const auto cols = rect.right - rect.left;
		const auto rows = rect.bottom - rect.top;
		pbyte_t pDstRow = static_cast<pbyte_t>(dst.address) + addressOffset(dst, rect);
		pbyte_t pSrcRow = static_cast<pbyte_t>(src.address) + addressOffset(src, rect);
		pbyte_t pAlpRow = static_cast<pbyte_t>(alpha.address) + addressOffset(alpha, rect);
		pbyte_t pSelRow = static_cast<pbyte_t>(select.address) + addressOffset(select, rect);
		for (int y = 0; y < rows; ++y) {
			pbyte_t pSrc = pSrcRow;
			pbyte_t pDst = pDstRow;
			pbyte_t pAlp = pAlpRow;
			pbyte_t pSel = pSelRow;
			for (int x = 0; x < cols; ++x) {
				if (*pAlp > 0) {
					uint16_t alpha = *pSel;
					pDst[dstR] = BlendFunction(pDst[dstR], pSrc[srcR], alpha);
					pDst[dstG] = BlendFunction(pDst[dstG], pSrc[srcG], alpha);
					pDst[dstB] = BlendFunction(pDst[dstB], pSrc[srcB], alpha);
				}
				pSrc += srcPixelBytes;
				pDst += dstPixelBytes;
				pAlp += alpPixelBytes;
				pSel += selPixelBytes;
			}
			pSrcRow += srcRowBytes;
			pDstRow += dstRowBytes;
			pAlpRow += alpRowBytes;
			pSelRow += selRowBytes;
		}
	}
}
