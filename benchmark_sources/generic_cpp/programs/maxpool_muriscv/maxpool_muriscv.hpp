#ifndef MAXPOOL_MURISCV_HPP
#define MAXPOOL_MURISCV_HPP
#include <cstdint>

#include "vicuna_crt.hpp"
#include "uart.hpp"

/*
 * The types muriscv_nn_max_pool_s8() needs, copied from muRISCV-NN f694944
 * (Include/muriscv_nn_types.h, Include/muriscv_nn_math_types.h). Only the used fields are kept.
 */
typedef struct { int32_t w; int32_t h; } muriscv_nn_tile;
typedef struct { int32_t min; int32_t max; } muriscv_nn_activation;
typedef struct { void *buf; int32_t size; } muriscv_nn_context;
typedef struct { int32_t n; int32_t h; int32_t w; int32_t c; } muriscv_nn_dims;
typedef struct
{
    muriscv_nn_tile stride;
    muriscv_nn_tile padding;
    muriscv_nn_activation activation;
} muriscv_nn_pool_params;

typedef int8_t q7_t;
typedef enum { MURISCV_NN_SUCCESS = 0, MURISCV_NN_ARG_ERROR = -1 } muriscv_nn_status;

muriscv_nn_status muriscv_nn_max_pool_s8(const muriscv_nn_context *ctx,
                                         const muriscv_nn_pool_params *pool_params,
                                         const muriscv_nn_dims *input_dims,
                                         const q7_t *src,
                                         const muriscv_nn_dims *filter_dims,
                                         const muriscv_nn_dims *output_dims,
                                         q7_t *dst);

#endif
