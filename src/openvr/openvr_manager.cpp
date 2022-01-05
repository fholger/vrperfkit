#include "openvr_manager.h"

#include "logging.h"
#include "resolution_scaling.h"

#include "d3d11/d3d11_helper.h"
#include "d3d11/d3d11_post_processor.h"

#include <unordered_map>

namespace vrperfkit {
	OpenVrManager g_openVr;

	using namespace vr;

	struct OpenVrD3D11Resources {
		std::unique_ptr<D3D11PostProcessor> postProcessor;
		ComPtr<ID3D11Device> device;
		ComPtr<ID3D11DeviceContext> context;
		ComPtr<ID3D11Texture2D> resolveTexture;
		std::unordered_map<ID3D11Texture2D*, ComPtr<ID3D11ShaderResourceView>> inputViews;
		ComPtr<ID3D11Texture2D> outputTexture;
		ComPtr<ID3D11ShaderResourceView> outputView;
		ComPtr<ID3D11UnorderedAccessView> outputUav;
		bool requiresResolve;
		bool usingArrayTex;
	};

	void OpenVrManager::Shutdown() {
		d3d11Res.reset();
		initialized = false;
		failed = false;
		graphicsApi = GraphicsApi::UNKNOWN;
		textureWidth = 0;
		textureHeight = 0;
	}

	void OpenVrManager::OnSubmit(OpenVrSubmitInfo &info) {
		if (failed || info.texture == nullptr || info.texture->handle == nullptr) {
			return;
		}

		static VRTextureBounds_t defaultBounds { 0, 0, 1, 1 };
		if (info.bounds == nullptr) {
			info.bounds = &defaultBounds;
		}

		try {
			EnsureInit(info);
			if (!initialized) {
				return;
			}
		}
		catch (std::exception &e) {
			LOG_ERROR << "Error during OpenVR submit: " << e.what();
			Shutdown();
			failed = true;
		}
	}

	void OpenVrManager::EnsureInit(const OpenVrSubmitInfo &info) {
		if (info.texture->eType == TextureType_DirectX) {
			ID3D11Texture2D *d3d11Tex = (ID3D11Texture2D*)info.texture->handle;
			ComPtr<ID3D11Device> device;
			d3d11Tex->GetDevice(device.GetAddressOf());

			// check if this texture is a D3D10 texture in disguise
			ComPtr<ID3D10Device> d3d10Device;
			if (device->QueryInterface(d3d10Device.GetAddressOf()) == S_OK) {
				LOG_ERROR << "Game is submitting D3D10 textures. Not supported.";
				failed = true;
				return;
			}

			D3D11_TEXTURE2D_DESC td;
			d3d11Tex->GetDesc(&td);

			if (!initialized || graphicsApi != GraphicsApi::D3D11
					|| td.Width > textureWidth || td.Width < textureWidth - 10
					|| td.Height > textureHeight || td.Height < textureHeight - 10
					|| d3d11Res->usingArrayTex != td.ArraySize > 1) {
				Shutdown();
				InitD3D11(info);
			}
		}

		if (!initialized) {
			LOG_ERROR << "Failed to initialize OpenVR resources. Game may be using an unsupported graphics API";
			failed = true;
		}
	}

	void OpenVrManager::InitD3D11(const OpenVrSubmitInfo &info) {
		LOG_INFO << "Game is submitting D3D11 textures, creating necessary output resources...";

		d3d11Res.reset(new OpenVrD3D11Resources);
		ID3D11Texture2D *tex = (ID3D11Texture2D*)info.texture->handle;
		D3D11_TEXTURE2D_DESC td;
		tex->GetDesc(&td);
		tex->GetDevice(d3d11Res->device.GetAddressOf());
		d3d11Res->device->GetImmediateContext(d3d11Res->context.GetAddressOf());

		d3d11Res->postProcessor.reset(new D3D11PostProcessor(d3d11Res->device));

		graphicsApi = GraphicsApi::D3D11;
		textureWidth = td.Width;
		textureHeight = td.Height;
		d3d11Res->usingArrayTex = td.ArraySize > 1;
		d3d11Res->requiresResolve = td.SampleDesc.Count > 1 || !(td.BindFlags & D3D11_BIND_SHADER_RESOURCE) || IsSrgbFormat(td.Format);

		if (d3d11Res->requiresResolve) {
			LOG_INFO << "Input texture can't be bound directly, need to resolve";
			d3d11Res->resolveTexture = CreateResolveTexture(d3d11Res->device.Get(), tex);
		}

		uint32_t outputWidth = td.Width, outputHeight = td.Height;
		AdjustOutputResolution(outputWidth, outputHeight);
		d3d11Res->outputTexture = CreatePostProcessTexture(d3d11Res->device.Get(), outputWidth, outputHeight, td.Format);
		d3d11Res->outputView = CreateShaderResourceView(d3d11Res->device.Get(), d3d11Res->outputTexture.Get());
		d3d11Res->outputUav = CreateUnorderedAccessView(d3d11Res->device.Get(), d3d11Res->outputTexture.Get());

		initialized = true;
	}
}
