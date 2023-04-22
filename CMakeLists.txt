cmake_minimum_required(VERSION 3.1)

project(xsdcpp VERSION 0.1.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".cmake")

set(CDEPLOY_NO_DEBUG_BUILD True)
set(CDEPLOY_NO_COMPILER True)

include(CDeploy)
include(CPack)

deploy_package(mingtest 0.1.9 "https://github.com/craflin/mingtest/releases/download/0.1.9")

enable_testing()

add_subdirectory(3rdparty)
add_subdirectory(src)
add_subdirectory(test)

install(FILES NOTICE LICENSE DESTINATION .)

if(WIN32)
    deploy_export(xsxcpp EXECUTABLE
        CONFIGURATION Release
        IMPORTED_LOCATION bin/xsxcpp.exe
    )
    deploy_export(xsxcpp EXECUTABLE
        CONFIGURATION Debug
        IMPORTED_LOCATION bin/xsxcpp.exe
    )
else()
    deploy_export(xsxcpp EXECUTABLE
        IMPORTED_LOCATION bin/xsxcpp
    )
endif()

include(InstallRequiredSystemLibraries)
install_deploy_export()
