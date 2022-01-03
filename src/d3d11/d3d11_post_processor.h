#pragma once
#include "types.h"
#include "d3d11_helper.h"

#include <memory>

namespace vrperfkit {
	struct D3D11PostProcessInput {
		ID3D11Texture2D *inputTexture;
		ID3D11Texture2D *outputTexture;
		ID3D11ShaderResourceView *inputView;
		ID3D11ShaderResourceView *outputView;
		ID3D11UnorderedAccessView *outputUav;
		Viewport inputViewport;
		int eye;
		TextureMode mode;
		Point<float> projectionCenter;
	};

	class D3D11Upscaler {
	public:
		virtual void Upscale(const D3D11PostProcessInput &input, const Viewport &outputViewport) = 0;
	};

	class __declspec(uuid("eca6ea48-d763-48b9-8181-4e316335ad97")) D3D11PostProcessor {
	public:
		D3D11PostProcessor(ComPtr<ID3D11Device> device);
		bool Apply(const D3D11PostProcessInput &input, Viewport &outputViewport);

	private:
		ComPtr<ID3D11Device> device;
		std::unique_ptr<D3D11Upscaler> upscaler;
		UpscaleMethod upscaleMethod;

		void PrepareUpscaler(ID3D11Texture2D *outputTexture);
	};
}
