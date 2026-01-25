
add_custom_command(
    COMMAND "$<TARGET_FILE:xsdcpp>" "${CMAKE_CURRENT_SOURCE_DIR}/input.xsd" -o "${CMAKE_CURRENT_BINARY_DIR}"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/input.hpp" "${CMAKE_CURRENT_BINARY_DIR}/input.cpp"
    DEPENDS "$<TARGET_FILE:xsdcpp>"
        "${CMAKE_CURRENT_SOURCE_DIR}/input.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/ore_types.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/instruments.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/referencedata.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/simulation.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/creditsimulation.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/curveconfig.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/conventions.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/nettingsetdefinitions.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/pricingengines.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/todaysmarket.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/sensitivity.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/stress.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/ore.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/calendaradjustment.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/currencyconfig.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/iborfallbackconfig.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/baselTrafficLightconfig.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/scriptlibrary.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/simmcalibration.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/counterparty.xsd"
        "${CMAKE_CURRENT_SOURCE_DIR}/historicalreturnconfig.xsd"
)
add_executable(Ore_test
    Ore_test.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/input.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/input.cpp"
)
target_require_cpp11(Ore_test)
target_link_libraries(Ore_test PRIVATE mingtest::gtest mingtest::gtest_main)
target_include_directories(Ore_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME Ore_test COMMAND Ore_test)
