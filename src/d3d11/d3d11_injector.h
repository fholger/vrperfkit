#pragma once
#include "d3d11_helper.h"

#include <vector>

namespace vrperfkit {
	class D3D11Listener {
	public:
		virtual bool PrePSSetSamplers(UINT startSlot, UINT numSamplers, ID3D11SamplerState *const *ppSamplers) { return false; }
		virtual void PostOMSetRenderTargets(UINT numViews, ID3D11RenderTargetView *const *renderTargetViews, ID3D11DepthStencilView *depthStencilView) {}

	protected:
		~D3D11Listener() = default;
	};

	class __declspec(uuid("c0d7b492-1bfb-4099-9c67-7144e1f586ed")) D3D11Injector {
	public:
		explicit D3D11Injector(ComPtr<ID3D11Device> device);
		~D3D11Injector();

		void AddListener(D3D11Listener *listener);
		void RemoveListener(D3D11Listener *listener);

		bool PrePSSetSamplers(UINT startSlot, UINT numSamplers, ID3D11SamplerState *const *ppSamplers);
		void PostOMSetRenderTargets(UINT numViews, ID3D11RenderTargetView *const *renderTargetViews, ID3D11DepthStencilView *depthStencilView);

	private:
		ComPtr<ID3D11Device> device;
		ComPtr<ID3D11DeviceContext> context;

		std::vector<D3D11Listener*> listeners;
	};
}
