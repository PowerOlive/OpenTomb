/**
 * OpenAL cross platform audio library
 * Copyright (C) 2014 by Timothy Arceri <t_arceri@yahoo.com.au>.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include "../config.h"

#ifdef HAVE_SSE4
#include <xmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>

#include "../alu.h"
#include "mixer_defs.h"


const ALfloat *Resample_lerp32_SSE41(const ALfloat *src, ALuint frac, ALuint increment,
                                     ALfloat *restrict dst, ALuint numsamples)
{
    const __m128i increment4 = _mm_set1_epi32(increment*4);
    const __m128 fracOne4 = _mm_set1_ps(1.0f/FRACTIONONE);
    const __m128i fracMask4 = _mm_set1_epi32(FRACTIONMASK);
    alignas(16) union { ALuint i[4]; float f[4]; } pos_;
    alignas(16) union { ALuint i[4]; float f[4]; } frac_;
    __m128i frac4, pos4;
    ALuint pos;
    ALuint i;

    InitiatePositionArrays(frac, increment, frac_.i, pos_.i, 4);

    frac4 = _mm_castps_si128(_mm_load_ps(frac_.f));
    pos4 = _mm_castps_si128(_mm_load_ps(pos_.f));

    for(i = 0;numsamples-i > 3;i += 4)
    {
        const __m128 val1 = _mm_setr_ps(src[pos_.i[0]], src[pos_.i[1]], src[pos_.i[2]], src[pos_.i[3]]);
        const __m128 val2 = _mm_setr_ps(src[pos_.i[0]+1], src[pos_.i[1]+1], src[pos_.i[2]+1], src[pos_.i[3]+1]);

        /* val1 + (val2-val1)*mu */
        const __m128 r0 = _mm_sub_ps(val2, val1);
        const __m128 mu = _mm_mul_ps(_mm_cvtepi32_ps(frac4), fracOne4);
        const __m128 out = _mm_add_ps(val1, _mm_mul_ps(mu, r0));

        _mm_store_ps(&dst[i], out);

        frac4 = _mm_add_epi32(frac4, increment4);
        pos4 = _mm_add_epi32(pos4, _mm_srli_epi32(frac4, FRACTIONBITS));
        frac4 = _mm_and_si128(frac4, fracMask4);

        pos_.i[0] = _mm_extract_epi32(pos4, 0);
        pos_.i[1] = _mm_extract_epi32(pos4, 1);
        pos_.i[2] = _mm_extract_epi32(pos4, 2);
        pos_.i[3] = _mm_extract_epi32(pos4, 3);
    }

    pos = pos_.i[0];
    frac = _mm_cvtsi128_si32(frac4);

    for(;i < numsamples;i++)
    {
        dst[i] = lerp(src[pos], src[pos+1], frac * (1.0f/FRACTIONONE));

        frac += increment;
        pos  += frac>>FRACTIONBITS;
        frac &= FRACTIONMASK;
    }
    return dst;
}
#endif
