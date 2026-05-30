##############################################################################
# navigator-cpp/src/CMakeLists.txt
#
# Build order matters: interfaces first, then sensors, then vendored,
# then the library that links them all.
##############################################################################

add_subdirectory(02_interfaces)
add_subdirectory(01_sensors)
add_subdirectory(03_vendored)
add_subdirectory(00_lib_nav)
