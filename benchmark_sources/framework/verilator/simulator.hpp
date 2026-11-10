#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include "uart.hpp"
#include <cstdint>

extern "C" {
void __attribute__((noinline)) address_match_start() { volatile int a = 1; }
void __attribute__((noinline)) address_match_end() { volatile int a = 1; }
}

class Simulator {
private:
  // Stats variables
  uint32_t start_cycles, start_instr;
  uint32_t end_cycles, end_instr;
  uint32_t start_busy_redminmax, start_busy_alu;

  // Memory mapped address to terminate execution
  volatile int *const terminate_addr = (int *)0x00000408;

public:
  Simulator(){};

  // Function to start measurement
  // Verilator Simulation uses simulated CSRs
  inline void begin_measurement() {
    // Busy counters (0xC10), immediate selects the unit: 10 = REDMINMAX, 1 = ALU
    asm volatile("csrrsi %0, 0xC10, 10" : "=r"(start_busy_redminmax));
    asm volatile("csrrsi %0, 0xC10, 1" : "=r"(start_busy_alu));
    asm volatile("csrr %0,mcycle" : "=r"(start_cycles));
    asm volatile("csrr %0,minstret" : "=r"(start_instr));
    address_match_start();
  };

  // Function to end measurement
  // Verilator Simulation uses simulated CSRs
  inline void end_measurement() {
    address_match_end();
    asm volatile("csrr %0,mcycle" : "=r"(end_cycles));
    asm volatile("csrr %0,minstret" : "=r"(end_instr));
    uart_printf("Total Cycles:       %d\n", end_cycles - start_cycles);
    uart_printf("Total Instructions: %d\n\n", end_instr - start_instr);

    // Read after the prints: a read right after the kernel overtakes the last reduction
    uint32_t end_busy_redminmax, end_busy_alu;
    asm volatile("csrrsi %0, 0xC10, 10" : "=r"(end_busy_redminmax));
    asm volatile("csrrsi %0, 0xC10, 1" : "=r"(end_busy_alu));
    uart_printf("Busy REDMINMAX:     %u\n", end_busy_redminmax - start_busy_redminmax);
    uart_printf("Busy ALU:           %u\n\n", end_busy_alu - start_busy_alu);
  };

  // Termination success or failure function for this simulator
  void terminate(int code) // TODO: Int return codes
  {
    if (code == 0) {
      *terminate_addr = 0x0; // Write 0x0 to signal success
    } else {
      *terminate_addr = 0xf; // Write 0xF to signal success
    }
  };

  // Cleanup any allocatations
  ~Simulator(){};
};
#endif
