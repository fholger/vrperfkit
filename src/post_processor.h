#pragma once
#include <cstdint>
#include <d3d11.h>

namespace vrperfkit {
	struct D3D11PostProcessInput {
		ID3D11Texture2D *inputTexture;
		ID3D11ShaderResourceView *inputView;
		ID3D11ShaderResourceView *outputView;
		ID3D11UnorderedAccessView *outputUav;
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	};

	class PostProcessor {
	  public:
		void AdjustInputResolution(uint32_t &width, uint32_t &height);
		void AdjustOutputResolution(uint32_t &width, uint32_t &height);

		bool ApplyD3D11(const D3D11PostProcessInput &input);
	};

	extern PostProcessor g_postprocess;
}
