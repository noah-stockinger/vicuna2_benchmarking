#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <cstdint>
//BSP includes
#include "uart.hpp"
//Includes for benchmark
#include "test_data.hpp"
#include "redminmax_micro.hpp"

class Benchmark
{
    private:
    /*
    * Private Helper Functions and variables
    */
    const void* data;
    test_metadata meta;
    int32_t result;

    uint32_t sew_mask()
    {
        return (meta.sew == 32) ? 0xffffffffu : ((1u << meta.sew) - 1);
    }

    void report_metadata()
    {
        static const char* names[] = {"vredmax", "vredmaxu", "vredmin", "vredminu"};
        uart_printf("Testcase:\n\n");
        uart_printf("%s.vs SEW=%u LMUL=%u vl=%u reps=%u\n\n", names[meta.op], meta.sew, meta.lmul, meta.vl, meta.reps);
    }

    public:
    /*
    * Required Functions for test framework
    */
    //override constructor to initialize any variables needed for the benchmark
    Benchmark(){
        //load inputs from test data -- only set up for 1 test case per file
        data = data_array[0];
        meta = *(test_metadata*)meta_array[0];
        result = 0;
    };

    //Call code to be benchmarked
    inline int run_benchmark()
    {
        result = redminmax_micro(meta.op, meta.sew, meta.lmul, meta.vl, data, meta.seed, meta.reps);
        return 0; //always success
    };
    //Validate Output
    int validate_benchmark()
    {
        report_metadata();
        uint32_t got      = (uint32_t)result & sew_mask();
        uint32_t expected = (uint32_t)meta.reference & sew_mask();
        if (got != expected) {
            uart_printf("ERROR: OUTPUT MISMATCH\n");
            uart_printf("Your Result:      0x%x\n", got);
            uart_printf("Reference Result: 0x%x\n", expected);
            return 1;
        }
        return 0;
    };
    //Cleanup any allocatations
    ~Benchmark(){};
};

#endif
