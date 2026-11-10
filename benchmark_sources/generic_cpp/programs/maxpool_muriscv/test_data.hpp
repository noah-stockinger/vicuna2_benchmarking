/*
*  Header file containing test data.
*/
#ifndef TEST_DATA_HPP
#define TEST_DATA_HPP

#include <cstdint>

/*
* struct containing metadata for the test: one TFLite max-pooling layer (NHWC, int8).
*/
struct test_metadata{
  int32_t batch;
  int32_t in_h, in_w, channels;
  int32_t filter_h, filter_w;
  int32_t stride_h, stride_w;
  int32_t pad_h, pad_w;
  int32_t out_h, out_w;
  int32_t act_min, act_max;
};

#define NUM_TESTS 1 //Due to space concerns, each test is compiled separately

extern const void* input_array[];
extern const void* reference_array[];
extern const void* meta_array[];

#endif
