cmake_minimum_required(VERSION 3.5)

project(xsdcpp VERSION 0.5.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".cmake")

if(MSVC AND NOT CMAKE_TOOLCHAIN_FILE)
    string(REPLACE "/MD" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/MD" "" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
    string(REPLACE "/MD" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT /Os /Oy /O1 /GF /GL")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "/OPT:REF /OPT:ICF /INCREMENTAL:NO /LTCG")
    set(CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL "/LTCG")
endif()

set(CDEPLOY_NO_DEBUG_BUILD True)
set(CDEPLOY_NO_COMPILER True)

include(CDeploy)
include(CPack)

enable_testing()

add_subdirectory(3rdparty)
add_subdirectory(src)
add_subdirectory(test)

install(FILES NOTICE LICENSE DESTINATION .)

if(WIN32)
    deploy_export(xsdcpp EXECUTABLE
        CONFIGURATION Release
        IMPORTED_LOCATION xsdcpp.exe
    )
    deploy_export(xsdcpp EXECUTABLE
        CONFIGURATION Debug
        IMPORTED_LOCATION xsdcpp.exe
    )
else()
    deploy_export(xsdcpp EXECUTABLE
        IMPORTED_LOCATION xsdcpp
    )
endif()

install_deploy_export()
