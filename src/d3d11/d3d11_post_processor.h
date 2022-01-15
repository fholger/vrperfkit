#pragma once
#include "types.h"
#include "d3d11_helper.h"
#include "d3d11_injector.h"

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

	class D3D11PostProcessor : public D3D11Listener {
	public:
		D3D11PostProcessor(ComPtr<ID3D11Device> device);

		bool Apply(const D3D11PostProcessInput &input, Viewport &outputViewport);

		bool PrePSSetSamplers(UINT startSlot, UINT numSamplers, ID3D11SamplerState * const *ppSamplers) override;

	private:
		ComPtr<ID3D11Device> device;
		ComPtr<ID3D11DeviceContext> context;
		std::unique_ptr<D3D11Upscaler> upscaler;
		UpscaleMethod upscaleMethod;

		void PrepareUpscaler(ID3D11Texture2D *outputTexture);
		void SaveTextureToFile(ID3D11Texture2D *texture);

		std::unordered_set<ID3D11SamplerState*> passThroughSamplers;
		std::unordered_map<ID3D11SamplerState*, ComPtr<ID3D11SamplerState>> mappedSamplers;
		float mipLodBias = 0.0f;

		struct ProfileQuery {
			ComPtr<ID3D11Query> queryDisjoint;
			ComPtr<ID3D11Query> queryStart;
			ComPtr<ID3D11Query> queryEnd;
		};
		static const int QUERY_COUNT = 6;
		ProfileQuery profileQueries[QUERY_COUNT];
		int currentQuery = 0;
		float summedGpuTime = 0.0f;
		int countedQueries = 0;

		void CreateProfileQueries();
		void StartProfiling();
		void EndProfiling();
	};
}
