#include "d3d11_fsr_upscaler.h"

#include "d3d11_helper.h"
#include "logging.h"
#include "shader_fsr_easu.h"
#include "shader_fsr_rcas.h"

#define A_CPU
#include "config.h"
#include "fsr/ffx_a.h"
#include "fsr/ffx_fsr1.h"

namespace vrperfkit {
	struct UpscaleShaderConstants {
		AU1 const0[4];
		AU1 const1[4];
		AU1 const2[4];
		AU1 const3[4]; // store output offset in final 2
		AU1 projCentre[2];
		AU1 squaredRadius;
		AU1 _padding;
	};

	struct SharpenShaderConstants {
		AU1 const0[4]; // store output offset in final 2
		AU1 projCentre[2];
		AU1 squaredRadius;
		AU1 debugMode;
	};

	D3D11FsrUpscaler::D3D11FsrUpscaler(ID3D11Device *device, uint32_t outputWidth, uint32_t outputHeight, DXGI_FORMAT format) {
		LOG_INFO << "Creating D3D11 resources for FSR upscaling...";
		CheckResult("creating FSR upscale shader", device->CreateComputeShader(g_FSRUpscaleShader, sizeof(g_FSRUpscaleShader), nullptr, upscaleShader.GetAddressOf()));
		CheckResult("creating FSR sharpen shader", device->CreateComputeShader(g_FSRSharpenShader, sizeof(g_FSRSharpenShader), nullptr, sharpenShader.GetAddressOf()));

		constantsBuffer = CreateConstantsBuffer(device, max(sizeof(UpscaleShaderConstants), sizeof(SharpenShaderConstants)));
		upscaledTexture = CreatePostProcessTexture(device, outputWidth, outputHeight, format);
		upscaledView = CreateShaderResourceView(device, upscaledTexture.Get());
		upscaledUav = CreateUnorderedAccessView(device, upscaledTexture.Get());
		sampler = CreateLinearSampler(device);

		device->GetImmediateContext(context.GetAddressOf());
	}

	void D3D11FsrUpscaler::Upscale(const D3D11PostProcessInput &input, const Viewport &outputViewport) {
		D3D11_TEXTURE2D_DESC td;
		input.inputTexture->GetDesc(&td);

		context->CSSetSamplers(0, 1, sampler.GetAddressOf());
		ID3D11ShaderResourceView *srvs[1] = {input.inputView};
		UINT uavCount = -1;
		ID3D11UnorderedAccessView *uavs[] = {upscaledUav.Get()};
		float radius = 0.5f * g_config.upscaling.radius * outputViewport.height;

		if (input.inputViewport != outputViewport) {
			// upscaling pass
			UpscaleShaderConstants upscaleConstants;
			FsrEasuConOffset(upscaleConstants.const0, upscaleConstants.const1, upscaleConstants.const2, upscaleConstants.const3,
				input.inputViewport.width, input.inputViewport.height, td.Width, td.Height,
				outputViewport.width, outputViewport.height,
				input.inputViewport.x, input.inputViewport.y);
			upscaleConstants.const3[2] = outputViewport.x;
			upscaleConstants.const3[3] = outputViewport.y;
			upscaleConstants.squaredRadius = radius * radius;
			upscaleConstants.projCentre[0] = outputViewport.width * input.projectionCenter.x;
			upscaleConstants.projCentre[1] = outputViewport.height * input.projectionCenter.y;
			context->UpdateSubresource(constantsBuffer.Get(), 0, nullptr, &upscaleConstants, 0, 0);

			context->CSSetUnorderedAccessViews(0, 1, uavs, &uavCount);
			context->CSSetConstantBuffers(0, 1, constantsBuffer.GetAddressOf());
			context->CSSetShaderResources(0, 1, srvs);
			context->CSSetShader(upscaleShader.Get(), nullptr, 0);
			context->Dispatch((outputViewport.width + 15) >> 4, (outputViewport.height + 15) >> 4, 1);
			srvs[0] = upscaledView.Get();
		}

		// sharpening pass
		SharpenShaderConstants sharpenConstants;
		FsrRcasCon(sharpenConstants.const0, 2.f - 2 * g_config.upscaling.sharpness);
		sharpenConstants.const0[2] = outputViewport.x;
		sharpenConstants.const0[3] = outputViewport.y;
		sharpenConstants.squaredRadius = radius * radius;
		sharpenConstants.projCentre[0] = outputViewport.width * input.projectionCenter.x;
		sharpenConstants.projCentre[1] = outputViewport.height * input.projectionCenter.y;
		sharpenConstants.debugMode = g_config.debugMode ? 1 : 0;
		context->UpdateSubresource(constantsBuffer.Get(), 0, nullptr, &sharpenConstants, 0, 0);

		uavs[0] = input.outputUav;
		context->CSSetUnorderedAccessViews(0, 1, uavs, &uavCount);
		context->CSSetConstantBuffers(0, 1, constantsBuffer.GetAddressOf());
		context->CSSetShaderResources(0, 1, srvs);
		context->CSSetShader(sharpenShader.Get(), nullptr, 0);
		context->Dispatch((outputViewport.width + 15) >> 4, (outputViewport.height + 15) >> 4, 1);
	}
}
