
add_executable(xsdcpp_parse_test xsdcpp_parse_test.cpp)
target_link_libraries(xsdcpp_parse_test PRIVATE libxsdcpp)
target_compile_definitions(xsdcpp_parse_test PRIVATE "FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
add_test(NAME xsdcpp_parse_test COMMAND xsdcpp_parse_test)

add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ED247A_ECIC.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.cpp"
    DEPENDS xsdcpp
)
add_executable(xsdcpp_build_test
    xsdcpp_build_test.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.cpp"
)
target_include_directories(xsdcpp_build_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME xsdcpp_build_test COMMAND xsdcpp_build_test)

add_executable(xsdcpp_XmlParser_test xsdcpp_XmlParser_test.cpp)
add_test(NAME xsdcpp_XmlParser_test COMMAND xsdcpp_XmlParser_test)