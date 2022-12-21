#!/bin/bash
#############################################################################
# Copyright (c) 2022 One Identity LLC
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as published
# by the Free Software Foundation, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As an additional exemption you are allowed to compile & link against the
# OpenSSL libraries as published by the OpenSSL project. See the file
# COPYING for details.
#
#############################################################################

# Cache arguments
argv=("$@")
argc=$#

TRUE=0
FALSE=1

partition_line_length=32

## Formatting and partitioning
print_partition_template(){
  for (( printhead=0; printhead<=partition_line_length; printhead++ )); do
      printf -- "%s" "$1"
  done
  printf "\n"
}

print_partition_hash(){
  print_partition_template "#"
}

print_partition_dash(){
  print_partition_template "-"
}

print_partition_underline(){
  print_partition_template "_"
}

print_partition_star(){
  print_partition_template "*"
}

partition_h1(){
  print_partition_hash
}

partition_h2(){
  print_partition_star
}
partition_h3(){
  print_partition_underline
}
partition_h4(){
  print_partition_dash
}

help() {
  cat <<ENDHELP
    This script is meant to run a fuzz test on your target.

    Usage: $0 -t | --target [your target] <options>

    Options:
      -h | --help : _______________________ Prints this help message.

      -t | --target [target name] : _______ This is the name of your target. This option is mandatory. This should be the same as the folder of the testcase under 'fuzzing/tests'.

      -s | --source [source.c]: ___________ Sets the testcase source file. By default it is 'fuzzing/tests/\${target}/target/\${target}.c'.

      -c | --corpus [corpus dir]: _________ Sets the directory name containing the corpora under your testcase folder. By default it is 'fuzzing/tests/\${target}/corpora'.

      -l | --libraries [linked libraries] : The list of libraries your test should be linked against. Surround with quotes, if more than one is specified. syslog-ng is linked by default.

      -E | --experimental : _______________ Enables experimental features while testing. For more information, see the included documentation.

      -T | --timeout [X]: _________________ Sets the timeout in seconds for the fuzzing test. If not provided, 1500 is used by default.

      -C | --testcase-timeout [X] : _______ Sets the testcase_timeout in seconds for the fuzzing test. If not provided, 60 is used by default.

      -e | --exec-parameters [parameters]:  Provides extra execution parameters for the fuzzer. For more information, see the libFuzzer documentation. Surround with quotes, if more than one is specified.

ENDHELP

  exit 0
}

check_not_empty() {
  if [ "${1:+''}" == "" ]; then
    return $FALSE
  else
    return $TRUE
  fi
}

handle_empty() {
  if $(check_not_empty "$1"); then
    return $TRUE
  else
    printf "invalid %s\n", "$2"
    exit 1
  fi
}

skip_next_arg=$FALSE

target_name=""
set_target() {
  handle_empty "$1" "target"
  target_name="$1"
  skip_next_arg=$TRUE
}

source=""
set_source() {
  handle_empty "$1" "source"
  source="./tests/${target_name}/${1}"
  skip_next_arg=$TRUE
}

libraries="syslog-ng"
set_libs() {
  handle_empty "$1" "libraries"
  libraries+="$1"
  skip_next_arg=$TRUE
}

exec_parameters=""
set_exec_parameters() {
  handle_empty "$1" "execution parameters"
  exec_parameters+="$1"
  skip_next_arg=$TRUE
}

experimental=false
set_experimental() {
  experimental=true
  skip_next_arg=$FALSE
}

corpus=""
set_corpus() {
  handle_empty "$1" "corpus folder"
  corpus="./tests/${target_name}/${1}"
  skip_next_arg=$TRUE
}

timeout=1500
set_timeout() {
  handle_empty "$1" "timeout"
  timeout="$1"
  skip_next_arg=$TRUE
}

testcase_timeout=60
set_testcase_timeout() {
  handle_empty "$1" "timeout"
  testcase_timeout="$1"
  skip_next_arg=$TRUE
}

compiler_options=""
composite_compiler_options() {
  if $experimental; then
    compiler_options="-o1 -fsanitize=\"fuzzer,memory,signed-integer-overflow,null,alignment\" -fno-omit-frame-pointer"
  else
    compiler_options="-o1 -fsanitize=\"address,fuzzer\" -fno-omit-frame-pointer"
  fi
}

linker_options=""
composite_linker_options() {
  if $experimental; then
    linker_options="-o1 -fsanitize=\"fuzzer,memory,signed-integer-overflow,null,alignment\" -fno-omit-frame-pointer"
  else
    linker_options="-o1 -fsanitize=\"address,fuzzer\" -fno-omit-frame-pointer"
  fi
}

check_arg_switch() {
  arg=$1
  next_arg=$2
  case $arg in

  -h | --help)
    help
    ;;

  -t | --target)
    set_target "$next_arg"
    ;;

  -l | --libraries)
    set_libs "$next_arg"
    ;;

  -E | --experimental)
    set_experimental
    ;;

  -c | --corpus)
    set_corpus "$next_arg"
    ;;

  -T | --timeout)
    set_timeout "$next_arg"
    ;;

  -C | --testcase-timeout)
    set_testcase_timeout "$next_arg"
    ;;

  -s | --source)
    set_source "$next_arg"
    ;;

  -e | --exec-parameters)
    set_exec_parameters "$next_arg"
    ;;

  *)
    printf "Invalid argument %s\n", "$arg"
    exit 1
    ;;
  esac
}

#####################
# EXECUTION FROM HERE
#####################

if [ $# -lt 1 ]; then
  printf "At least a target name should be given.\n"
  printf "For proper usage, run %s --help.\n", "$0"
  exit 1
fi

# Argument processing
arg_index=0
while [ "$arg_index" -lt "$argc" ]; do
  check_arg_switch "${argv[$arg_index]}" "${argv[$arg_index + 1]}"
  ((arg_index += 1))
  if [ $skip_next_arg -eq 0 ]; then
    ((arg_index += 1))
  fi
done

# Finding clang
partition_h1
clang_location=$(which clang)
# shellcheck disable=SC2181
if [ $? -eq 0 ]; then
  printf "Clang found at %s\n" "$clang_location"
  partition_h3
else
  printf "No Clang found.\nYou need Clang to continue.\n"
  partition_h1
  exit 1
fi

# Compiler/linker settings
composite_compiler_options
composite_linker_options

fuzz_target_name="fuzz_"$target_name

# Variable check

#default variables
if ! check_not_empty "$source"; then
  printf "No source provided, using default\n"
  source="./tests/${target_name}/targets/${target_name}.c"
fi
if ! check_not_empty "$corpus"; then
  printf "No corpus directory provided, using default\n"
  corpus="./tests/${target_name}/corpora"
fi

cat <<ENDCHECK
$(eval partition_h2)
$fuzz_target_name
$(eval partition_h2)
$(eval partition_h4)
Parameters:
  - target:                 $target_name
  - source:                 $source
  - corpus folder:          $corpus
  - libraries:              $libraries
  - compiler parameters:    $compiler_options
  - linker parameters:      $linker_options
  - extra exec. parameters: $exec_parameters
  $(
  eval
  if [ $experimental == true ]; then
    printf -- "- experimental features:  ON"
  else
    printf -- "- experimental features:  OFF"
  fi
)
  - timeouts (test/total):  ${testcase_timeout}/${timeout}
$(eval partition_h2)
ENDCHECK

printf "Compiling...\n"
partition_h3

#if ! $(cd "tests/${target_name}" 2>/dev/null);then
#  printf 'could not find directory "%s"\n' "$target_name"
#  printf "Exiting\n"
#  exit 1
#fi

if ! clang -g -o "$fuzz_target_name" "$linker_options" "$source"; then
  printf "Compilation failed, exiting.\n"
  partition_h1
  exit 1
fi

printf "Running test...\n"
partition_h2
./"$fuzz_target_name" -max_total_time="$timeout" -timeout="$testcase_timeout" "$exec_parameters" "$corpus"
partition_h1
