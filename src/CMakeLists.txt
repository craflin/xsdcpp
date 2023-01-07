
add_library(libxsdcpp OBJECT
    Xsd.cpp
    Xsd.hpp
    Generator.cpp
    Generator.hpp
)
target_link_libraries(libxsdcpp PUBLIC
    libnstd::Document
)
target_compile_features(libxsdcpp PUBLIC
    cxx_override
)

add_executable(xsdcpp
    Main.cpp
)

target_compile_definitions(xsdcpp PRIVATE
     "VERSION=\"${PROJECT_VERSION}\""
)

target_link_libraries(xsdcpp PRIVATE
    libxsdcpp
)
