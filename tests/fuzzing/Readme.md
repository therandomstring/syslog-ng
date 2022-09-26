# Fuzzing framework for syslog-ng

## Contents:
1. [What is fuzzing](#what-is-fuzzing)
2. [Used tools](#this-is-what-you-will-need)
   1. [LibFuzzer](#libfuzzer)
   2. [Clang](#clang)
3. [How to fuzz](#how-to-fuzz-syslog-ng)
   1. [Fuzz targets](#fuzz-targets)
   2. [Corpora](#corpora)


## What is fuzzing

According to [OWASP](https://owasp.org/www-community/Fuzzing),
> Fuzz testing or Fuzzing is a Black Box software testing technique, which basically consists in finding implementation bugs using malformed/semi-malformed data injection in an automated fashion.

Here we take a more white box approach, and try random/semi-random inputs on syslog.ng or its parts.

## Used tools

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

Please put each test in its own subdirectory in this folder. You can look at the example test to get an idea of the structure.

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

You should add your `CMakeLists.txt` file to your test directory (on the same level as your `targets` and `corpora` folder). You should also add your module to `tests/fuzzing/CMakeLists.txt`.

<!-- TODO how to write a fuzzing target. -->

### Building with automake

automake is currently not supported, but since it is our primary build system, it will be upon release.

