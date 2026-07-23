cmake_minimum_required(VERSION 3.14)
project(navigator_cpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# navigator_cpp is built as a shared library so its private dependency on
# libgpiod travels with it (DT_NEEDED). The OBJECT libraries linked into it
# must therefore be position-independent.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

option(BUILD_UNIT_TESTS "Build unit test binaries" OFF)
option(BUILD_EXAMPLES   "Build example binaries"   OFF)

# Library (always built)
add_subdirectory(src)

# Unit tests (opt-in)
if(BUILD_UNIT_TESTS)
    add_subdirectory(src/04_unit_tests)
endif()

# Examples (opt-in)
if(BUILD_EXAMPLES)
    add_subdirectory(src/05_examples)
endif()

# Always built
add_subdirectory(src/06_navigator_test)
add_subdirectory(src/07_test_rc)

# Install
install(TARGETS navigator_cpp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES include/nav_bindings.h
    DESTINATION include
)
