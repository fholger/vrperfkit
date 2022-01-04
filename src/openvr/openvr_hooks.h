#pragma once

namespace vrperfkit {
	void InstallOpenVrHooks();

	void HookOpenVrInterface(const char *interfaceName, void *instance);
}
