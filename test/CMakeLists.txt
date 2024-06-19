
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall")
endif()

function(target_require_cpp11 target)

    list(FIND CMAKE_CXX_COMPILE_FEATURES cxx_std_11 feature_index)
    if(NOT feature_index EQUAL -1)
        target_compile_features(${target} PUBLIC cxx_std_11)
    else()
        list(FIND CMAKE_CXX_COMPILE_FEATURES cxx_rvalue_references feature_index)
        if(NOT feature_index EQUAL -1)
            target_compile_features(${target} PUBLIC cxx_rvalue_references)
        endif()
    endif()

endfunction()

add_executable(Parser_test Parser_test.cpp)
target_link_libraries(Parser_test PRIVATE mingtest::gtest mingtest::gtest_main)
add_test(NAME Parser_test COMMAND Parser_test)
target_require_cpp11(Parser_test)

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
add_executable(Ecic_test
    Ecic_test.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/ED247A_ECIC.cpp"
)
target_require_cpp11(Ecic_test)
target_link_libraries(Ecic_test PRIVATE mingtest::gtest mingtest::gtest_main)
target_include_directories(Ecic_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME Ecic_test COMMAND Ecic_test)

add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-types-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_types_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_types_2_0.cpp"
    DEPENDS xsdcpp
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-interface-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_interface_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_interface_2_0.cpp"
    DEPENDS xsdcpp
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/sca/sca-1.1-cd06-subset-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sca_1_1_cd06_subset_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/sca_1_1_cd06_subset_2_0.cpp"
    DEPENDS xsdcpp
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-implementation-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_implementation_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_implementation_2_0.cpp"
    DEPENDS xsdcpp
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-deployment-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_deployment_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_deployment_2_0.cpp"
    DEPENDS xsdcpp
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-logicalsystem-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_logicalsystem_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_logicalsystem_2_0.cpp"
    DEPENDS xsdcpp
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/ecoa-project-2.0.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ecoa_project_2_0.hpp" "${CMAKE_CURRENT_BINARY_DIR}/ecoa_project_2_0.cpp"
    DEPENDS xsdcpp
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

set_target_properties(Parser_test Ecic_test Ecoa_test Generator_test Reader_test
    PROPERTIES
        FOLDER   "test"
)
