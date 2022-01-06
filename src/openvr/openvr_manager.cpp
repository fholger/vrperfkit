#include "openvr_manager.h"

#include "logging.h"
#include "openvr_hooks.h"
#include "resolution_scaling.h"

#include "d3d11/d3d11_helper.h"
#include "d3d11/d3d11_post_processor.h"

#include <unordered_map>

namespace vrperfkit {
	OpenVrManager g_openVr;

	using namespace vr;

	namespace {
		DXGI_FORMAT DetermineOutputFormat(DXGI_FORMAT inputFormat) {
			switch (inputFormat) {
			case DXGI_FORMAT_R10G10B10A2_UNORM:
			case DXGI_FORMAT_R10G10B10A2_TYPELESS:
				// SteamVR applies a different color conversion for these formats that we can't match
				// with R8G8B8 textures, so we have to use a matching texture format for our own resources.
				// Otherwise we'll get darkened pictures (applies to Revive mostly)
				return DXGI_FORMAT_R10G10B10A2_UNORM;
			default:
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			}
		}

		bool IsConsideredSrgbByOpenVR(DXGI_FORMAT format) {
			switch (format) {
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
				return true;
			case DXGI_FORMAT_B8G8R8A8_TYPELESS:
			case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT_B8G8R8X8_TYPELESS:
			case DXGI_FORMAT_R10G10B10A2_TYPELESS:
				// OpenVR appears to treat submitted typeless textures as SRGB
				return true;
			default:
				return false;
			}
		}
	}

	struct OpenVrD3D11Resources {
		std::unique_ptr<D3D11PostProcessor> postProcessor;
		ComPtr<ID3D11Device> device;
		ComPtr<ID3D11DeviceContext> context;
		ComPtr<ID3D11Texture2D> resolveTexture;
		ComPtr<ID3D11ShaderResourceView> resolveView;
		ComPtr<ID3D11Texture2D> outputTexture;
		ComPtr<ID3D11ShaderResourceView> outputView;
		ComPtr<ID3D11UnorderedAccessView> outputUav;
		bool requiresResolve;
		bool usingArrayTex;

		struct EyeViews {
			ComPtr<ID3D11ShaderResourceView> view[2];
		};
		std::unordered_map<ID3D11Texture2D*, EyeViews> inputViews;

		ID3D11ShaderResourceView *GetInputView(ID3D11Texture2D *inputTexture, int eye) {
			D3D11_TEXTURE2D_DESC td;
			inputTexture->GetDesc(&td);

			if (requiresResolve) {
				if (td.SampleDesc.Count > 1) {
					context->ResolveSubresource(resolveTexture.Get(), 0, inputTexture, 0, td.Format);
				} else {
					D3D11_BOX region;
					region.left = region.top = region.front = 0;
					region.right = td.Width;
					region.bottom = td.Height;
					region.back = 1;
					context->CopySubresourceRegion(resolveTexture.Get(), 0, 0, 0, 0, inputTexture, 0, &region);
				}
				return resolveView.Get();
			}

			if (inputViews.find(inputTexture) == inputViews.end()) {
				LOG_INFO << "Creating shader resource view for input texture " << inputTexture;
				EyeViews &views = inputViews[inputTexture];
				views.view[0] = CreateShaderResourceView(device.Get(), inputTexture);
				if (td.ArraySize > 1) {
					views.view[1] = CreateShaderResourceView(device.Get(), inputTexture, 1);
				}
				else {
					views.view[1] = views.view[0];
				}
			}

			return inputViews[inputTexture].view[eye].Get();
		}
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
			if (!initialized || failed) {
				return;
			}

			if (graphicsApi == GraphicsApi::D3D11) {
				PostProcessD3D11(info);
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
			d3d11Res->resolveTexture = CreateResolveTexture(d3d11Res->device.Get(), tex, MakeSrgbFormatsTypeless(td.Format));
			d3d11Res->resolveView = CreateShaderResourceView(d3d11Res->device.Get(), d3d11Res->resolveTexture.Get());
		}

		uint32_t outputWidth = td.Width, outputHeight = td.Height;
		AdjustOutputResolution(outputWidth, outputHeight);
		d3d11Res->outputTexture = CreatePostProcessTexture(d3d11Res->device.Get(), outputWidth, outputHeight, DetermineOutputFormat(td.Format));
		d3d11Res->outputView = CreateShaderResourceView(d3d11Res->device.Get(), d3d11Res->outputTexture.Get());
		d3d11Res->outputUav = CreateUnorderedAccessView(d3d11Res->device.Get(), d3d11Res->outputTexture.Get());

		CalculateProjectionCenters();
		CalculateEyeTextureAspectRatio();

		initialized = true;
	}

	void OpenVrManager::CalculateProjectionCenters() {
		IVRSystem *vrSystem = GetOpenVrSystem();
		auto *ctr = projCenters.eyeCenter;
		if (vrSystem == nullptr) {
			LOG_ERROR << "Failed to acquire VRSystem interface, can't calculate projection centers";
			ctr[0].x = ctr[0].y = ctr[1].x = ctr[1].y = 0.5f;
			return;
		}

		for (int eye = 0; eye < 2; ++eye) {
			float left, right, top, bottom;
			vrSystem->GetProjectionRaw((EVREye)eye, &left, &right, &top, &bottom);
			LOG_INFO << "Raw projection for eye " << eye << ": l " << left << ", r " << right << ", t " << top << ", b " << bottom;

			// calculate canted angle between the eyes
			auto ml = vrSystem->GetEyeToHeadTransform(Eye_Left);
			auto mr = vrSystem->GetEyeToHeadTransform(Eye_Right);
			float dotForward = ml.m[2][0] * mr.m[2][0] + ml.m[2][1] * mr.m[2][1] + ml.m[2][2] * mr.m[2][2];
			float cantedAngle = std::abs(std::acosf(dotForward) / 2) * (eye == Eye_Right ? -1 : 1);
			LOG_INFO << "Display is canted by " << cantedAngle << " RAD";

			float canted = std::tanf(cantedAngle);
			ctr[eye].x = 0.5f * (1.f + (right + left - 2*canted) / (left - right));
			ctr[eye].y = 0.5f * (1.f + (bottom + top) / (top - bottom));
			LOG_INFO << "Projection center for eye " << eye << ": " << ctr[eye].x << ", " << ctr[eye].y;
		}
	}

	void OpenVrManager::CalculateEyeTextureAspectRatio() {
		IVRSystem *vrSystem = GetOpenVrSystem();
		if (vrSystem == nullptr) {
			LOG_ERROR << "Failed to acquire VRSystem interface, can't calculate aspect ratio";
			aspectRatio = 0;
			return;
		}
		
		uint32_t width = 0, height = 0;
		vrSystem->GetRecommendedRenderTargetSize(&width, &height);
		aspectRatio = float(width) / height;
	}

	void OpenVrManager::PostProcessD3D11(OpenVrSubmitInfo &info) {
		ID3D11Texture2D *inputTexture = reinterpret_cast<ID3D11Texture2D *>(info.texture->handle);
		D3D11_TEXTURE2D_DESC itd, otd;
		inputTexture->GetDesc(&itd);
		d3d11Res->outputTexture->GetDesc(&otd);

		bool inputIsSrgb = info.texture->eColorSpace == ColorSpace_Gamma || (info.texture->eColorSpace == ColorSpace_Auto && IsConsideredSrgbByOpenVR(itd.Format));
		bool isCombinedTex = float(itd.Width) / itd.Height >= 1.5f * aspectRatio && std::abs(info.bounds->uMax - info.bounds->uMin) <= 0.5f;

		D3D11PostProcessInput input;
		input.eye = info.eye;
		input.inputTexture = inputTexture;
		input.inputView = d3d11Res->GetInputView(inputTexture, info.eye);
		input.inputViewport.x = std::roundf(itd.Width * min(info.bounds->uMin, info.bounds->uMax));
		input.inputViewport.y = std::roundf(itd.Height * min(info.bounds->vMin, info.bounds->vMax));
		input.inputViewport.width = std::roundf(itd.Width * std::abs(info.bounds->uMax - info.bounds->uMin));
		input.inputViewport.height = std::roundf(itd.Height * std::abs(info.bounds->vMax - info.bounds->vMin));
		input.outputTexture = d3d11Res->outputTexture.Get();
		input.outputView = d3d11Res->outputView.Get();
		input.outputUav = d3d11Res->outputUav.Get();
		input.projectionCenter = projCenters.eyeCenter[info.eye];
		input.mode = d3d11Res->usingArrayTex ? TextureMode::ARRAY : (isCombinedTex ? TextureMode::COMBINED : TextureMode::SINGLE);

		Viewport outputViewport;
		if (d3d11Res->postProcessor->Apply(input, outputViewport)) {
			outputBounds.uMin = float(outputViewport.x) / otd.Width;
			outputBounds.vMin = float(outputViewport.y) / otd.Height;
			outputBounds.uMax = float(outputViewport.width + outputViewport.x) / otd.Width;
			outputBounds.vMax = float(outputViewport.height + outputViewport.y) / otd.Height;
			info.bounds = &outputBounds;

			PrepareOutputTexInfo(info.texture, info.submitFlags);
			outputTexInfo->handle = d3d11Res->outputTexture.Get();
			outputTexInfo->eColorSpace = inputIsSrgb ? ColorSpace_Gamma : ColorSpace_Auto;
			info.texture = outputTexInfo.get();
		}
	}

	void OpenVrManager::PrepareOutputTexInfo(const Texture_t *input, EVRSubmitFlags submitFlags) {
		if ((submitFlags & Submit_TextureWithDepth) && (submitFlags & Submit_TextureWithPose)) {
			outputTexInfo.reset(new VRTextureWithPoseAndDepth_t);
			memcpy(outputTexInfo.get(), input, sizeof(VRTextureWithPoseAndDepth_t));
		}
		else if (submitFlags & Submit_TextureWithDepth) {
			outputTexInfo.reset(new VRTextureWithDepth_t);
			memcpy(outputTexInfo.get(), input, sizeof(VRTextureWithDepth_t));
		}
		else if (submitFlags & Submit_TextureWithPose) {
			outputTexInfo.reset(new VRTextureWithPose_t);
			memcpy(outputTexInfo.get(), input, sizeof(VRTextureWithPose_t));
		}
		else {
			outputTexInfo.reset(new Texture_t);
			memcpy(outputTexInfo.get(), input, sizeof(Texture_t));
		}
	}
}
