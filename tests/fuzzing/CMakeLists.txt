#BEGIN FUNCTIONS

#function(configure_fuzz_target_flags TARGET)
#    set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-O0, -fsanitize=\"address,fuzzer\" -fno-omit-frame-pointer")
#    set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-O0, -fsanitize=\"address,fuzzer\" -fno-omit-frame-pointer")
#endfunction()

#function(configure_fuzz_target_libs TARGET LIBRARIES)
#    target_link_libraries(${TARGET} PRIVATE syslog-ng ${LIBRARIES})
#endfunction()

#END FUNCTIONS

add_subdirectory(tests)

module_switch(ENABLE_FUZZING "Enable fuzz testing")
if(NOT ENABLE_FUZZING)
    return()
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
    message("Apple Clang currently not supported")
    return()
endif()

if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    message("This function is only available when compiling with Clang. Please make sure you are indeed using Clang if you want to fuzz syslog-ng.")
    return()
endif()

add_dependencies(Clang)