
add_library(libxsdcpp STATIC
    Xsd.cpp
    Xsd.hpp
    Generator.cpp
    Generator.hpp
)
target_compile_features(libxsdcpp PUBLIC cxx_override)
target_link_libraries(libxsdcpp PUBLIC libnstd::Document)

add_executable(xsdcpp 
    Main.cpp
)
target_link_libraries(xsdcpp PUBLIC libxsdcpp)
target_compile_definitions(xsdcpp PRIVATE "VERSION=\"${PROJECT_VERSION}\"")
