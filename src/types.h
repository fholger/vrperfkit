#pragma once
#include <cstdint>
#include <string>

namespace vrperfkit {
	enum class GraphicsApi {
		UNKNOWN,
		D3D11,
		D3D12,
		DXVK,
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

		const bool operator==(const Viewport &o) const {
			return x == o.x && y == o.y && width == o.width && height == o.height;
		}

		const bool operator !=(const Viewport &o) const {
			return !(*this == o);
		}
	};

	template<typename Num>
	struct Point {
		Num x;
		Num y;
	};

	struct ProjectionCenters {
		Point<float> eyeCenter[2];
	};

	enum class UpscaleMethod {
		FSR,
		NIS,
		CAS,
	};
	UpscaleMethod MethodFromString(std::string s);
	std::string MethodToString(UpscaleMethod method);

	enum class FixedFoveatedMethod {
		VRS,
	};
	FixedFoveatedMethod FFRMethodFromString(std::string s);
	std::string FFRMethodToString(FixedFoveatedMethod method);
}
