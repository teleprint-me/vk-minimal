cmake_minimum_required(VERSION 3.17)
project("vkc" LANGUAGES C)

set(CMAKE_C_STANDARD 17)

# Add sanitizers for memory safety
# Ref: https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
# Ref: https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc

# Common warning flags
set(COMMON_WARNING_FLAGS "-Wall -Wextra -Wpedantic -Werror -Wformat-security -Wshadow -fexceptions")

# Additional Debug-only flags (sanitizers and memory safety checks)
set(DEBUG_SANITIZERS "-fsanitize=address,undefined -fno-omit-frame-pointer")
set(DEBUG_EXTRA_WARNINGS "-Wformat -Wnull-dereference -Wdouble-promotion")

# Static analysis flags for catching common security issues
set(DEBUG_ANALYSIS "-Wanalyzer-double-free -Wanalyzer-file-leak -Wanalyzer-malloc-leak -Wanalyzer-null-dereference -Wanalyzer-out-of-bounds -Wanalyzer-va-list-leak")

# Enable debugging level 3 for macros
set(DEBUG_FLAGS "-g3")

# Apply flags conditionally based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_WARNING_FLAGS} ${DEBUG_SANITIZERS} ${DEBUG_EXTRA_WARNINGS} ${DEBUG_ANALYSIS} ${DEBUG_FLAGS}")
    add_compile_definitions(-DVKC_DEBUG=1)
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_WARNING_FLAGS}")
    add_compile_definitions(-DVKC_DEBUG=0)
endif()

# Enable Shared Libraries option
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# Add library
add_library(vkc SHARED
    "src/vk/allocator.c"
    "src/vk/instance.c"
    "src/vk/device.c"
    "src/vk/shader.c"
)
target_include_directories("vkc" PUBLIC include dsa/include)
target_link_libraries("vkc" PUBLIC m rt pthread vulkan dsa)

enable_testing()
add_subdirectory(dsa)
add_subdirectory(examples)
# add_subdirectory(tests)

# Custom doc target for generating docs
add_custom_target(doc
    COMMAND doxygen ${CMAKE_SOURCE_DIR}/doxy.conf
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
)

# Custom clean target for removing generated files
# add_custom_target(clean_all
#     COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib
#     COMMENT "Cleaning all generated files"
# )
