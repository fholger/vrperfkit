cbuffer cb : register(b0) {
	uint4 const0;
	uint4 const1;
	uint2 inputOffset;
	uint2 outputOffset;
	uint2 inputTextureSize;
	uint2 outputTextureSize;
	uint2 projCentre;
	uint squaredRadius;
	uint debugMode;
};

SamplerState samLinearClamp : register(s0);
Texture2D InputTexture : register(t0);
RWTexture2D<float4> OutputTexture : register(u0);

#define A_GPU 1
#define A_HLSL 1
#define CAS_BETTER_DIAGONALS 1

#include "ffx_a.h"

AF3 CasLoad(ASU2 p) {
	return InputTexture.Load(int3(p + inputOffset, 0)).rgb;
}

// for transforming to linear color space, not needed (?)
void CasInput(inout AF1 r, inout AF1 g, inout AF1 b) {}

#include "ffx_cas.h"

#if CAS_SHARPEN_ONLY
#define WITHOUT_UPSCALE true
#else
#define WITHOUT_UPSCALE false
#endif

void Cas(int2 pos) {
	AF3 c;
	CasFilter(c.r, c.g, c.b, pos, const0, const1, WITHOUT_UPSCALE);
	OutputTexture[ASU2(pos)+outputOffset] = AF4(c, 1);
}

void Bilinear(int2 pos) {
	AF4 mul = AF4(1, 1, 1, 1) - debugMode * AF4(0, 0.3, 0.3, 0);
	float2 samplePos = ((float2(pos) + 0.5) * AF2_AU2(const0.xy) + float2(inputOffset)) / float2(inputTextureSize);
	//float2 samplePos = (float2(pos + outputOffset) + 0.5) / outputTextureSize;
	AF3 c = InputTexture.SampleLevel(samLinearClamp, samplePos, 0).rgb;
	OutputTexture[ASU2(pos) + outputOffset] = AF4(c, 1) * mul;
}

[numthreads(64, 1, 1)]
void main(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID) {
	AU2 gxy = ARmp8x8( LocalThreadId.x ) + AU2(WorkGroupId.x << 4u, WorkGroupId.y << 4u);

	AU2 groupCentre = AU2((WorkGroupId.x << 4u) + 8u, (WorkGroupId.y << 4u) + 8u);
	AU2 dc = projCentre - groupCentre;
	if (dot(dc, dc) <= squaredRadius) {
		// only apply CAS for workgroups inside the configured radius
		Cas(gxy);
		gxy.x += 8u;
		Cas(gxy);
		gxy.y += 8u;
		Cas(gxy);
		gxy.x -= 8u;
		Cas(gxy);
	}
	else {
		// resort to cheaper bilinear sampling
		Bilinear(gxy);
		gxy.x += 8u;
		Bilinear(gxy);
		gxy.y += 8u;
		Bilinear(gxy);
		gxy.x -= 8u;
		Bilinear(gxy);
	}
}
