# WinAFL-TinyInst

## What is WinAFL-TinyInst

Added [TinyInst](https://github.com/googleprojectzero/TinyInst) to [`afl-fuzz.c`](https://github.com/donghyunlee00/WinAFL-TinyInst/blob/master/afl-fuzz.c) with option `-Z`.

## Building WinAFL-TinyInst

1. If you are building with DynamoRIO support, download and build
DynamoRIO sources or download DynamoRIO Windows binary package from
https://github.com/DynamoRIO/dynamorio/releases

2. Pull third party dependencies by running `git submodule update --init --recursive` from the WinAFL-TinyInst source directory

3. 

4. Open Visual Studio Command Prompt (or Visual Studio x64 Win64 Command Prompt
if you want a 64-bit build). Note that you need a 64-bit winafl.dll build if
you are fuzzing 64-bit targets and vice versa.

5. Go to the directory containing the source

6. Type the following commands. Modify the -DDynamoRIO_DIR flag to point to the
location of your DynamoRIO cmake files (either full path or relative to the
source directory).

### For a 32-bit build:

```
mkdir build32
cd build32
cmake -G"Visual Studio 16 2019" -A Win32 .. -DDynamoRIO_DIR=..\path\to\DynamoRIO\cmake -DINTELPT=1
cmake --build . --config Release
```

### For a 64-bit build:

```
mkdir build64
cd build64
cmake -G"Visual Studio 16 2019" -A x64 .. -DDynamoRIO_DIR=..\path\to\DynamoRIO\cmake -DINTELPT=1
cmake --build . --config Release
```

### Build configuration options

The following cmake configuration options are supported:

 - `-DDynamoRIO_DIR=..\path\to\DynamoRIO\cmake` - Needed to build the
   winafl.dll DynamoRIO client

 - `-DINTELPT=1` - Enable Intel PT mode. For more information see
   https://github.com/googleprojectzero/winafl/blob/master/readme_pt.md

 - `-DUSE_COLOR=1` - color support (Windows 10 Anniversary edition or higher)

 - `-DUSE_DRSYMS=1` - Drsyms support (use symbols when available to obtain
   -target_offset from -target_method). Enabling this has been known to cause
   issues on Windows 10 v1809, though there are workarounds,
   see https://github.com/googleprojectzero/winafl/issues/145

## Using WinAFL-TinyInst

The command line for afl-fuzz on Windows is different than on Linux. Instead of:

```
%s [ afl options ] -- target_cmd_line
```

it now looks like this:

```
afl-fuzz [afl options] -- [instrumentation options] -- target_cmd_line
```

The following afl-fuzz options are supported:

```
  -i dir        - input directory with test cases
  -o dir        - output directory for fuzzer findings
  -t msec       - timeout for each run
  -s            - deliver sample via shared memory
  -D dir        - directory containing DynamoRIO binaries (drrun, drconfig)
  -w path       - path to winafl.dll
  -e            - expert mode to run WinAFL-TinyInst as a DynamoRIO tool
  -P            - use Intel PT tracing mode
  -Y            - enable the static instrumentation mode
  -Z            - use TinyInst
  -f file       - location read by the fuzzed program
  -m limit      - memory limit for the target process
  -p            - persist DynamoRIO cache across target process restarts
  -c cpu        - the CPU to run the fuzzed program
  -d            - quick & dirty mode (skips deterministic steps)
  -n            - fuzz without instrumentation (dumb mode)
  -x dir        - optional fuzzer dictionary
  -I msec       - timeout for process initialization and first run
  -T text       - text banner to show on the screen
  -M \\ -S id   - distributed mode
  -C            - crash exploration mode (the peruvian rabbit thing)
  -l path       - a path to user-defined DLL for custom test cases processing
  -A module     - a module identifying a unique process to attach to
```

Example:

```
afl-fuzz.exe -Z -i in -o out -M test_1 -p fast -t 10000+ -- -callconv fastcall -target_method main -nargs 2 -loop -persist -iterations 5000 -instrument_module gdiplus.dll -instrument_module WindowsCodecs.dll -target_module test_gdiplus.exe -- test_gdiplus.exe @@
```

## References

- https://github.com/linhlhq/TinyAFL
