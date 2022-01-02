#pragma once
#include <cstdint>

namespace vrperfkit {
	enum class GraphicsApi {
		UNKNOWN,
		D3D11,
		D3D12,
	};

	enum Eye {
		LEFT_EYE,
		RIGHT_EYE,
	};

	enum class TextureMode {
		SINGLE,
		COMBINED,
		ARRAY,
	};

	struct Viewport {
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	};

	enum class UpscaleMethod {
		FSR,
		NIS,
	};
}
