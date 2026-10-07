#include "redminmax_micro.hpp"

/*
 * Each (load, op) pair is one asm block, so the compiler cannot place its own vector code
 * between vsetvl and the reductions. vtype comes from a register (vsetvl), so one block
 * serves every LMUL; only the load (element width) and the reduction differ.
 *
 * vmv.x.s at the end writes a scalar register, so the scalar core has to wait for the last
 * reduction before end_measurement() reads mcycle.
 */
#define RED8(OP) OP "\n" OP "\n" OP "\n" OP "\n" OP "\n" OP "\n" OP "\n" OP "\n"

#define KERNEL(LOAD, OP)                                                        \
  asm volatile(                                                                 \
      "vsetvl  t0, %[vl], %[vtype]\n"                                           \
      LOAD "   v8, (%[src])\n"                                                  \
      "vmv.s.x v1, %[seed]\n"                                                   \
      "mv      t1, %[iters]\n"                                                  \
      "1:\n"                                                                    \
      RED8(OP " v1, v8, v1")                                                    \
      "addi    t1, t1, -1\n"                                                    \
      "bnez    t1, 1b\n"                                                        \
      "vmv.x.s %[res], v1\n"                                                    \
      : [res] "=&r"(res)                                                        \
      : [vl] "r"(vl), [vtype] "r"(vtype), [src] "r"(src), [seed] "r"(seed),    \
        [iters] "r"(iters)                                                      \
      : "t0", "t1", "v1", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", \
        "memory")

#define KERNEL_OPS(LOAD)                                                        \
  switch (op) {                                                                 \
    case OP_VREDMAX:  KERNEL(LOAD, "vredmax.vs");  break;                       \
    case OP_VREDMAXU: KERNEL(LOAD, "vredmaxu.vs"); break;                       \
    case OP_VREDMIN:  KERNEL(LOAD, "vredmin.vs");  break;                       \
    case OP_VREDMINU: KERNEL(LOAD, "vredminu.vs"); break;                       \
  }

int32_t redminmax_micro(uint32_t op, uint32_t sew, uint32_t lmul, uint32_t vl,
                        const void * src, int32_t seed, uint32_t reps)
{
  // vtype = vma | vta | vsew | vlmul
  uint32_t vsew  = (sew == 8) ? 0 : (sew == 16) ? 1 : 2;
  uint32_t vlmul = (lmul == 1) ? 0 : (lmul == 2) ? 1 : (lmul == 4) ? 2 : 3;
  uint32_t vtype = (1u << 7) | (1u << 6) | (vsew << 3) | vlmul;
  uint32_t iters = reps / 8;
  int32_t  res   = 0;

  switch (sew) {
    case 8:  KERNEL_OPS("vle8.v");  break;
    case 16: KERNEL_OPS("vle16.v"); break;
    case 32: KERNEL_OPS("vle32.v"); break;
  }
  return res;
}
