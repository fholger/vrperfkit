// The MIT License(MIT)
// 
// Copyright(c) 2021 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files(the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#define NIS_SCALER 0
#define NIS_HDR_MODE 0
#define NIS_BLOCK_WIDTH 32
#define NIS_BLOCK_HEIGHT 32
#define NIS_THREAD_GROUP_SIZE 256
#define NIS_VIEWPORT_SUPPORT 1

#include "NIS_Common.h"
#include "NIS_Scaler.h"

[numthreads(NIS_THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 blockIdx : SV_GroupID, uint3 threadIdx : SV_GroupThreadID)
{
	uint2 groupCentre = uint2((blockIdx.x * 32) + 16, (blockIdx.y * 32) + 16);
	uint2 dc = projCentre.xy - groupCentre;
	if (dot(dc, dc) <= squaredRadius) {
		NVSharpen(blockIdx.xy, threadIdx.x);
	}
	else {
		DirectCopy(blockIdx.xy, threadIdx.x);
	}
}
