find_package(Clang)

module_switch(ENABLE_FUZZING "Enable fuzz testing" Clang_FOUND)
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