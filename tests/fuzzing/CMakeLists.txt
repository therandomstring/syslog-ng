#BEGIN FUNCTIONS

function(configure_fuzz_target_flags TARGET)
    cmake_parse_arguments(FUZZ "" "TARGET" "")
    set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-O1 -fsanitize=\"address,fuzzer\" -fno-omit-frame-pointer")
    set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-O1, -fsanitize=\"address,fuzzer\" -fno-omit-frame-pointer")
endfunction()

function(configure_fuzz_target_libs TARGET LIBRARIES)
    cmake_parse_arguments(FUZZ "" "TARGET" "LIBRARIES")
    target_link_libraries(${TARGET} PRIVATE syslog-ng ${LIBRARIES})
endfunction()

#END FUNCTIONS

option(ENABLE_FUZZING "Enable fuzz testing" "OFF")
if(NOT ENABLE_FUZZING)
    return()
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
    message(WARNING "Apple Clang currently not supported")
    return()
endif()

if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    message(WARNING "This function is only available when compiling with Clang. Please make sure you are indeed using Clang if you want to fuzz syslog-ng.")
    return()
endif()

#Required by Clang
find_package(LLVM)

add_subdirectory(tests)