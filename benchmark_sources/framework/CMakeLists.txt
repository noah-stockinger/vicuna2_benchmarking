add_subdirectory(vicuna2_bsp vicuna2_bsp)
set(VICUNA_BSP_TOP ${CMAKE_CURRENT_SOURCE_DIR}/vicuna2_bsp/ PARENT_SCOPE)

add_subdirectory(etiss)
set(ETISS_CRT_TOP ${CMAKE_CURRENT_SOURCE_DIR}/etiss/ PARENT_SCOPE)

set(FRAMEWORK_TOP ${CMAKE_CURRENT_SOURCE_DIR}/ PARENT_SCOPE)

#Add header library for Verilator only simulation specific implementations
add_library(sim_Verilator INTERFACE)

target_include_directories(sim_Verilator INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/verilator
)

#gem5/hybrid support needs sources from the gem5 submodule; only set it up when one of those simulators is enabled
if(GEM5_SIM OR HYBRID_GEM5_SIM)
#Add library for m5 support ops (called from inside simulated program to interact with gem5)
add_library(m5 STATIC)

target_sources(m5 PRIVATE
  ${GEM5_MODEL_DIR}/gem5/util/m5/src/m5.cc
  ${GEM5_MODEL_DIR}/gem5/util/m5/src/args.cc
  ${GEM5_MODEL_DIR}/gem5/util/m5/src/call_type.cc
  ${GEM5_MODEL_DIR}/gem5/util/m5/src/command.cc
  ${GEM5_MODEL_DIR}/gem5/util/m5/src/usage.cc
  ${GEM5_MODEL_DIR}/gem5/util/m5/src/abi/riscv/m5op.S
)
target_include_directories(m5 PUBLIC
  ${GEM5_MODEL_DIR}/gem5/util/m5/src
  ${GEM5_MODEL_DIR}/gem5/util/m5/src/abi/riscv
  ${GEM5_MODEL_DIR}/gem5/util/m5/src/abi/call_type
  ${GEM5_MODEL_DIR}/gem5/util/m5/src/abi/command
  ${GEM5_MODEL_DIR}/gem5/include
)

#Add header library for gem5 only simulation specific implementations.  Also add path to m5 support functions
add_library(sim_gem5 INTERFACE)

target_include_directories(sim_gem5 INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/gem5
  ${GEM5_MODEL_DIR}/gem5/include
)
target_link_libraries(sim_gem5 INTERFACE
  m5
)

#Add library for hybrid simulation specific implementations
add_library(sim_hybrid INTERFACE)

target_include_directories(sim_hybrid INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/hybrid
  ${GEM5_MODEL_DIR}/gem5/include
)
target_link_libraries(sim_hybrid INTERFACE
  m5
)
endif()


#Add library for spike simulation specific implementations
add_library(sim_spike INTERFACE)

target_include_directories(sim_spike INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/spike
)

#Add library for ETISS simulation specific implementations
add_library(sim_etiss INTERFACE)

target_include_directories(sim_etiss INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/etiss
)

