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

# --- Task #7: Classic 2D Game Core extension (zero-coupling, builds on the contracts above) ---
zcsr_module(input)     # Game Core Agent 1 — input / window / 60FPS timer
zcsr_module(audio)     # Game Core Agent 2 — WAV audio (signals only)
zcsr_module(render)    # Game Core Agent 3 — PNG (stb_image) + batch sprite renderer
zcsr_module(gamelogic) # Game Core Agent 4 — AABB collision + binary save/load
if(UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
  target_link_libraries(zcsr_platform PUBLIC ${X11_LIBRARIES})
  target_include_directories(zcsr_platform PRIVATE ${X11_INCLUDE_DIR})
  target_link_libraries(zcsr_overlay PUBLIC ${X11_LIBRARIES})
  target_include_directories(zcsr_overlay PRIVATE ${X11_INCLUDE_DIR})
  target_link_libraries(zcsr_input PUBLIC ${X11_LIBRARIES})  # input pumps X11 key/close events
  target_include_directories(zcsr_input PRIVATE ${X11_INCLUDE_DIR})
  target_link_libraries(zcsr_render PUBLIC ${X11_LIBRARIES})
  target_include_directories(zcsr_render PRIVATE ${X11_INCLUDE_DIR})
endif()

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

add_executable(test_core tests/test_core.c)
target_link_libraries(test_core PRIVATE zcsr_core)
add_test(NAME core COMMAND test_core)
add_executable(test_overlay tests/test_overlay.c)
target_link_libraries(test_overlay PRIVATE zcsr_overlay zcsr_platform)
add_test(NAME overlay COMMAND test_overlay)

add_executable(test_hsm tests/test_hsm.c)
target_link_libraries(test_hsm PRIVATE zcsr_hsm zcsr_core)  # hsm persists into the core state buffer
add_test(NAME hsm COMMAND test_hsm)

add_executable(test_state_reuse tests/test_state_reuse.c)
target_link_libraries(test_state_reuse PRIVATE zcsr_core)
add_test(NAME state_reuse COMMAND test_state_reuse)

add_executable(test_integration tests/test_integration.c)
target_link_libraries(test_integration PRIVATE zcsr_overlay zcsr_hsm zcsr_core zcsr_signal zcsr_platform)
add_test(NAME integration COMMAND test_integration)

add_executable(test_gamelogic tests/test_gamelogic.c)
target_link_libraries(test_gamelogic PRIVATE zcsr_gamelogic)
add_test(NAME gamelogic COMMAND test_gamelogic)

add_executable(test_input tests/test_input.c)
target_link_libraries(test_input PRIVATE zcsr_input zcsr_platform)  # input uses platform accessors
add_test(NAME input COMMAND test_input)

# --- Zero-coupling guard: fail if a module includes another module ---
add_custom_target(check_coupling
  COMMAND ${CMAKE_SOURCE_DIR}/tools/check_no_cross_includes.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Verifying no cross-module includes")
