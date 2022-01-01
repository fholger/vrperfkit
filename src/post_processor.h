#pragma once
#include <cstdint>
#include <d3d11.h>
#include <memory>

namespace vrperfkit {
	struct D3D11PostProcessInput {
		ID3D11Texture2D *inputTexture;
		ID3D11Texture2D *outputTexture;
		ID3D11ShaderResourceView *inputView;
		ID3D11ShaderResourceView *outputView;
		ID3D11UnorderedAccessView *outputUav;
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	};

	class D3D11Upscaler {
	public:
		virtual void Upscale(const D3D11PostProcessInput &input) = 0;
	};

	class PostProcessor {
	public:
		bool ApplyD3D11(const D3D11PostProcessInput &input);

	private:
		std::unique_ptr<D3D11Upscaler> d3d11Upscaler;
	};

	extern PostProcessor g_postprocess;
}
