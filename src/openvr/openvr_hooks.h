#pragma once
#include "openvr.h"

namespace vrperfkit {
	void InstallOpenVrHooks();

	void HookOpenVrInterface(const char *interfaceName, void *instance);

	vr::IVRSystem *GetOpenVrSystem();
}
