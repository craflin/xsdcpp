
add_executable(Reader_test Reader_test.cpp)
target_link_libraries(Reader_test PRIVATE mingtest::gtest mingtest::gtest_main libxsdcpp)
target_compile_definitions(Reader_test PRIVATE "FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
add_test(NAME Reader_test COMMAND Reader_test)

add_executable(Generator_test Generator_test.cpp)
target_link_libraries(Generator_test PRIVATE mingtest::gtest mingtest::gtest_main libxsdcpp)
target_compile_definitions(Generator_test PRIVATE "FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
add_test(NAME Generator_test COMMAND Generator_test)

add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ED247A_ECIC.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.cpp"
    DEPENDS xsdcpp
)
add_executable(Example_test
    Example_test.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.cpp"
)
target_link_libraries(Example_test PRIVATE mingtest::gtest mingtest::gtest_main)
target_include_directories(Example_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME Example_test COMMAND Example_test)

#add_executable(XmlParser_test XmlParser_test.cpp)
#target_link_libraries(XmlParser_test PRIVATE mingtest::gtest mingtest::gtest_main)
#add_test(NAME XmlParser_test COMMAND XmlParser_test)