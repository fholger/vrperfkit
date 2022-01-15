#include "d3d11_injector.h"
#include "hooks.h"

namespace vrperfkit {
	namespace {
		bool alreadyInsideHook = false;

		class HookGuard {
		public:
			HookGuard() {
				state = alreadyInsideHook;
				alreadyInsideHook = true;
			}

			~HookGuard() {
				alreadyInsideHook = state;
			}

			const bool AlreadyInsideHook() const { return state; }

		private:
			bool state;
		};

		template<typename T>
		D3D11Injector *GetInjector(T *object) {
			D3D11Injector *injector = nullptr;
			UINT size = sizeof(injector);
			object->GetPrivateData(__uuidof(D3D11Injector), &size, &injector);
			return injector;
		}

		void D3D11ContextHook_PSSetSamplers(ID3D11DeviceContext *self, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState * const *ppSamplers) {
			HookGuard hookGuard;

			D3D11Injector *injector = GetInjector(self);
			if (injector != nullptr && !hookGuard.AlreadyInsideHook()) {
				if (injector->PrePSSetSamplers(StartSlot, NumSamplers, ppSamplers)) {
					return;
				}
			}

			hooks::CallOriginal(D3D11ContextHook_PSSetSamplers)(self, StartSlot, NumSamplers, ppSamplers);
		}
	}

	D3D11Injector::D3D11Injector(ComPtr<ID3D11Device> device) {
		this->device = device;
		device->GetImmediateContext(context.GetAddressOf());

		D3D11Injector *instance = this;
		UINT size = sizeof(instance);
		device->SetPrivateData(__uuidof(D3D11Injector), size, &instance);
		context->SetPrivateData(__uuidof(D3D11Injector), size, &instance);

		hooks::InstallVirtualFunctionHook("ID3D11DeviceContext::PSSetSamplers", context.Get(), 10, (void*)&D3D11ContextHook_PSSetSamplers);
	}

	D3D11Injector::~D3D11Injector() {
		hooks::RemoveHook((void*)&D3D11ContextHook_PSSetSamplers);

		device->SetPrivateData(__uuidof(D3D11Injector), 0, nullptr);
		context->SetPrivateData(__uuidof(D3D11Injector), 0, nullptr);
	}

	void D3D11Injector::AddListener(D3D11Listener *listener) {
		if (std::find(listeners.begin(), listeners.end(), listener) == listeners.end()) {
			listeners.push_back(listener);
		}
	}

	void D3D11Injector::RemoveListener(D3D11Listener *listener) {
		auto it = std::find(listeners.begin(), listeners.end(), listener);
		if (it != listeners.end()) {
			listeners.erase(it);
		}
	}

	bool D3D11Injector::PrePSSetSamplers(UINT startSlot, UINT numSamplers, ID3D11SamplerState * const *ppSamplers) {
		for (D3D11Listener *listener : listeners) {
			if (listener->PrePSSetSamplers(startSlot, numSamplers, ppSamplers)) {
				return true;
			}
		}

		return false;
	}
}
