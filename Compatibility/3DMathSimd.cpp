// Copyright (c) 2024, OpenEmu Team
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the OpenEmu Team nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY OpenEmu Team ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL OpenEmu Team BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "3DMath.h"
#include <cmath>
#include "Log.h"
#include "Types.h"
#include <simd/simd.h>

// Note that this has not been thouroughly tested!

static inline simd::float4 simd_make_float4(const float m0[4])
{
	simd::float4 val = simd_make_float4(m0[0], m0[1], m0[2], m0[3]);
	return val;
}

void MultMatrix(float m0[4][4], float m1[4][4], float dest[4][4])
{
	// Load m0
	simd::float4x4 _m0(simd_make_float4(m0[0]),
					   simd_make_float4(m0[1]),
					   simd_make_float4(m0[2]),
					   simd_make_float4(m0[3]));

	// Load m1
	simd::float4x4 _m1(simd_make_float4(m1[0]),
					   simd_make_float4(m1[1]),
					   simd_make_float4(m1[2]),
					   simd_make_float4(m1[3]));

	// Huh, that was easy.
	simd::float4x4 _dest = _m0 * _m1;
	
	memcpy(dest[0], &_dest.columns[0], sizeof(float) * 4);
	memcpy(dest[1], &_dest.columns[1], sizeof(float) * 4);
	memcpy(dest[2], &_dest.columns[2], sizeof(float) * 4);
	memcpy(dest[3], &_dest.columns[3], sizeof(float) * 4);
}

void MultMatrix2(float m0[4][4], float m1[4][4])
{
	MultMatrix(m0, m1, m0);
}

void TransformVectorNormalize(float vec[3], float mtx[4][4])
{
	// Load mtx
	simd::float4x4 _mtx(simd_make_float4(mtx[0]),
						simd_make_float4(mtx[1]),
						simd_make_float4(mtx[2]),
						simd_make_float4(mtx[3]));

	// Multiply and add
	simd::float4 product = _mtx.columns[0] * vec[0];
	product += _mtx.columns[1] * vec[1];
	product += _mtx.columns[2] * vec[2];

	// Normalize
	product = simd::normalize(product);

	// Store mtx
	vec[0] = product.x;
	vec[1] = product.y;
	vec[2] = product.z;
}

void InverseTransformVectorNormalize(float src[3], float dst[3], float mtx[4][4])
{
	// Load mtx
	simd::float4x4 _mtx = simd_matrix_from_rows(simd_make_float4(mtx[0]),
												simd_make_float4(mtx[1]),
												simd_make_float4(mtx[2]),
												simd_make_float4(mtx[3]));
	
	// Multiply and add
	simd::float4 product = _mtx.columns[0] * src[0];
	product += _mtx.columns[1] * src[1];
	product += _mtx.columns[2] * src[2];

	// Normalize
	product = simd::normalize(product);

	// Store mtx
	dst[0] = product.x;
	dst[1] = product.y;
	dst[2] = product.z;
}

void Normalize(float v[3])
{
	// Load vector
	simd::float3 product = {v[0], v[1], v[2]};
	
	// Normalize
	product = simd::normalize(product);

	// Store vector
	v[0] = product.x;
	v[1] = product.y;
	v[2] = product.z;
}

void InverseTransformVectorNormalizeN(float src[][3], float dst[][3], float mtx[4][4], u32 count)
{
	for (u32 i = 0; i < count; i++)
	{
		InverseTransformVectorNormalize(static_cast<float*>(src[i]), static_cast<float*>(dst[i]), mtx);
	}
}

void CopyMatrix( float m0[4][4], float m1[4][4] )
{
	// We'll cheat!
	memcpy( m0, m1, 16 * sizeof( float ) );
}
