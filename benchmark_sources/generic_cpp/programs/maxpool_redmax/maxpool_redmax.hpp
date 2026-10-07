#ifndef MAXPOOL_REDMAX_HPP
#define MAXPOOL_REDMAX_HPP

// Same types and signature as the muRISCV-NN port, so benchmark.hpp and the test data are shared.
#include "../maxpool_muriscv/maxpool_muriscv.hpp"

// Max pooling with one vredmax.vs per window row and channel. Window rows longer than 16 bytes
// (VLMAX at e8,m1 with VLEN 128) are not supported and return MURISCV_NN_ARG_ERROR.
muriscv_nn_status maxpool_redmax_s8(const muriscv_nn_context *ctx,
                                    const muriscv_nn_pool_params *pool_params,
                                    const muriscv_nn_dims *input_dims,
                                    const q7_t *src,
                                    const muriscv_nn_dims *filter_dims,
                                    const muriscv_nn_dims *output_dims,
                                    q7_t *dst);

#endif
