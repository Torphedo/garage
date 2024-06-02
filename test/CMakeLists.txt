# List of all test files
set(test_sources
    test_struct_size.c
    test_stfs.c
)

add_executable(test
    ../src/common/logging.c
    ../src/common/int.c
    ../src/common/sha1.c
    ../src/stfs.c
    ${test_sources}
    main.c
)
