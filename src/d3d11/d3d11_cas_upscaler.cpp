#include "d3d11_cas_upscaler.h"
#include "d3d11_helper.h"
#include "logging.h"
#include "shader_cas_upscale.h"
#include "shader_cas_sharpen.h"
#include "config.h"

#include "nis/NIS_Config.h"

#define A_CPU 1
#include "cas/ffx_a.h"
#include "cas/ffx_cas.h"

namespace vrperfkit {
	struct ShaderConstants {
		uint32_t const0[4];
		uint32_t const1[4];
		uint32_t inputOffset[2];
		uint32_t outputOffset[2];
		uint32_t inputTextureSize[2];
		uint32_t outputTextureSize[2];
		uint32_t projCentre[2];
		uint32_t squaredRadius;
		uint32_t debugMode;
	};

	D3D11CasUpscaler::D3D11CasUpscaler(ID3D11Device *device) {
		LOG_INFO << "Creating D3D11 resources for CAS upscaling...";
		device->GetImmediateContext(context.GetAddressOf());

		CheckResult("creating CAS upscale shader", device->CreateComputeShader(g_CASUpscaleShader, sizeof(g_CASUpscaleShader), nullptr, upscaleShader.GetAddressOf()));
		CheckResult("creating CAS sharpen shader", device->CreateComputeShader(g_CASSharpenShader, sizeof(g_CASSharpenShader), nullptr, sharpenShader.GetAddressOf()));

		constantsBuffer = CreateConstantsBuffer(device, sizeof(ShaderConstants));
		sampler = CreateLinearSampler(device);
	}

	void D3D11CasUpscaler::Upscale(const D3D11PostProcessInput &input, const Viewport &outputViewport) {
		D3D11_TEXTURE2D_DESC td, otd;
		input.inputTexture->GetDesc(&td);
		input.outputTexture->GetDesc(&otd);

		context->CSSetSamplers(0, 1, sampler.GetAddressOf());
		ID3D11ShaderResourceView *srvs[1] = {input.inputView};
		context->CSSetShaderResources(0, 1, srvs);
		UINT uavCount = -1;
		ID3D11UnorderedAccessView *uavs[] = {input.outputUav};
		context->CSSetUnorderedAccessViews(0, 1, uavs, &uavCount);

		ShaderConstants constants;
		CasSetup(constants.const0, constants.const1, g_config.upscaling.sharpness, 
				input.inputViewport.width, input.inputViewport.height,
				outputViewport.width, outputViewport.height);
		constants.inputOffset[0] = input.inputViewport.x;
		constants.inputOffset[1] = input.inputViewport.y;
		constants.outputOffset[0] = outputViewport.x;
		constants.outputOffset[1] = outputViewport.y;
		constants.inputTextureSize[0] = td.Width;
		constants.inputTextureSize[1] = td.Height;
		constants.outputTextureSize[0] = otd.Width;
		constants.outputTextureSize[1] = otd.Height;
		float radius = 0.5f * g_config.upscaling.radius * outputViewport.height;
		constants.projCentre[0] = outputViewport.width * input.projectionCenter.x;
		constants.projCentre[1] = outputViewport.height * input.projectionCenter.y;
		constants.squaredRadius = radius * radius;
		constants.debugMode = g_config.debugMode;
		context->UpdateSubresource(constantsBuffer.Get(), 0, nullptr, &constants, 0, 0);
		context->CSSetConstantBuffers(0, 1, constantsBuffer.GetAddressOf());

		if (input.inputViewport != outputViewport) {
			// full upscaling pass
			context->CSSetShader(upscaleShader.Get(), nullptr, 0);
		} else {
			// just sharpening
			context->CSSetShader(sharpenShader.Get(), nullptr, 0);
		}
		context->Dispatch((outputViewport.width + 15) >> 4, (outputViewport.height + 15) >> 4, 1);
	}
}
