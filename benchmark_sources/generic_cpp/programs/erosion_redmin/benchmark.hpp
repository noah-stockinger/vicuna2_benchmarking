#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <cstdint>
//BSP includes
#include "uart.hpp"
//Includes for benchmark
#include "test_data.hpp"
#include "erosion_redmin.hpp"

class Benchmark
{
    private:
    /*
    * Private Helper Functions and variables
    */
    const uint8_t* input;
    uint8_t* output;
    test_metadata meta;
    uint32_t len;

    void report_metadata()
    {
        uart_printf("Testcase:\n\n");
        uart_printf("Erosion u8 (vredminu): H=%d W=%d k=%d\n\n", meta.height, meta.width, meta.k);
    }

    public:
    /*
    * Required Functions for test framework
    */
    //override constructor to initialize any variables needed for the benchmark
    Benchmark(){
        //load inputs from test data -- only set up for 1 test case per file
        input = (const uint8_t*)input_array[0];
        meta = *(test_metadata*)meta_array[0];
        len = meta.height * meta.width;
        output = (uint8_t*)vicuna_malloc(len);
    };

    //Call code to be benchmarked
    inline int run_benchmark()
    {
        return erosion_redmin_u8(input, output, meta.height, meta.width, meta.k);
    };
    //Validate Output
    int validate_benchmark()
    {
        report_metadata();
        const uint8_t* reference = (const uint8_t*)reference_array[0];
        for(uint32_t i=0; i<len; i++)
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
