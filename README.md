# tm — lightweight ps/top-style viewer for Windows

`tm` is a small C++20 command-line tool that mimics a BSD-style `ps`/`top` view on Windows. It enumerates processes, shows per-process CPU% (sampled over time), working-set memory, thread count, and parent PID, and can optionally terminate a process.

The default run prints one snapshot sized to fit your console without scrolling. Top mode refreshes the display at a user-controlled interval.

## Features
- One-shot snapshot similar to `ps` with a concise summary header.
- Top mode that refreshes in-place (default every 1 second).
- Console-aware row limiting to avoid scrolling; override with `-n/--numprocs`.
- Kill a process by PID with `-k/--kill`.
- C++20 implementation using Win32 APIs (`Toolhelp32Snapshot`, `GetProcessTimes`, `GetSystemTimes`, `GetProcessMemoryInfo`).

## Usage
```
tm.exe [options]

Options:
  -t, --top                 Refresh continuously (top mode)
  -s, --seconds <sec>       Seconds between refreshes (implies -t)
  -n, --numprocs <n>        Max processes to display (default: fit console)
  -k, --kill <pid>          Terminate a process
  -h, -?, --help            Show this help
```

Examples:
- One-shot snapshot: `tm.exe`
- Top mode, default 1s refresh: `tm.exe -t`
- Top mode every 2.5 seconds: `tm.exe -s 2.5`
- Limit rows to 30: `tm.exe -n 30`
- Kill PID 1234, then exit: `tm.exe -k 1234`
- Kill and then stay in top mode: `tm.exe -k 1234 -t`

## How it works (brief)
- Processes are enumerated with `CreateToolhelp32Snapshot`.
- Per-process CPU% is approximated by comparing `GetProcessTimes` deltas to `GetSystemTimes` deltas across samples.
- Memory comes from `GetProcessMemoryInfo` (working set in MB).
- Console height is read via `GetConsoleScreenBufferInfo`; the tool subtracts header lines so the summary stays visible.

## Building on Windows (novice-friendly)
You need the Microsoft C++ toolchain (Visual Studio or Build Tools) and `nmake`.

1) Install:
   - Install **Visual Studio 2022** (any edition) with the **Desktop development with C++** workload; or
   - Install **Build Tools for Visual Studio 2022** with the same C++ workload.

2) Open a **Developer Command Prompt for VS 2022** (or x64 Native Tools Command Prompt). This preconfigures `cl` and `nmake`.

3) In the project folder, build:
   ```
   nmake
   ```
   This uses the provided `Makefile` (targets `tm.exe`, links `psapi.lib`).

Alternate: run the helper script that sets up the environment then builds:
```
cmd /c build.cmd
```
(This calls `vcvarsall.bat x64` and then `nmake`.)

4) Run:
   ```
   tm.exe
   ```

## File overview
- `main.cpp` — the entire application (argument parsing, sampling, rendering).
- `Makefile` — `nmake` build rules using `cl` and `psapi.lib`.
- `build.cmd` — convenience script to load the MSVC environment and invoke `nmake`.

## Notes and limitations
- CPU% is sampled; very short-lived processes might show 0% or not appear.
- Requires access rights to query or terminate target processes; some system processes may not open or kill.
- Output is UTF-8; executable names are narrow-converted from wide strings.

Enjoy! Contributions and tweaks are welcome.***
