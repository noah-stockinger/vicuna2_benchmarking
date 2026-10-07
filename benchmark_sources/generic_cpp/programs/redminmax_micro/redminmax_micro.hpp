#ifndef REDMINMAX_MICRO_HPP
#define REDMINMAX_MICRO_HPP
#include <cstdint>

#include "vicuna_crt.hpp"
#include "uart.hpp"

// Operation codes used in test_metadata.op
enum redminmax_op : uint32_t { OP_VREDMAX = 0, OP_VREDMAXU = 1, OP_VREDMIN = 2, OP_VREDMINU = 3 };

// Runs `reps` dependent vred*.vs (vd = vs1 = v16, vs2 = v8 group) over `vl` elements of `src`,
// starting from `seed`, and returns the final v16[0] (sign-extended to 32 bit by vmv.x.s).
// reps must be a non-zero multiple of 8.
int32_t redminmax_micro(uint32_t op, uint32_t sew, uint32_t lmul, uint32_t vl,
                        const void * src, int32_t seed, uint32_t reps);

#endif
