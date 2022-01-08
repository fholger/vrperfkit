#include "proxy_helpers.h"

#include "logging.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace vrperfkit {
	fs::path GetSystemPath() {
		WCHAR buf[4096] = L"";
		GetSystemDirectoryW(buf, ARRAYSIZE(buf));
		return buf;
	}

	// RenderDoc hooks the GetProcAddress function to inject its own hooks. If we call GetProcAddress,
	// it will create an endless redirect loop between our exports and RenderDoc's hooks.
	// Therefore, we need our own GetProcAddress function which reads the address pointer directly
	// from the DLL headers.
	// This code is shamelessly adapted from Reshade
	void * GetDllFunctionPointer(HMODULE module, const std::string &name) {
		const auto image_base = reinterpret_cast<const BYTE *>(module);
		const auto image_header = reinterpret_cast<const IMAGE_NT_HEADERS *>(image_base +
			reinterpret_cast<const IMAGE_DOS_HEADER *>(image_base)->e_lfanew);

		if (image_header->Signature != IMAGE_NT_SIGNATURE ||
			image_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size == 0)
			return nullptr; // The handle does not point to a valid module

		const auto export_dir = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY *>(image_base +
			image_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		const auto export_base = static_cast<WORD>(export_dir->Base);

		for (size_t i = 0; i < export_dir->NumberOfNames; i++)
		{
			auto functionName = reinterpret_cast<const char *>(image_base +
				reinterpret_cast<const DWORD *>(image_base + export_dir->AddressOfNames)[i]);
			if (name == functionName) {
				auto ordinal = export_base +
					reinterpret_cast<const  WORD *>(image_base + export_dir->AddressOfNameOrdinals)[i];
				return const_cast<void *>(reinterpret_cast<const void *>(image_base +
					reinterpret_cast<const DWORD *>(image_base + export_dir->AddressOfFunctions)[ordinal - export_base]));
			}
		}

		return nullptr;
	}

	void EnsureLoadDll(HMODULE &pModule, const fs::path &path) {
		if (pModule != nullptr) {
			return;
		}

		LOG_INFO << "Loading DLL at " << path;
		pModule = LoadLibraryW(path.c_str());
		if (pModule == nullptr) {
			LOG_ERROR << "Failed to load DLL " << path;
		}
	}
}
