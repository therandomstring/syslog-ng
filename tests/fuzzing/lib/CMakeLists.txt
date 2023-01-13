add_library(fuzzing_intercept_memory_functions SHARED fuzzing_malloc.c)

add_library(fuzzing_helpers SHARED fuzzing_helper.c fuzzing_helper.h)
target_include_directories(fuzzing_helpers PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(fuzzing_helpers PRIVATE syslog-ng syslogformat)
