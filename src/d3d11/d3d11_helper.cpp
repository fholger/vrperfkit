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

	ComPtr<ID3D11ShaderResourceView> CreateShaderResourceView(ID3D11Device *device, ID3D11Texture2D *texture, int arrayIndex) {
		D3D11_TEXTURE2D_DESC td;
		texture->GetDesc(&td);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
		srvd.Format = td.Format;
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

}
