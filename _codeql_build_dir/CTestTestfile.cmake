# CMake generated Testfile for 
# Source directory: /home/runner/work/hwbench-c/hwbench-c
# Build directory: /home/runner/work/hwbench-c/hwbench-c/_codeql_build_dir
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[hwbench-tests]=] "/home/runner/work/hwbench-c/hwbench-c/_codeql_build_dir/hwbench-tests")
set_tests_properties([=[hwbench-tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/hwbench-c/hwbench-c/CMakeLists.txt;63;add_test;/home/runner/work/hwbench-c/hwbench-c/CMakeLists.txt;0;")
add_test([=[hwbench-smoke-list]=] "/home/runner/work/hwbench-c/hwbench-c/_codeql_build_dir/hwbench-c" "--list")
set_tests_properties([=[hwbench-smoke-list]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/hwbench-c/hwbench-c/CMakeLists.txt;65;add_test;/home/runner/work/hwbench-c/hwbench-c/CMakeLists.txt;0;")
add_test([=[hwbench-smoke-quick]=] "/home/runner/work/hwbench-c/hwbench-c/_codeql_build_dir/hwbench-c" "--suite" "quick" "--samples" "3" "--warmup-ms" "10" "--min-sample-ms" "10")
set_tests_properties([=[hwbench-smoke-quick]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/hwbench-c/hwbench-c/CMakeLists.txt;66;add_test;/home/runner/work/hwbench-c/hwbench-c/CMakeLists.txt;0;")
