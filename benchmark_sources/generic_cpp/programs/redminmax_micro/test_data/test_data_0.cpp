#include "test_data.hpp"
// Hand-written first configuration: vredmax.vs, SEW 8, LMUL 1, vl = VLMAX = 16, reps = 16.
// Maximum (97) at index 11; seed (-100) is below every element, so returning the seed fails.
alignas(4) const int8_t test_data[] = {
-12, 45, -88, 3, 77, -5, 60, -127, 21, 9, -40, 97, 14, -66, 50, 8
};

const test_metadata test_meta = {
.op = 0,
.sew = 8,
.lmul = 1,
.vl = 16,
.reps = 16,
.seed = -100,
.reference = 97
};

const void * data_array[] = {
(void*)test_data
};

const void * meta_array[] = {
(void*)&test_meta
};
