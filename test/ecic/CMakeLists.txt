
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ED247A_ECIC.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ED247A_ECIC.xsd"
)
add_executable(Ecic_test
    Ecic_test.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.cpp"
)
target_require_cpp11(Ecic_test)
target_link_libraries(Ecic_test PRIVATE mingtest::gtest mingtest::gtest_main)
target_include_directories(Ecic_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME Ecic_test COMMAND Ecic_test)
