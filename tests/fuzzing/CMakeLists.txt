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
add_subdirectory(lib)
