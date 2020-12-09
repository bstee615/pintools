# Trace

Trace is a [Pintool](https://software.intel.com/content/www/us/en/develop/articles/pin-a-dynamic-binary-instrumentation-tool.html) that prints out the location of each statement in a trace of the program.

# Unpacking

Clone this project into `<pin-3.16-root>/source/tools/` (next to the other example Pintools).

# Usage

```
cd <pin-3.16-root>/source/tools/    # Navigate to the project root
make                                # Build the Pintool
gcc -g -O0 test.c -o test           # Build the example program with debug flags and no optimizations
../../../pin \                      # Run Pin...
    -t obj-intel64/trace.so \       # ...with our tool...
    -- ./test                       # ...on the example program.
```

Make sure to compile the target program with debug flag `-g`.

Example output:
```
[me@centos8 Trace]$ ../../../pin -t obj-intel64/trace.so -- ./a.out
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:3
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:4
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:5
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:8
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:9
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:11
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:13
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:14
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:15
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:16
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:17
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:14
/home/me/work/dynamic/pin-3.16-98275-ge0db48c31-gcc-linux/source/tools/Trace/test.c:19
```
