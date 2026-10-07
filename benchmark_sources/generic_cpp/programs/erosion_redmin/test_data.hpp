/*
*  Header file containing test data.
*/
#ifndef TEST_DATA_HPP
#define TEST_DATA_HPP

#include <cstdint>

/*
* struct containing metadata for the test: one grayscale erosion (uint8, 1 channel).
*/
struct test_metadata{
  int32_t height, width;  // image size (output has the same size)
  int32_t k;              // square structuring element k x k, odd, centred
};

#define NUM_TESTS 1 //Due to space concerns, each test is compiled separately

extern const void* input_array[];
extern const void* reference_array[];
extern const void* meta_array[];

#endif
