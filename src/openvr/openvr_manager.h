#pragma once
#include "openvr.h"
#include "types.h"

#include <memory>

namespace vrperfkit {
	struct OpenVrSubmitInfo {
		vr::EVREye eye;
		const vr::Texture_t *texture;
		const vr::VRTextureBounds_t *bounds;
		vr::EVRSubmitFlags submitFlags;
	};

	struct OpenVrD3D11Resources;

	class OpenVrManager {
	public:
		void Shutdown();

		void OnSubmit(OpenVrSubmitInfo &info);

	private:
		bool failed = false;
		bool initialized = false;
		GraphicsApi graphicsApi = GraphicsApi::UNKNOWN;
		uint32_t textureWidth = 0;
		uint32_t textureHeight = 0;

		std::unique_ptr<OpenVrD3D11Resources> d3d11Res;

		void EnsureInit(const OpenVrSubmitInfo &info);
		void InitD3D11(const OpenVrSubmitInfo &info);
		
	};

	extern OpenVrManager g_openVr;
}
