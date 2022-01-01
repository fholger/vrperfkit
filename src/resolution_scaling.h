#pragma once
#include "config.h"

namespace vrperfkit {
	template<typename Int>
	void AdjustRenderResolution(Int &width, Int &height) {
		if (g_config.upscaling.enabled && g_config.upscaling.renderScale < 1.f) {
			width = std::roundf(width * g_config.upscaling.renderScale);
			height = std::roundf(height * g_config.upscaling.renderScale);

			// make sure returned render sizes are multiples of 2; this works better for RDM
			if (width & 1)
				++width;
			if (height & 1)
				++height;
		}
	}

	template<typename Int>
	void AdjustOutputResolution(Int &width, Int &height) {
		if (!g_config.upscaling.enabled) {
			return;
		}

		if (g_config.upscaling.renderScale < 1.f) {
			width = std::roundf(width / g_config.upscaling.renderScale);
			height = std::roundf(height / g_config.upscaling.renderScale);
		}
		else {
			width = std::roundf(width * g_config.upscaling.renderScale);
			height = std::roundf(height * g_config.upscaling.renderScale);
		}

		// make sure returned render sizes are multiples of 2; this works better for RDM
		if (width & 1)
			++width;
		if (height & 1)
			++height;
	}
}
