#pragma once
#include "types.h"
#include "d3d11_helper.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

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
		~D3D11PostProcessor();

		bool Apply(const D3D11PostProcessInput &input, Viewport &outputViewport);

		void OnPSSetSamplers(ID3D11SamplerState **samplers, UINT numSamplers);

	private:
		ComPtr<ID3D11Device> device;
		std::unique_ptr<D3D11Upscaler> upscaler;
		UpscaleMethod upscaleMethod;

		void PrepareUpscaler(ID3D11Texture2D *outputTexture);

		std::unordered_set<ID3D11SamplerState*> passThroughSamplers;
		std::unordered_map<ID3D11SamplerState*, ComPtr<ID3D11SamplerState>> mappedSamplers;
		float mipLodBias = 0.0f;
	};
}
