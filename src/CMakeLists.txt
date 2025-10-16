
add_subdirectory(ResourceCompiler)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Resources.hpp"
    COMMAND ResourceCompiler "${CMAKE_CURRENT_SOURCE_DIR}/xsd.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/XmlParser.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/XmlParser.hpp" -o "${CMAKE_CURRENT_BINARY_DIR}/Resources.hpp"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/xsd.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/XmlParser.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/XmlParser.hpp"  "$<TARGET_FILE:ResourceCompiler>"
)

add_library(libxsdcpp STATIC
    Reader.cpp
    Reader.hpp
    Generator.cpp
    Generator.hpp
    "${CMAKE_CURRENT_BINARY_DIR}/Resources.hpp"
)
target_compile_features(libxsdcpp PUBLIC cxx_override)
target_link_libraries(libxsdcpp PUBLIC libnstd::Document)
target_include_directories(libxsdcpp PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(xsdcpp 
    Main.cpp
)
target_link_libraries(xsdcpp PUBLIC libxsdcpp)
target_compile_definitions(xsdcpp PRIVATE "VERSION=\"${PROJECT_VERSION}\"")

install(TARGETS xsdcpp
    DESTINATION .
)

set_target_properties(xsdcpp libxsdcpp
    PROPERTIES
        FOLDER   "src"
)
