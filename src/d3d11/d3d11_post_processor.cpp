#include "d3d11_post_processor.h"

#include "config.h"
#include "d3d11_fsr_upscaler.h"

namespace vrperfkit {
	D3D11PostProcessor::D3D11PostProcessor(ComPtr<ID3D11Device> device) : device(device) {}

	bool D3D11PostProcessor::Apply(const D3D11PostProcessInput &input, Viewport &outputViewport) {
		if (g_config.upscaling.enabled) {
			PrepareUpscaler(input.outputTexture);
			D3D11_TEXTURE2D_DESC td;
			input.outputTexture->GetDesc(&td);
			outputViewport.x = outputViewport.y = 0;
			outputViewport.width = td.Width;
			outputViewport.height = td.Height;
			if (input.mode == TextureMode::COMBINED) {
				outputViewport.width /= 2;
				if (input.eye == RIGHT_EYE) {
					outputViewport.x += outputViewport.width;
				}
			}
			upscaler->Upscale(input, outputViewport);
			return true;
		}
		return false;
	}

	void D3D11PostProcessor::PrepareUpscaler(ID3D11Texture2D *outputTexture) {
		if (upscaler == nullptr || upscaleMethod != g_config.upscaling.method) {
			D3D11_TEXTURE2D_DESC td;
			outputTexture->GetDesc(&td);
			upscaleMethod = g_config.upscaling.method;
			switch (upscaleMethod) {
			case UpscaleMethod::FSR:
				upscaler.reset(new D3D11FsrUpscaler(device.Get(), td.Width, td.Height, td.Format));
				break;
			case UpscaleMethod::NIS:
				// FIXME implement NIS upscaler
				upscaler.reset(new D3D11FsrUpscaler(device.Get(), td.Width, td.Height, td.Format));
				break;
			}
		}
	}
}
