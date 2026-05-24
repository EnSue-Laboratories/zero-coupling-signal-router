cmake_minimum_required(VERSION 3.16)
project(zcsr C)

set(CMAKE_C_STANDARD 11)          # C11/C17 — no C++
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

# --- Shared interface contracts: the ONLY include surface modules may use ---
add_library(zcsr_contracts INTERFACE)
target_include_directories(zcsr_contracts INTERFACE ${CMAKE_SOURCE_DIR}/include)

# --- Each module compiles independently, seeing ONLY the contracts (zero-coupling) ---
function(zcsr_module name)
  file(GLOB SRC CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/modules/${name}/*.c)
  add_library(zcsr_${name} STATIC ${SRC})
  target_link_libraries(zcsr_${name} PUBLIC zcsr_contracts)
endfunction()

zcsr_module(core)      # Agent 1
zcsr_module(signal)    # Agent 2
zcsr_module(hsm)       # Agent 3
zcsr_module(overlay)   # Agent 4
zcsr_module(platform)  # Agent 5

# --- Agent 6: Integration & Validation (architecture/review owned) ---
add_executable(zcsr_demo  integration/demo.c)
add_executable(zcsr_bench integration/bench.c)
foreach(exe zcsr_demo zcsr_bench)
  target_link_libraries(${exe} PRIVATE
    zcsr_core zcsr_signal zcsr_hsm zcsr_overlay zcsr_platform)
endforeach()

# --- Tests ---
enable_testing()
add_executable(zcsr_test_signal tests/test_signal.c)
target_link_libraries(zcsr_test_signal PRIVATE zcsr_signal zcsr_contracts)
add_test(NAME signal_router COMMAND zcsr_test_signal)

# --- Zero-coupling guard: fail if a module includes another module ---
add_custom_target(check_coupling
  COMMAND ${CMAKE_SOURCE_DIR}/tools/check_no_cross_includes.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Verifying no cross-module includes")
