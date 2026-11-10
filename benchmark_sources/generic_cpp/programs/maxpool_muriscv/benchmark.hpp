#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <cstdint>
//BSP includes
#include "uart.hpp"
//Includes for benchmark
#include "test_data.hpp"
#include "maxpool_muriscv.hpp"

class Benchmark
{
    private:
    /*
    * Private Helper Functions and variables
    */
    const int8_t* input;
    int8_t* output;
    test_metadata meta;
    uint32_t out_len;

    muriscv_nn_context ctx;
    muriscv_nn_pool_params pool_params;
    muriscv_nn_dims input_dims, filter_dims, output_dims;

    void report_metadata()
    {
        uart_printf("Testcase:\n\n");
        uart_printf("Max pool s8 (muRISCV-NN): N=%d H=%d W=%d C=%d filter %dx%d stride %dx%d pad %dx%d -> %dx%d\n\n",
                    meta.batch, meta.in_h, meta.in_w, meta.channels, meta.filter_h, meta.filter_w,
                    meta.stride_h, meta.stride_w, meta.pad_h, meta.pad_w, meta.out_h, meta.out_w);
    }

    public:
    /*
    * Required Functions for test framework
    */
    //override constructor to initialize any variables needed for the benchmark
    Benchmark(){
        //load inputs from test data -- only set up for 1 test case per file
        input = (const int8_t*)input_array[0];
        meta = *(test_metadata*)meta_array[0];
        out_len = meta.batch * meta.out_h * meta.out_w * meta.channels;
        output = (int8_t*)vicuna_malloc(out_len);

        //same parameter setup as muRISCV-NN Tests/TestCases/test_muriscv_nn_maxpool_s8
        input_dims.n = meta.batch;
        input_dims.h = meta.in_h;
        input_dims.w = meta.in_w;
        input_dims.c = meta.channels;
        filter_dims.h = meta.filter_h;
        filter_dims.w = meta.filter_w;
        output_dims.h = meta.out_h;
        output_dims.w = meta.out_w;
        output_dims.c = meta.channels;
        pool_params.stride.h = meta.stride_h;
        pool_params.stride.w = meta.stride_w;
        pool_params.padding.h = meta.pad_h;
        pool_params.padding.w = meta.pad_w;
        pool_params.activation.min = meta.act_min;
        pool_params.activation.max = meta.act_max;
    };

    //Call code to be benchmarked
    inline int run_benchmark()
    {
        return muriscv_nn_max_pool_s8(&ctx, &pool_params, &input_dims, input, &filter_dims, &output_dims, output);
    };
    //Validate Output
    int validate_benchmark()
    {
        report_metadata();
        const int8_t* reference = (const int8_t*)reference_array[0];
        for(uint32_t i=0; i<out_len; i++)
        {
            if (output[i] != reference[i])
            {
                uart_printf("ERROR: OUTPUT MISMATCH at index %d\n", i);
                uart_printf("Your Result:      %d\n", output[i]);
                uart_printf("Reference Result: %d\n", reference[i]);
                return 1;
            }
        }
        return 0;
    };
    //Cleanup any allocatations
    ~Benchmark(){};
};

#endif
