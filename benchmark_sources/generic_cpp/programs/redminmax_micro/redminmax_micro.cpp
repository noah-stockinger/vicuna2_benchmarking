#include "redminmax_micro.hpp"

/*
 * One asm block per (load, op), so the compiler can't put vector code between vsetvl and the
 * reductions. vmv.x.s at the end makes the scalar core wait for the last reduction.
 *
 * Workarounds for core bugs at LMUL > 1 (e54481e):
 *  - accumulator v16, not v1: odd vs1/vd trap (vproc_decoder.sv:5111)
 *  - seed written at LMUL 1: vmv.s.x hangs the pipeline (vproc_decoder.sv:4638)
 */
#define RED8(OP) OP "\n" OP "\n" OP "\n" OP "\n" OP "\n" OP "\n" OP "\n" OP "\n"

#define KERNEL(LOAD, OP)                                                        \
  asm volatile(                                                                 \
      "vsetvl  t0, %[vl], %[vtype1]\n"                                          \
      "vmv.s.x v16, %[seed]\n"                                                  \
      "vsetvl  t0, %[vl], %[vtype]\n"                                           \
      LOAD "   v8, (%[src])\n"                                                  \
      "mv      t1, %[iters]\n"                                                  \
      "1:\n"                                                                    \
      RED8(OP " v16, v8, v16")                                                  \
      "addi    t1, t1, -1\n"                                                    \
      "bnez    t1, 1b\n"                                                        \
      "vmv.x.s %[res], v16\n"                                                   \
      : [res] "=&r"(res)                                                        \
      : [vl] "r"(vl), [vtype] "r"(vtype), [vtype1] "r"(vtype1),                 \
        [src] "r"(src), [seed] "r"(seed), [iters] "r"(iters)                    \
      : "t0", "t1", "v16", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", \
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
  uint32_t vtype  = (1u << 7) | (1u << 6) | (vsew << 3) | vlmul;
  uint32_t vtype1 = vtype & ~0x7u;  // same SEW and policy, LMUL 1 (for vmv.s.x, see above)
  uint32_t iters = reps / 8;
  int32_t  res   = 0;

  switch (sew) {
    case 8:  KERNEL_OPS("vle8.v");  break;
    case 16: KERNEL_OPS("vle16.v"); break;
    case 32: KERNEL_OPS("vle32.v"); break;
  }
  return res;
}
