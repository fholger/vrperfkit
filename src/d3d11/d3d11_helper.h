#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <string>

using Microsoft::WRL::ComPtr;

namespace vrperfkit {
	void CheckResult(const std::string &action, HRESULT result);

	ComPtr<ID3D11ShaderResourceView> CreateShaderResourceView(ID3D11Device *device, ID3D11Texture2D *texture, int arrayIndex = 0); 

	ComPtr<ID3D11UnorderedAccessView> CreateUnorderedAccessView(ID3D11Device *device, ID3D11Texture2D *texture, int arrayIndex = 0);

	ComPtr<ID3D11Texture2D> CreateResolveTexture(ID3D11Device *device, ID3D11Texture2D *texture);

	DXGI_FORMAT TranslateTypelessFormats(DXGI_FORMAT format);
}
