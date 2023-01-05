# Fuzzing framework for syslog-ng

## Contents:
1. [What is fuzzing](#what-is-fuzzing)
2. [Used tools](#this-is-what-you-will-need)
   1. [LibFuzzer](#libfuzzerhttpsllvmorgdocslibfuzzerhtmlfaq)
   2. [Clang](#clang)
3. [How to fuzz](#how-to-fuzz-syslog-ng)
   1. [Fuzz targets](#fuzz-targets)
   2. [Corpora](#corpora)
   3. [Building targets with CMake](#building-with-cmake)
   4. [Building targets with automake](#building-with-automake)
   5. [Fuzzing without a build system](#running-fuzz-tests-without-a-build-system)
   6. [Experimental features](#experimental-features)


## What is fuzzing

According to [OWASP](https://owasp.org/www-community/Fuzzing),
> Fuzz testing or Fuzzing is a Black Box software testing technique, which basically consists in finding implementation bugs using malformed/semi-malformed data injection in an automated fashion.

Here we take a more white box approach, and try random/semi-random inputs on syslog-ng or its parts.

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

___
**NOTICE**

Unfortunately some of our modules might not compile well, or not compile at all with Clang. As we are continuously trying to improve syslog-ng, the list of these is subject to change. We are currently compiling a list of these problematic modules, and it will be included as soon as ready.

<!-- TODO: include list when ready -->
___

## How to fuzz syslog-ng

The LibFuzzer library used by our fuzzing framework needs tests to run. These tests consist of two parts: test code and sample inputs. The code part is called a test target, and a sample input is called a corpus.

Please put each test in its own subdirectory in this folder. You can look at the example test to get an idea of how a test should look like, but the structure should be like this:

___
**NOTICE**

Fuzzing might find bugs in libs used by syslog-ng. These are outside our influence, and should be reported to the respective project owners.
___

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

Your test folder should have the same name as your testcase for integration with the test environment.

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

#### Helper libraries

We have provided some helper libraries with this framework to ease writing testcases. You will find these in the `lib` directory under `fuzzing`. They are not documented here, but instead are commented in the header(s).

### Corpora

The sample input used by mutational fuzzers is called a corpus (pl. corpuses or corpora). LibFuzzer in theory operates perfectly without corpora, but since syslog-ng sanitizes its own inputs, running tests without any is not recommended.

The reason for this is twofold:
 * A random input will most likely will be dropped quickly by the program, so it is much more efficient not to test inputs not adhering to the correct input syntax.
 * You can avoid inputs, which would likely break the tested code path, but which are already filtered by another function before calling the tested one. In the latter case you should also check the other function to make sure it behaves as intended.

<!-- TODO write tutorial on writing corpora. -->

### Building with CMake

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
| EXPERIMENTAL     | OPTION |                  off                  | Enables some [features](#experimental-features) not supported by default. Might result in better performance or more useful results.                                                                              |
| CORPUS_DIR       | SINGLE |                corpora                | Defines the directory in which the test input examples are located.                                                                                                                                               |
| TIMEOUT          | SINGLE |                1500 s                 | Defines the CTest timeout. For technical reasons LibFuzzer stops 10 seconds before this deadline to allow proper shutdown. Take care to set a long enough deadline to account for a possible testcase to timeout. |
| TESTCASE_TIMEOUT | SINGLE |                 60 s                  | Sets the timeout for a given testcase. Should be set based on the test, but the 60 seconds by default is a good blanket value.                                                                                    |
| SRC              | MULTI  |         targets/`${TARGET}`.c         | A list of your test sources. In theory, multiple sources are possible, but in practice, LibFuzzer might not support this.                                                                                         |
| LIBS             | MULTI  | empty (syslog-ng is added by default) | A list of the libraries your testcase should be compiled against.                                                                                                                                                 |
| EXEC_PARMS       | MULTI  |                 empty                 | Any extra parameters you might want to pass on to LibFuzzer. For more info, consult [the LibFuzzer manual](https://llvm.org/docs/LibFuzzer.html#options).                                                         |

You must also append your folder to `tests/fuzzing/tests/CMakeLists.txt` with the `add_subdirectory()` function.

### Building with automake

automake is currently not supported, but since it is our primary build system, it will be upon release.

Fuzzing with automake is will be possible by the included shell script.
To add a fuzz test target, the following are necessary:
 * Add a makefile (`Makefile.am`) to your test directory.
 * Include your directory in the `EXTRA_DIST` section in `fuzzing/tests/Makefile.am`
 * In your makefile, add a target to the included shell script. The correct argumentation is found int the [corresponding section](#running-fuzz-tests-without-a-build-system).

### Running fuzz tests without a build system
We have included [`run_fuzz_test.sh`](run_fuzz_test.sh) to enable adding fuzz tests as an automake/makefile target. Incidentally this same script can be used to run testcases without a build system. This can be useful, when a full incremental build (such as with CTest) is undesirable.</br>
To usage of the script is as follows: (the same information is printed with the `-h` or `--help` option)

| switch             | short |        argument        |   default value    | description                                                  |
|--------------------|:-----:|:----------------------:|:------------------:|--------------------------------------------------------------|
| --help             |  -h   |         [NONE]         |       [NONE]       | Prints help message                                          |
| --target           |  -t   |      target name       |    [MANDATORY]     | The name of your target. This option is mandatory.           |
| --source           |  -s   |      source file       | targets/[TARGET].c | Sets the source file containing the fuzz testcase.           |
| --corpus           |  -c   |    corpus directory    |      corpora       | Sets the directory containing the corpora.                   |
| --libraries        |  -l   | extra linker libraries |      [EMPTY]       | Specifies extra libraries to link the target against.        |
| --exec-parameters  |  -e   |  execution parameters  |      [EMPTY]       | Passes on extra execution parameters to the fuzzer.          |
| --timeout          |  -T   |   timeout in seconds   |        1500        | Defines a total timeout for the fuzzer.                      |
| --testcase-timeout |  -C   |   timeout in seconds   |         60         | Defines a per-testcase timeout.                              |
| --experimental     |  -E   |         [NONE]         |       [NONE]       | Enables the [experimental features](#experimental-features). |

### Experimental features

libFuzzer comes with certain non-standard or not-yet-stable features. In addition to this, certain standard features are also switched off by default in this framework.</br>
You can switch on some of these by enabling the experimental feature set.

The experimental features are the following:
* `-print_final_stats`
* `-detect-leaks`
* `UBSAN` (sanitize undefined behaviour)
* `MSAN` (memory sanitizing) instead of `ASAN` (address sanitizing)

## TODO

 * Implement rerun failed mode (--rerun-failed --output-on-failure)
 * -use_value_profile in experimental mode (also add to the paragraph)
