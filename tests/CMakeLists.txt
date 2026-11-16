# Include FetchContent module
include(FetchContent)

# Declare GoogleTest dependency from its official GitHub repository
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest GIT_TAG main)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Download and make the GTest targets available
FetchContent_MakeAvailable(googletest)


add_executable(unit_tests test_main.cpp)
set_target_properties(unit_tests PROPERTIES COMPILE_WARNING_AS_ERROR ON)
target_link_libraries(unit_tests PRIVATE bprs::bprs)
target_link_libraries(unit_tests PRIVATE GTest::gtest_main)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang") # Matches both AppleClang and Clang
        target_compile_options(unit_tests PRIVATE -fprofile-instr-generate -fcoverage-mapping -O0 -g)
        target_link_options(unit_tests PRIVATE -fprofile-instr-generate)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(unit_tests PRIVATE --coverage -O0 -g)
        target_link_options(unit_tests PRIVATE --coverage)
    else()
        message(WARNING "Code coverage is only configured for GCC and Clang.")
    endif()
endif()
add_test(NAME RunUnitTests COMMAND unit_tests)