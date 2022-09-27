add_executable(fuzz_example targets/example1.c)

configure_fuzz_target_flags(fuzz_example)
configure_fuzz_target_libs(fuzz_example "")
