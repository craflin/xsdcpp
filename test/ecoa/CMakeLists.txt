
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-types-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_types_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_types_2_0.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-types-2.0.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-interface-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_interface_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_interface_2_0.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-interface-2.0.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/sca/sca-1.1-cd06-subset-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sca_1_1_cd06_subset_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/sca_1_1_cd06_subset_2_0.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/sca/sca-1.1-cd06-subset-2.0.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-implementation-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_implementation_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_implementation_2_0.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-implementation-2.0.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-deployment-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_deployment_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_deployment_2_0.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-deployment-2.0.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-logicalsystem-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_logicalsystem_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_logicalsystem_2_0.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-logicalsystem-2.0.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-project-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_project_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_project_2_0.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-project-2.0.xsd"
)


add_executable(Ecoa_test
    Ecoa_test.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_types_2_0.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_types_2_0.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_interface_2_0.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_interface_2_0.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/sca_1_1_cd06_subset_2_0.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/sca_1_1_cd06_subset_2_0.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_implementation_2_0.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_implementation_2_0.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_deployment_2_0.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_deployment_2_0.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_logicalsystem_2_0.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_logicalsystem_2_0.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_project_2_0.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ecoa_project_2_0.cpp"
)
target_require_cpp11(Ecoa_test)
target_link_libraries(Ecoa_test PRIVATE mingtest::gtest mingtest::gtest_main)
target_include_directories(Ecoa_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(Ecoa_test PRIVATE "FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
add_test(NAME Ecoa_test COMMAND Ecoa_test)
