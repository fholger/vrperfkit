#include "d3d11_nis_upscaler.h"
#include "d3d11_helper.h"
#include "logging.h"
#include "shader_nis_upscale.h"
#include "shader_nis_sharpen.h"
#include "config.h"

#include "nis/NIS_Config.h"

namespace vrperfkit {
	D3D11NisUpscaler::D3D11NisUpscaler(ID3D11Device *device) {
		LOG_INFO << "Creating D3D11 resources for NIS upscaling...";
		device->GetImmediateContext(context.GetAddressOf());

		CheckResult("creating NIS upscale shader", device->CreateComputeShader(g_NISUpscaleShader, sizeof(g_NISUpscaleShader), nullptr, upscaleShader.GetAddressOf()));
		CheckResult("creating NIS sharpen shader", device->CreateComputeShader(g_NISSharpenShader, sizeof(g_NISSharpenShader), nullptr, sharpenShader.GetAddressOf()));

		constantsBuffer = CreateConstantsBuffer(device, sizeof(NISConfig));
		sampler = CreateLinearSampler(device);

		D3D11_TEXTURE2D_DESC td;
		td.Width = kFilterSize / 4;
		td.Height = kPhaseCount;
		td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.SampleDesc.Count = 1;
		td.SampleDesc.Quality = 0;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.CPUAccessFlags = 0;
		td.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA texData;
		texData.pSysMem = coef_scale;
		texData.SysMemPitch = kFilterSize * 4;
		texData.SysMemSlicePitch = kFilterSize * 4 * kPhaseCount;
		CheckResult("creating NIS upscale coefficients texture", device->CreateTexture2D(&td, &texData, scalerCoeffTexture.GetAddressOf()));
		texData.pSysMem = coef_usm;
		CheckResult("creating NIS USM coefficients texture", device->CreateTexture2D(&td, &texData, usmCoeffTexture.GetAddressOf()));

		scalerCoeffView = CreateShaderResourceView(device, scalerCoeffTexture.Get());
		usmCoeffView = CreateShaderResourceView(device, usmCoeffTexture.Get());
	}

	void D3D11NisUpscaler::Upscale(const D3D11PostProcessInput &input, const Viewport &outputViewport) {
		D3D11_TEXTURE2D_DESC td, otd;
		input.inputTexture->GetDesc(&td);
		input.outputTexture->GetDesc(&otd);

		context->CSSetSamplers(0, 1, sampler.GetAddressOf());
		ID3D11ShaderResourceView *srvs[1] = {input.inputView};
		context->CSSetShaderResources(0, 1, srvs);
		UINT uavCount = -1;
		ID3D11UnorderedAccessView *uavs[] = {input.outputUav};
		context->CSSetUnorderedAccessViews(0, 1, uavs, &uavCount);

		NISConfig constants;
		NVScalerUpdateConfig(constants, g_config.upscaling.sharpness, input.inputViewport.x, input.inputViewport.y,
				input.inputViewport.width, input.inputViewport.height, td.Width, td.Height,
				outputViewport.x, outputViewport.y, outputViewport.width, outputViewport.height,
				otd.Width, otd.Height);
		float radius = 0.5f * g_config.upscaling.radius * outputViewport.height;
		constants.projCentre[0] = outputViewport.width * input.projectionCenter.x;
		constants.projCentre[1] = outputViewport.height * input.projectionCenter.y;
		constants.squaredRadius = radius * radius;
		constants.debugMode = g_config.debugMode;
		context->UpdateSubresource(constantsBuffer.Get(), 0, nullptr, &constants, 0, 0);
		context->CSSetConstantBuffers(0, 1, constantsBuffer.GetAddressOf());

		if (input.inputViewport != outputViewport) {
			// full upscaling pass
			ID3D11ShaderResourceView *coeffViews[2] = {scalerCoeffView.Get(), usmCoeffView.Get()};
			context->CSSetShaderResources(1, 2, coeffViews);
			context->CSSetShader(upscaleShader.Get(), nullptr, 0);
			context->Dispatch((UINT)std::ceil(outputViewport.width / 32.f), (UINT)std::ceil(outputViewport.height / 24.f), 1);
		} else {
			// just sharpening
			context->CSSetShader(sharpenShader.Get(), nullptr, 0);
			context->Dispatch((UINT)std::ceil(outputViewport.width / 32.f), (UINT)std::ceil(outputViewport.height / 32.f), 1);
		}
	}
}
