cmake_minimum_required(VERSION 3.1)

project(xsdcpp VERSION 0.1.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".cmake")

enable_testing()

add_subdirectory(3rdparty)
add_subdirectory(src)
add_subdirectory(test)
