#include "d3d11_helper.h"

#include "logging.h"

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

	ComPtr<ID3D11Buffer> CreateConstantsBuffer(ID3D11Device *device, uint32_t size) {
		D3D11_BUFFER_DESC bd;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.ByteWidth = size;
		ComPtr<ID3D11Buffer> buffer;
		CheckResult("creating constants buffer", device->CreateBuffer(&bd, nullptr, buffer.GetAddressOf()));
		return buffer;
	}

	ComPtr<ID3D11SamplerState> CreateLinearSampler(ID3D11Device *device) {
		D3D11_SAMPLER_DESC sd;
		sd.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sd.MipLODBias = 0;
		sd.MaxAnisotropy = 1;
		sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sd.MinLOD = 0;
		sd.MaxLOD = 0;
		ComPtr<ID3D11SamplerState> sampler;
		CheckResult("creating sampler state", device->CreateSamplerState(&sd, sampler.GetAddressOf()));
		return sampler;
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

	DXGI_FORMAT MakeSrgbFormatsTypeless(DXGI_FORMAT format) {
		switch (format) {
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_TYPELESS;
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8X8_TYPELESS;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_TYPELESS;
		default:
			return format;
		}
	}

	bool IsSrgbFormat(DXGI_FORMAT format) {
		switch (format) {
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}

	void StoreD3D11State(ID3D11DeviceContext *context, D3D11State &state) {
		context->VSGetShader(state.vertexShader.ReleaseAndGetAddressOf(), nullptr, nullptr);
		context->PSGetShader(state.pixelShader.ReleaseAndGetAddressOf(), nullptr, nullptr);
		context->CSGetShader(state.computeShader.ReleaseAndGetAddressOf(), nullptr, nullptr);
		context->IAGetInputLayout( state.inputLayout.ReleaseAndGetAddressOf() );
		context->IAGetPrimitiveTopology( &state.topology );
		context->IAGetVertexBuffers( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, state.vertexBuffers, state.strides, state.offsets );
		context->IAGetIndexBuffer(state.indexBuffer.ReleaseAndGetAddressOf(), &state.format, &state.offset);
		context->OMGetRenderTargets( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, state.renderTargets, state.depthStencil.GetAddressOf() );
		context->RSGetState( state.rasterizerState.ReleaseAndGetAddressOf() );
		context->OMGetDepthStencilState( state.depthStencilState.ReleaseAndGetAddressOf(), &state.stencilRef );
		context->RSGetViewports( &state.numViewports, nullptr );
		context->RSGetViewports( &state.numViewports, state.viewports );
		context->VSGetConstantBuffers( 0, 1, state.vsConstantBuffer.GetAddressOf() );
		context->PSGetConstantBuffers( 0, 1, state.psConstantBuffer.GetAddressOf() );
		context->CSGetConstantBuffers(0, 1, state.csConstantBuffer.GetAddressOf());
		context->CSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, state.csShaderResources);
		context->CSGetUnorderedAccessViews(0, D3D11_1_UAV_SLOT_COUNT, state.csUavs);
	}

	void RestoreD3D11State(ID3D11DeviceContext *context, const D3D11State &state) {
		context->VSSetShader(state.vertexShader.Get(), nullptr, 0);
		context->PSSetShader(state.pixelShader.Get(), nullptr, 0);
		context->CSSetShader(state.computeShader.Get(), nullptr, 0);
		context->IASetInputLayout( state.inputLayout.Get() );
		context->IASetPrimitiveTopology( state.topology );
		context->IASetVertexBuffers( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, state.vertexBuffers, state.strides, state.offsets );
		for (int i = 0; i < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i) {
			if (state.vertexBuffers[i])
				state.vertexBuffers[i]->Release();
		}
		context->IASetIndexBuffer( state.indexBuffer.Get(), state.format, state.offset );
		context->OMSetRenderTargets( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, state.renderTargets, state.depthStencil.Get() );
		for (int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
			if (state.renderTargets[i]) {
				state.renderTargets[i]->Release();
			}
		}
		context->RSSetState( state.rasterizerState.Get() );
		context->OMSetDepthStencilState( state.depthStencilState.Get(), state.stencilRef );
		context->RSSetViewports( state.numViewports, state.viewports );
		context->VSSetConstantBuffers( 0, 1, state.vsConstantBuffer.GetAddressOf() );
		context->PSSetConstantBuffers( 0, 1, state.psConstantBuffer.GetAddressOf() );
		context->CSSetConstantBuffers(0, 1, state.csConstantBuffer.GetAddressOf());
		context->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, state.csShaderResources);
		for (int i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; ++i) {
			if (state.csShaderResources[i])
				state.csShaderResources[i]->Release();
		}
		UINT initial = 0;
		context->CSSetUnorderedAccessViews(0, D3D11_1_UAV_SLOT_COUNT, state.csUavs, &initial);
		for (int i = 0; i < D3D11_1_UAV_SLOT_COUNT; ++i) {
			if (state.csUavs[i])
				state.csUavs[i]->Release();
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
		uavd.Format = TranslateTypelessFormats(td.Format);
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
		LOG_DEBUG << "Create UAV for texture of format " << uavd.Format << " and view dimension " << uavd.ViewDimension;
		CheckResult("creating unordered access view", device->CreateUnorderedAccessView(texture, &uavd, uav.GetAddressOf()));
		return uav;
	}

	ComPtr<ID3D11Texture2D> CreateResolveTexture(ID3D11Device *device, ID3D11Texture2D *texture, DXGI_FORMAT format) {
		D3D11_TEXTURE2D_DESC td;
		texture->GetDesc(&td);
		td.SampleDesc.Count = 1;
		td.SampleDesc.Quality = 0;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		td.CPUAccessFlags = 0;
		td.MiscFlags = 0;
		td.MipLevels = 1;
		if (format != DXGI_FORMAT_UNKNOWN) {
			td.Format = format;
		}

		ComPtr<ID3D11Texture2D> tex;
		CheckResult("creating resolve texture", device->CreateTexture2D(&td, nullptr, tex.GetAddressOf()));
		return tex;
	}

	ComPtr<ID3D11Texture2D> CreatePostProcessTexture(ID3D11Device *device, uint32_t width, uint32_t height, DXGI_FORMAT format) {
		D3D11_TEXTURE2D_DESC td;
		td.Width = width;
		td.Height = height;
		td.Format = format;
		td.MipLevels = 1;
		td.SampleDesc.Count = 1;
		td.SampleDesc.Quality = 0;
		td.ArraySize = 1;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		td.CPUAccessFlags = 0;
		td.MiscFlags = 0;
		ComPtr<ID3D11Texture2D> texture;
		CheckResult("creating post-process texture", device->CreateTexture2D(&td, nullptr, texture.GetAddressOf()));
		return texture;
	}
}
