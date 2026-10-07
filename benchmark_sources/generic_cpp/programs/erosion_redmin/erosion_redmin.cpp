#include "erosion_redmin.hpp"

/*
 * Grayscale erosion with the reduction unit: each output pixel is one vredminu.vs chain over
 * the window, one contiguous row load (vle8.v) per window row. Same mapping as maxpool_redmax,
 * with 1 channel and stride 1.
 *
 * LMUL 1 on purpose: the reduction unit always streams the whole register group, so a larger
 * LMUL only adds cycles for rows of <= 16 bytes. The seed is written at LMUL 1 anyway, because
 * vmv.s.x at LMUL > 1 hangs the pipeline on core e54481e.
 */

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define ROW_MAX 16 // VLMAX at e8,m1 with VLEN 128

static inline uint8_t window_min(const uint8_t *row, int32_t rows, int32_t row_len, int32_t row_stride)
{
    uint32_t res;
    asm volatile(
        "vsetivli zero, 1, e8, m1, ta, ma\n"
        "vmv.s.x  v16, %[seed]\n"               // identity for unsigned min: 255
        "1:\n"
        "vsetvli  zero, %[len], e8, m1, ta, ma\n"
        "vle8.v   v8, (%[row])\n"               // one window row
        "vredminu.vs v16, v8, v16\n"
        "add      %[row], %[row], %[rs]\n"
        "addi     %[rows], %[rows], -1\n"
        "bnez     %[rows], 1b\n"
        "vmv.x.s  %[res], v16\n"
        : [res] "=&r"(res), [row] "+&r"(row), [rows] "+&r"(rows)
        : [seed] "r"(255), [len] "r"(row_len), [rs] "r"(row_stride)
        : "v8", "v16", "memory");
    return (uint8_t)res;
}

int erosion_redmin_u8(const uint8_t *src, uint8_t *dst, int32_t height, int32_t width, int32_t k)
{
    if (k % 2 == 0 || k > ROW_MAX)
    {
        return -1;
    }
    const int32_t r = k / 2; // anchor in the centre

    for (int32_t y = 0; y < height; y++)
    {
        // window rows clipped to the image
        const int32_t y0 = MAX(0, y - r);
        const int32_t y1 = MIN(height, y + r + 1);
        for (int32_t x = 0; x < width; x++)
        {
            const int32_t x0 = MAX(0, x - r);
            const int32_t x1 = MIN(width, x + r + 1);
            dst[y * width + x] = window_min(src + y0 * width + x0, y1 - y0, x1 - x0, width);
        }
    }
    return 0;
}
