
set(DISABLE_ADD_SUBDIRECTORY_TEST true)
macro(add_subdirectory)
    if(NOT DISABLE_ADD_SUBDIRECTORY_TEST OR NOT ("${ARGV0}" STREQUAL "test" OR "${ARGV0}" STREQUAL "tests"))
        _add_subdirectory(${ARGV})
    endif()
endmacro()

set(DISABLE_INCLUDE_CPACK true)
macro(include)
    if(NOT DISABLE_INCLUDE_CPACK OR NOT ("${ARGV0}" STREQUAL "CPack" OR "${ARGV0}" STREQUAL "CDeploy"))
        _include(${ARGV})
    endif()
endmacro()

set(DISABLE_INSTALL true)
macro(install)
    if(NOT DISABLE_INSTALL)
        _install(${ARGV})
    endif()
endmacro()

set(DISABLE_INSTALL_DEPLOY_EXPORT true)
macro(install_deploy_export)
    if(NOT DISABLE_INSTALL_DEPLOY_EXPORT)
        _install_deploy_export(${ARGV})
    endif()
endmacro()

add_subdirectory(libnstd)
add_subdirectory(mingtest)

set_target_properties(nstd nstdCrypto nstdDocument nstdSocket gtest gtest_main
    PROPERTIES
        FOLDER   "3rdparty"
)
