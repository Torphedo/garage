# Generate test driver with a list of all source files / functions.
# CommonCTests is an auto-generated test runner that runs functions with the
# same name as the .c filename of each test source file.
create_test_sourcelist(tests
  "CommonCTests.c" # Test driver file that makes up the main() function and takes arguments to run unit tests
  "test_struct.c"
)

# Single executable that contains all unit tests
add_executable(CommonCTests ${tests} "../src/common/logging.c")
target_include_directories(CommonCTests PRIVATE "../src" "../ext/cglm/include")

# Don't include the test driver (CommonCTests.c) as a unit test
remove(tests "CommonCTests.c")

# Add every source file as its own unit test
foreach(test ${tests})
    get_filename_component(TName ${test} NAME_WE)
    add_test(NAME ${TName} COMMAND CommonCTests ${TName})
endforeach()

