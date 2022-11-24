# Fuzzing framework for syslog-ng

## Contents:
1. [What is fuzzing](#what-is-fuzzing)
2. [Used tools](#this-is-what-you-will-need)
   1. [LibFuzzer](#libfuzzerhttpsllvmorgdocslibfuzzerhtmlfaq)
   2. [Clang](#clang)
3. [How to fuzz](#how-to-fuzz-syslog-ng)
   1. [Fuzz targets](#fuzz-targets)
   2. [Corpora](#corpora)


## What is fuzzing

According to [OWASP](https://owasp.org/www-community/Fuzzing),
> Fuzz testing or Fuzzing is a Black Box software testing technique, which basically consists in finding implementation bugs using malformed/semi-malformed data injection in an automated fashion.

Here we take a more white box approach, and try random/semi-random inputs on syslog.ng or its parts.

## This is what you will need

In order to fuzz our software, you will need the following tools:

### [LibFuzzer](https://llvm.org/docs/LibFuzzer.html#faq)

>LibFuzzer is an in-process, coverage-guided, evolutionary fuzzing engine.
> 
>LibFuzzer is linked with the library under test, and feeds fuzzed inputs to the library via a specific fuzzing entrypoint (aka “target function”); the fuzzer then tracks which areas of the code are reached, and generates mutations on the corpus of input data in order to maximize the code coverage. The code coverage information for libFuzzer is provided by LLVM’s SanitizerCoverage instrumentation.

LibFuzzer well integrated with Clang from version 6.0, and is shipped as part of it. In order to compile and run our fuzz test, you will need an up-to-date version of Clang, the fresher, the better.

### Clang

Since LibFuzzer is part of Clang, if you want to fuzz syslog-ng, you will need to compile it with Clang. We try to use a fresh version, and so should you.

Fuzzing with Clang requires `clang` and `llvm` development packages. These might change distribution to distribution. Check with your distributor to inquire about package names.

## How to fuzz syslog-ng

The LibFuzzer library used by our fuzzing framework needs tests to run. These tests consist of two parts: test code and sample inputs. The code part is called a test target, and a sample input is called a corpus.

Please put each test in its own subdirectory in this folder. You can look at the example test to get an idea of how a test should look like, but the structure should be like this:

```
/.../syslog-ng
   - tests
      - fuzzing
         - tests
            - [your_test_folder_here]
               - corpora
                  - [your_corpus_1].txt
                  - [your_corpus_2].txt
                  - ...
               - targets
                  - [your_target_1].c
                  - [your_target_2].c
                  - ...
               - CMakeLists.txt (you have to write this)
               - Makefile.am (you have to write this)
            - CMakeLists.txt (add your folder)
            - Makefile.am (add your folder)
```

In addition to the corpora and fuzz targets, you also have to specify your CMake or automake target (either or both). You must also add your folder to the test level CMake or automake makefile (depending on your build environment). 

### Fuzz targets

A target is a function called by the fuzzing engine. It consists of the function call you want to test and supporting code. You may think of it like a unit test, but bigger code sections can also be tested. (Even end-to-end testing is possible, but not always recommended.)

If you want to fuzz your code, you will need to write your own targets and place them in the target subdirectory in your test directory.

Each target must be a function with the following signature:

```c
int LLVMFuzzerTestOneInput( [args] )
```

or 

```c++
extern "C" int LLVMFuzzerTestOneInput( [args] )
```

if by any chance you are using C++. Since syslog-ng is written in C, the latter is not recommended.

Please put each target in its own `.c` source file. Try to write simple tests to maximize LibFuzzer's performance.

### Corpora

The sample input used by mutational fuzzers is called a corpus (pl. corpuses or corpora). LibFuzzer in theory operates perfectly without corpora, but since syslog-ng sanitizes its own inputs, running tests without any is not recommended.

The reason for this is twofold:
 * A random input will most likely will be dropped quickly by the program, so it is much more efficient not to test inputs not adhering to the correct input syntax.
 * You can avoid inputs, which would likely break the tested code path, but which are already filtered by another function before calling the tested one. In the latter case you should also check the other function to make sure it behaves as intended.

<!-- TODO write tutorial on writing corpora. -->

### Building with CMake

First you should make a new directory. This should have the same name as your testcase for integration with the test environment. Place this new directory on the following path: `${PATH_TO_SYSLOG_NG_REPO}/tests/fuzzing/tests/${YOUR_TEST_DIRECTORY}`

You should add your `CMakeLists.txt` file to your test directory (on the same level as your `targets` and `corpora` folder). You should also add your module to `tests/fuzzing/tests/CMakeLists.txt`.

The minimum required `CMakeLists.txt` should contain at least a test target. To add this target, use the provided `add_fuzz_test()` function. Do not use CMake's integrated `add_executable()` or `add_custom_target()` functions unless you are completely sure in what you are doing, as these do not set the variables and command arguments used by CTest.

Here is an example:

```cmake
add_fuzz_target(TARGET example SRC targets/target_1.c LIBS lib1 lib2 lib3 CORPUS_DIR corpora/target_1 EXEC_PARMS -merge=1 -only-ascii)
```

The only required argument is `TARGET`. If you do not provide other arguments, the function will try to apply default values. These are:

| argument         |  type  |                default                | description                                                                                                                                                                                                       |
|------------------|:------:|:-------------------------------------:|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| TARGET           | SINGLE |             **required**              | The name of your testcase. CTest will apply a "fuzz_" prefix to it.                                                                                                                                               |
| EXPERIMENTAL     | OPTION |                  off                  | Enables some features not supported by default. Might result in better performance or more useful results.                                                                                                        |
| CORPUS_DIR       | SINGLE |                corpora                | Defines the directory in which the test input examples are located.                                                                                                                                               |
| TIMEOUT          | SINGLE |                1500 s                 | Defines the CTest timeout. For technical reasons LibFuzzer stops 10 seconds before this deadline to allow proper shutdown. Take care to set a long enough deadline to account for a possible testcase to timeout. |
| TESTCASE_TIMEOUT | SINGLE |                 60 s                  | Sets the timeout for a given testcase. Should be set based on the test, but the 60 seconds by default is a good blanket value.                                                                                    |
| SRC              | MULTI  |         targets/`${TARGET}`.c         | A list of your test sources. In theory, multiple sources are possible, but in practice, LibFuzzer might not support this.                                                                                         |
| LIBS             | MULTI  | empty (syslog-ng is added by default) | A list of the libraries your testcase should be compiled against.                                                                                                                                                 |
| EXEC_PARMS       | MULTI  |                 empty                 | Any extra parameters you might want to pass on to LibFuzzer. For more info, consult [the LibFuzzer manual](https://llvm.org/docs/LibFuzzer.html#options).                                                         |

The experimental features are the following:
 * `-print_final_stats`
 * `-detect-leaks`
 * `UBSAN` (sanitize undefined behaviour)
 * `MSAN` (memory sanitizing) instead of `ASAN` (address sanitizing)

You must also append your folder to `tests/fuzzing/tests/CMakeLists.txt` with the `add_subdirectory()` function.

### Building with automake

automake is currently not supported, but since it is our primary build system, it will be upon release.

