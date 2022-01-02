#pragma once
#include "d3d11_post_processor.h"

#include <d3d11.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace vrperfkit {
	class D3D11FsrUpscaler : public D3D11Upscaler {
	public:
		D3D11FsrUpscaler(ID3D11Device *device, uint32_t outputWidth, uint32_t outputHeight, DXGI_FORMAT format);
		void Upscale(const D3D11PostProcessInput &input, const Viewport &outputViewport) override;

	private:
		ComPtr<ID3D11DeviceContext> context;
		ComPtr<ID3D11ComputeShader> upscaleShader;
		ComPtr<ID3D11ComputeShader> sharpenShader;
		ComPtr<ID3D11Buffer> constantsBuffer;
		ComPtr<ID3D11Texture2D> upscaledTexture;
		ComPtr<ID3D11ShaderResourceView> upscaledView;
		ComPtr<ID3D11UnorderedAccessView> upscaledUav;
		ComPtr<ID3D11SamplerState> sampler;
	};
}
