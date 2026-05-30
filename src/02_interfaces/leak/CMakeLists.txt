add_library(nav_leak OBJECT leak.cpp)
target_include_directories(nav_leak PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(nav_leak PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(nav_leak PRIVATE ${CMAKE_SOURCE_DIR}/src/02_interfaces/gpio)
