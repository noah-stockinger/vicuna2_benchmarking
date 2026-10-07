/*
 * Copyright (C) 2010-2021 Arm Limited or its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modifications copyright (C) 2021-2022 Chair of Electronic Design Automation, TUM
 *
 * Ported from muRISCV-NN f694944, Source/PoolingFunctions/muriscv_nn_max_pool_s8.c
 * (https://github.com/tum-ei-eda/muriscv-nn). Changes:
 *  - only the USE_VEXT path is kept (P-extension and scalar fallbacks removed)
 *  - RVV intrinsics replaced 1:1 by inline asm, one asm block per loop body: GCC 16 no longer
 *    provides the unprefixed intrinsic names, and riscv_vector.h fails to compile in C++ here
 * Loop structure and vector instructions are unchanged.
 */

#include <cstring>

#include "maxpool_muriscv.hpp"

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

static inline void compare_and_replace_if_larger(int8_t *base, const int8_t *target, int32_t length)
{
    /* We are operating on vl elements at a time. */
    for (size_t cnt = length, vl = 0; cnt > 0; cnt -= vl, base += vl, target += vl)
    {
        /* vl = vsetvl_e8m8(cnt); op_1 = vle8_v_i8m8(base, vl); op_2 = vle8_v_i8m8(target, vl);
         * max = vmax_vv_i8m8(op_1, op_2, vl); vse8_v_i8m8(base, max, vl); */
        asm volatile(
            "vsetvli %[vl], %[cnt], e8, m8, ta, ma\n"
            "vle8.v  v0, (%[base])\n"
            "vle8.v  v8, (%[target])\n"
            "vmax.vv v0, v0, v8\n"
            "vse8.v  v0, (%[base])\n"
            : [vl] "=&r"(vl)
            : [cnt] "r"(cnt), [base] "r"(base), [target] "r"(target)
            : "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
              "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "memory");
    }
}

static inline void clamp_output(int8_t *source, int32_t length, const int32_t act_min, const int32_t act_max)
{
    /* We are operating on vl elements at a time. */
    for (size_t cnt = length, vl = 0; cnt > 0; cnt -= vl, source += vl)
    {
        /* vl = vsetvl_e8m8(cnt); val = vle8_v_i8m8(source, vl); val = vmax_vx_i8m8(val, act_min, vl);
         * val = vmin_vx_i8m8(val, act_max, vl); vse8_v_i8m8(source, val, vl); */
        asm volatile(
            "vsetvli %[vl], %[cnt], e8, m8, ta, ma\n"
            "vle8.v  v0, (%[src])\n"
            "vmax.vx v0, v0, %[amin]\n"
            "vmin.vx v0, v0, %[amax]\n"
            "vse8.v  v0, (%[src])\n"
            : [vl] "=&r"(vl)
            : [cnt] "r"(cnt), [src] "r"(source), [amin] "r"(act_min), [amax] "r"(act_max)
            : "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "memory");
    }
}

muriscv_nn_status muriscv_nn_max_pool_s8(const muriscv_nn_context *ctx,
                                         const muriscv_nn_pool_params *pool_params,
                                         const muriscv_nn_dims *input_dims,
                                         const q7_t *src,
                                         const muriscv_nn_dims *filter_dims,
                                         const muriscv_nn_dims *output_dims,
                                         q7_t *dst)
{
    const int32_t input_y = input_dims->h;
    const int32_t input_x = input_dims->w;
    const int32_t output_y = output_dims->h;
    const int32_t output_x = output_dims->w;
    const int32_t stride_y = pool_params->stride.h;
    const int32_t stride_x = pool_params->stride.w;
    const int32_t kernel_y = filter_dims->h;
    const int32_t kernel_x = filter_dims->w;
    const int32_t pad_y = pool_params->padding.h;
    const int32_t pad_x = pool_params->padding.w;
    const int32_t act_min = pool_params->activation.min;
    const int32_t act_max = pool_params->activation.max;
    const int32_t channel_in = input_dims->c;
    (void)ctx;
    int8_t *dst_base = dst;

    if (input_dims->n < 1)
    {
        return MURISCV_NN_ARG_ERROR;
    }

    for (int32_t batch = 0; batch < input_dims->n; batch++)
    {
        for (int32_t i_y = 0, base_idx_y = -pad_y; i_y < output_y; base_idx_y += stride_y, i_y++)
        {
            for (int32_t i_x = 0, base_idx_x = -pad_x; i_x < output_x; base_idx_x += stride_x, i_x++)
            {
                /* Condition for kernel start dimension: (base_idx_<x,y> + kernel_<x,y>_start) >= 0 */
                const int32_t ker_y_start = MAX(0, -base_idx_y);
                const int32_t ker_x_start = MAX(0, -base_idx_x);

                /* Condition for kernel end dimension: (base_idx_<x,y> + kernel_<x,y>_end) < dim_src_<width,height> */
                const int32_t kernel_y_end = MIN(kernel_y, input_y - base_idx_y);
                const int32_t kernel_x_end = MIN(kernel_x, input_x - base_idx_x);

                int32_t count = 0;

                for (int32_t k_y = ker_y_start; k_y < kernel_y_end; k_y++)
                {
                    for (int32_t k_x = ker_x_start; k_x < kernel_x_end; k_x++)
                    {
                        const int8_t *start = src + channel_in * (k_x + base_idx_x + (k_y + base_idx_y) * input_x);

                        if (count == 0)
                        {
                            memcpy(dst, start, channel_in);
                            count++;
                        }
                        else
                        {
                            compare_and_replace_if_larger(dst, start, channel_in);
                        }
                    }
                }
                /* 'count' is expected to be non-zero here. */
                dst += channel_in;
            }
        }

        clamp_output(dst_base, output_x * output_y * channel_in, act_min, act_max);
        dst_base = dst;

        src += input_x * input_y * channel_in;
    }

    return MURISCV_NN_SUCCESS;
}
