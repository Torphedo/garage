# List of all test files
set(tests
    test_struct_size.c
)

# Add every source file as its own unit test
foreach(test_src ${tests})
    get_filename_component(test ${test_src} NAME_WE)

    # Compile an executable for each test source file
    add_executable(${test} ${test_src} "../src/common/logging.c")
    target_include_directories(${test} PRIVATE "../src" "../ext/cglm/include")

    add_test(NAME ${test} COMMAND ${test})
endforeach()

