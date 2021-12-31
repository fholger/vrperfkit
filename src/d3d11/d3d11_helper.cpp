#include "d3d11_helper.h"

#include <sstream>

namespace {
	std::string MapResult(HRESULT result) {
		switch (result) {
		case E_INVALIDARG: return "E_INVALIDARG";
		case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
		case E_FAIL: return "E_FAIL";
		case E_NOTIMPL: return "E_NOTIMPL";
		case E_NOINTERFACE: return "E_NOINTERFACE";
		default:
			std::ostringstream s;
			s << std::hex << result;
			return s.str();
		}
	}
}

namespace vrperfkit {

	void CheckResult(const std::string &action, HRESULT result) {
		if (FAILED(result)) {
			std::string message = "Failed " + action + ": " + MapResult(result);
			throw std::exception(message.c_str());
		}
	}
	
	DXGI_FORMAT TranslateTypelessFormats(DXGI_FORMAT format) {
		switch (format) {
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case DXGI_FORMAT_R32G32B32_TYPELESS:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
			return DXGI_FORMAT_R10G10B10A2_UINT;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		default:
			return format;
		}
	}

	ComPtr<ID3D11ShaderResourceView> CreateShaderResourceView(ID3D11Device *device, ID3D11Texture2D *texture, int arrayIndex) {
		D3D11_TEXTURE2D_DESC td;
		texture->GetDesc(&td);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
		srvd.Format = TranslateTypelessFormats(td.Format);
		if (td.ArraySize > 1) {
			srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvd.Texture2DArray.ArraySize = 1;
			srvd.Texture2DArray.FirstArraySlice = D3D11CalcSubresource(0, arrayIndex, td.MipLevels);
			srvd.Texture2DArray.MipLevels = td.MipLevels;
			srvd.Texture2DArray.MostDetailedMip = 0;
		}
		else {
			srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvd.Texture2D.MipLevels = td.MipLevels;
			srvd.Texture2D.MostDetailedMip = 0;
		}

		ComPtr<ID3D11ShaderResourceView> srv;
		CheckResult("creating shader resource view", device->CreateShaderResourceView(texture, &srvd, srv.GetAddressOf()));
		return srv;
	}

	ComPtr<ID3D11UnorderedAccessView> CreateUnorderedAccessView(ID3D11Device *device, ID3D11Texture2D *texture, int arrayIndex) {
		D3D11_TEXTURE2D_DESC td;
		texture->GetDesc(&td);

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
		uavd.Format = td.Format;
		if (td.ArraySize > 1) {
			uavd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			uavd.Texture2DArray.ArraySize = 1;
			uavd.Texture2DArray.FirstArraySlice = D3D11CalcSubresource(0, arrayIndex, td.MipLevels);
			uavd.Texture2DArray.MipSlice = 0;
		}
		else {
			uavd.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			uavd.Texture2D.MipSlice = 0;
		}

		ComPtr<ID3D11UnorderedAccessView> uav;
		CheckResult("creating unordered access view", device->CreateUnorderedAccessView(texture, &uavd, uav.GetAddressOf()));
		return uav;
	}

	ComPtr<ID3D11Texture2D> CreateResolveTexture(ID3D11Device *device, ID3D11Texture2D *texture) {
		D3D11_TEXTURE2D_DESC td;
		texture->GetDesc(&td);
		td.SampleDesc.Count = 1;
		td.SampleDesc.Quality = 0;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		td.CPUAccessFlags = 0;
		td.MiscFlags = 0;
		td.MipLevels = 1;

		ComPtr<ID3D11Texture2D> tex;
		CheckResult("creating resolve texture", device->CreateTexture2D(&td, nullptr, tex.GetAddressOf()));
		return tex;
	}
}
