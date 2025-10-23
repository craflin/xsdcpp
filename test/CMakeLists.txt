
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

add_executable(XmlParser_test XmlParser_test.cpp)
target_link_libraries(XmlParser_test PRIVATE mingtest::gtest mingtest::gtest_main)
add_test(NAME XmlParser_test COMMAND XmlParser_test)
target_require_cpp11(XmlParser_test)

add_executable(Reader_test Reader_test.cpp)
target_link_libraries(Reader_test PRIVATE mingtest::gtest mingtest::gtest_main libxsdcpp)
target_compile_definitions(Reader_test PRIVATE "FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
add_test(NAME Reader_test COMMAND Reader_test)

add_executable(Generator_test Generator_test.cpp)
target_link_libraries(Generator_test PRIVATE mingtest::gtest mingtest::gtest_main libxsdcpp)
target_compile_definitions(Generator_test PRIVATE "FOLDER=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
add_test(NAME Generator_test COMMAND Generator_test)

add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/SubstitutionGroup.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/SubstitutionGroup.hpp" "${CMAKE_CURRENT_BINARY_DIR}/SubstitutionGroup.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/SubstitutionGroup.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Choice.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}" -e xsdcpp
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Choice.hpp" "${CMAKE_CURRENT_BINARY_DIR}/Choice.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Choice.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Recursion.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}" -e xsdcpp
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Recursion.hpp" "${CMAKE_CURRENT_BINARY_DIR}/Recursion.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Recursion.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/SimpleTypeExtension.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}" -e xsdcpp -t Version
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/SimpleTypeExtension.hpp" "${CMAKE_CURRENT_BINARY_DIR}/SimpleTypeExtension.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/SimpleTypeExtension.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Import.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}" -e SimpleTypeExtension
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Import.hpp" "${CMAKE_CURRENT_BINARY_DIR}/Import.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Import.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Attributes.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}" -e xsdcpp
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Attributes.hpp" "${CMAKE_CURRENT_BINARY_DIR}/Attributes.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Attributes.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Occurrence.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}" -e xsdcpp
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Occurrence.hpp" "${CMAKE_CURRENT_BINARY_DIR}/Occurrence.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Occurrence.xsd"
)
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Example.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}" -e xsdcpp
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Example.hpp" "${CMAKE_CURRENT_BINARY_DIR}/Example.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/Example.xsd"
)
add_executable(Features_test
    Features_test.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/SubstitutionGroup.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/SubstitutionGroup.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Choice.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Choice.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Recursion.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Recursion.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/SimpleTypeExtension.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/SimpleTypeExtension.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Import.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Import.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Attributes.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Attributes.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Occurrence.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Occurrence.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Example.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/Example.cpp"
)
target_require_cpp11(Features_test)
target_link_libraries(Features_test PRIVATE mingtest::gtest mingtest::gtest_main)
target_include_directories(Features_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME Features_test COMMAND Features_test)

add_executable(XsdLib_test XsdLib_test.cpp)
target_link_libraries(XsdLib_test PRIVATE mingtest::gtest mingtest::gtest_main)
add_test(NAME XsdLib_test COMMAND XsdLib_test)
target_require_cpp11(XsdLib_test)

add_subdirectory(ecic)
add_subdirectory(ecoa)

set_target_properties(XmlParser_test Ecic_test Ecoa_test Generator_test Reader_test Features_test XsdLib_test
    PROPERTIES
        FOLDER   "test"
)
