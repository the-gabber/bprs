add_executable(example example.cpp)
set_target_properties(example PROPERTIES COMPILE_WARNING_AS_ERROR ON)
target_link_libraries(example PRIVATE bprs::bprs)