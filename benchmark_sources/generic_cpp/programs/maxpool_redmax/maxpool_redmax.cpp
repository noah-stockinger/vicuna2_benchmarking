#include "maxpool_redmax.hpp"

/*
 * Max pooling with the reduction unit. Loop nest, padding handling and clamp_output() are the
 * same as in maxpool_muriscv (muRISCV-NN muriscv_nn_max_pool_s8); only the window max differs:
 * muRISCV-NN runs vmax.vv across all channels of a pixel, here each output byte is one
 * vredmax.vs chain over the window, one strided row load (stride = channels) per window row.
 *
 * LMUL 1 on purpose: the reduction unit always streams the whole register group, so a larger
 * LMUL only adds cycles for rows of <= 16 bytes. The seed is written at LMUL 1 anyway, because
 * vmv.s.x at LMUL > 1 hangs the pipeline on core e54481e.
 */

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define ROW_MAX 16 // VLMAX at e8,m1 with VLEN 128

static inline int8_t window_max(const int8_t *row, int32_t rows, int32_t row_len,
                                int32_t elem_stride, int32_t row_stride)
{
    int32_t res;
    asm volatile(
        "vsetivli zero, 1, e8, m1, ta, ma\n"
        "vmv.s.x  v16, %[seed]\n"               // identity for signed max: -128
        "1:\n"
        "vsetvli  zero, %[len], e8, m1, ta, ma\n"
        "vlse8.v  v8, (%[row]), %[es]\n"        // one window row of this channel
        "vredmax.vs v16, v8, v16\n"
        "add      %[row], %[row], %[rs]\n"
        "addi     %[rows], %[rows], -1\n"
        "bnez     %[rows], 1b\n"
        "vmv.x.s  %[res], v16\n"
        : [res] "=&r"(res), [row] "+&r"(row), [rows] "+&r"(rows)
        : [seed] "r"(-128), [len] "r"(row_len), [es] "r"(elem_stride), [rs] "r"(row_stride)
        : "v8", "v16", "memory");
    return (int8_t)res;
}

// Same as clamp_output() in maxpool_muriscv.cpp
static inline void clamp_output(int8_t *source, int32_t length, const int32_t act_min, const int32_t act_max)
{
    for (size_t cnt = length, vl = 0; cnt > 0; cnt -= vl, source += vl)
    {
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

muriscv_nn_status maxpool_redmax_s8(const muriscv_nn_context *ctx,
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

    if (input_dims->n < 1 || kernel_x > ROW_MAX)
    {
        return MURISCV_NN_ARG_ERROR;
    }

    for (int32_t batch = 0; batch < input_dims->n; batch++)
    {
        for (int32_t i_y = 0, base_idx_y = -pad_y; i_y < output_y; base_idx_y += stride_y, i_y++)
        {
            for (int32_t i_x = 0, base_idx_x = -pad_x; i_x < output_x; base_idx_x += stride_x, i_x++)
            {
                const int32_t ker_y_start = MAX(0, -base_idx_y);
                const int32_t ker_x_start = MAX(0, -base_idx_x);
                const int32_t kernel_y_end = MIN(kernel_y, input_y - base_idx_y);
                const int32_t kernel_x_end = MIN(kernel_x, input_x - base_idx_x);

                // top-left pixel of the clipped window, channel 0
                const int8_t *start = src + channel_in * (ker_x_start + base_idx_x + (ker_y_start + base_idx_y) * input_x);

                for (int32_t c = 0; c < channel_in; c++)
                {
                    dst[c] = window_max(start + c, kernel_y_end - ker_y_start, kernel_x_end - ker_x_start,
                                        channel_in, channel_in * input_x);
                }
                dst += channel_in;
            }
        }

        clamp_output(dst_base, output_x * output_y * channel_in, act_min, act_max);
        dst_base = dst;

        src += input_x * input_y * channel_in;
    }

    return MURISCV_NN_SUCCESS;
}
