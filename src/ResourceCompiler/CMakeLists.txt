
add_executable(ResourceCompiler Main.cpp)
target_link_libraries(ResourceCompiler PUBLIC libnstd::Core)


set_target_properties(ResourceCompiler
    PROPERTIES
        FOLDER   "src/ResourceCompiler"
)
