
cbuffer cb : register(b0)
{
	float kDetectRatio;
	float kDetectThres;
	float kMinContrastRatio;
	float kRatioNorm;

	float kContrastBoost;
	float kEps;
	float kSharpStartY;
	float kSharpScaleY;

	float kSharpStrengthMin;
	float kSharpStrengthScale;
	float kSharpLimitMin;
	float kSharpLimitScale;

	float kScaleX;
	float kScaleY;

	float kDstNormX;
	float kDstNormY;
	float kSrcNormX;
	float kSrcNormY;

	uint kInputViewportOriginX;
	uint kInputViewportOriginY;
	uint kInputViewportWidth;
	uint kInputViewportHeight;

	uint kOutputViewportOriginX;
	uint kOutputViewportOriginY;
	uint kOutputViewportWidth;
	uint kOutputViewportHeight;

	float reserved0;
	float reserved1;

	uint2 projCentre;
	uint squaredRadius;
	uint debugMode;
};

SamplerState samplerLinearClamp : register(s0);
Texture2D in_texture            : register(t0);
RWTexture2D<unorm float4> out_texture : register(u0);


void DirectCopy(uint2 blockIdx, uint threadIdx)
{
	const float4 mul = float4(1, 1, 1, 1) - debugMode * float4(0, 0.3, 0.3, 0);
	const int dstBlockX = NIS_BLOCK_WIDTH * blockIdx.x;
	const int dstBlockY = NIS_BLOCK_HEIGHT * blockIdx.y;
	for (uint k = threadIdx; k < NIS_BLOCK_WIDTH * NIS_BLOCK_HEIGHT; k += NIS_THREAD_GROUP_SIZE)
	{
		const int2 pos = int2(k % NIS_BLOCK_WIDTH, k / NIS_BLOCK_WIDTH);
		const int dstX = dstBlockX + pos.x + kOutputViewportOriginX;
		const int dstY = dstBlockY + pos.y + kOutputViewportOriginY;
		float3 c = in_texture.SampleLevel(samplerLinearClamp, float2(dstX * kDstNormX, dstY * kDstNormY), 0).rgb;
		out_texture[uint2(dstX, dstY)] = float4(c, 1) * mul;
	}
}
