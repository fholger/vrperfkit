#pragma once
#include <cstdint>

namespace vrperfkit {
	class UpscalingMod {
	public:
		void AdjustRenderResolution(uint32_t &width, uint32_t &height);
	};

	extern UpscalingMod g_upscalingMod;
}
