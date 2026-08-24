cmake_minimum_required(VERSION 3.14)
project(navigator_auv_validation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NAVIGATOR_CPP_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." CACHE PATH
    "Path to the navigator-cpp source tree")

find_library(NAVIGATOR_CPP_LIBRARY
    NAMES navigator_cpp
    HINTS "${NAVIGATOR_CPP_ROOT}/lib"
    NO_DEFAULT_PATH)

if(NOT NAVIGATOR_CPP_LIBRARY)
    message(FATAL_ERROR
        "navigator_cpp was not found in ${NAVIGATOR_CPP_ROOT}/lib. "
        "Build navigator-cpp first with ./build.sh --release")
endif()

add_executable(pwm_cycle pwm_cycle.cpp)
target_include_directories(pwm_cycle PRIVATE "${NAVIGATOR_CPP_ROOT}/include")
target_link_libraries(pwm_cycle PRIVATE "${NAVIGATOR_CPP_LIBRARY}")

set_target_properties(pwm_cycle PROPERTIES
    BUILD_RPATH "${NAVIGATOR_CPP_ROOT}/lib")
