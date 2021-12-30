#include "upscaling_mod.h"

#include "config.h"

namespace vrperfkit {
	UpscalingMod g_upscalingMod;

	void UpscalingMod::AdjustRenderResolution(uint32_t &width, uint32_t &height) {
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
}
