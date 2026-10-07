/*
*  Header file containing test data.
*/
#ifndef TEST_DATA_HPP
#define TEST_DATA_HPP

#include <cstdint>

/*
* struct containing metadata for the test.
*/
struct test_metadata{
  uint32_t op;        // redminmax_op
  uint32_t sew;       // 8, 16, 32
  uint32_t lmul;      // 1, 2, 4, 8
  uint32_t vl;        // elements reduced
  uint32_t reps;      // dependent reductions in the timed loop (multiple of 8)
  int32_t  seed;      // vs1[0] before the first reduction
  int32_t  reference; // expected v1[0] after the loop (low SEW bits are compared)
};

#define NUM_TESTS 1 //Due to space concerns, each test is compiled separately

extern const void* data_array[];
extern const void* meta_array[];

#endif
