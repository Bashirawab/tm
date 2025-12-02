// ***************************************************************************
//                                                                            
//      ░██████████░███     ░███                                      
//          ░██    ░████   ░████                                      
//          ░██    ░██░██ ░██░██      ░███████  ░██    ░██  ░███████  
//          ░██    ░██ ░████ ░██     ░██    ░██  ░██  ░██  ░██    ░██ 
//          ░██    ░██  ░██  ░██     ░█████████   ░█████   ░█████████ 
//          ░██    ░██       ░██     ░██         ░██  ░██  ░██        
//          ░██    ░██       ░██ ░██  ░███████  ░██    ░██  ░███████  
//                                                                    
//                                                                            
//  ════════════════════════════════════════════════════════════════════════  
//                                                                            
//  PROGRAM:     CONSOLE BASED TASK MANAGER                         
//  MODULE:      MAIN.CPP                                                       
//  VERSION:     0.2                                                          
//  DATE:        DECEMBER 2025                                                
//                                                                            
//  ════════════════════════════════════════════════════════════════════════  
//                                                                            
//  DESCRIPTION:                                                              
//                                                                            
//    main processes options, spawns top mode or single snapshot, and 
//    handles process termination requests.                                      
//                                                                            
//  ════════════════════════════════════════════════════════════════════════  
//                                                                            
//  AUTHOR:      DAVE PLUMMER AND VARIOUS AI ASSISTS                                          
//  LICENSE:     GPL 2.0                                                      
//                                                                            
// ***************************************************************************

#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <optional>

struct Options {
    bool topMode{false};
    double intervalSeconds{1.0};
    std::optional<DWORD> killPid;
    size_t maxProcs{0}; // 0 = auto-fit to console height
};

// Converts FILETIME to a 64-bit tick count.
uint64_t fileTimeToUint64(const FILETIME &ft) {
    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    return li.QuadPart;
}

// Reads total non-idle system time ticks for CPU% calculations.
uint64_t readSystemTime() {
    FILETIME idle{}, kernel{}, user{};
    if (!GetSystemTimes(&idle, &kernel, &user)) {
        return 0;
    }
    const uint64_t kernelTime = fileTimeToUint64(kernel);
    const uint64_t userTime = fileTimeToUint64(user);
    const uint64_t idleTime = fileTimeToUint64(idle);
    return (kernelTime + userTime) - idleTime;
}

// Converts wide strings to UTF-8 narrow strings.
std::string narrow(const wchar_t *wstr) {
    if (!wstr) {
        return {};
    }
    const int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) {
        return {};
    }
    std::string result(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), len, nullptr, nullptr);
    return result;
}

// Builds a readable Windows error message.
std::string formatError(DWORD code) {
    LPWSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = FormatMessageW(flags, nullptr, code, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    if (length == 0 || buffer == nullptr) {
        return "unknown error";
    }
    std::wstring message(buffer, buffer + length);
    LocalFree(buffer);
    std::string result = narrow(message.c_str());
    while (!result.empty() && (result.back() == '\r' || result.back() == '\n')) {
        result.pop_back();
    }
    return result;
}

// Formats uptime in a friendly d/h/m/s string.
std::string formatUptime(uint64_t milliseconds) {
    uint64_t totalSeconds = milliseconds / 1000;
    const uint64_t days = totalSeconds / 86400;
    totalSeconds %= 86400;
    const uint64_t hours = totalSeconds / 3600;
    totalSeconds %= 3600;
    const uint64_t minutes = totalSeconds / 60;
    const uint64_t seconds = totalSeconds % 60;
    std::ostringstream oss;
    if (days > 0) {
        oss << days << "d ";
    }
    oss << hours << "h " << minutes << "m " << seconds << "s";
    return oss.str();
}

struct ProcessRow {
    DWORD pid{0};
    DWORD ppid{0};
    double cpuPercent{0.0};
    double workingSetMb{0.0};
    DWORD threads{0};
    std::string name;
};

// Tracks per-process CPU deltas between samples to approximate top-like CPU%.
class ProcessSampler {
  public:
    // Collects process info and computes CPU% based on previous sample.
    std::vector<ProcessRow> sample() {
        const uint64_t systemTime = readSystemTime();
        std::unordered_map<DWORD, uint64_t> currentTimes;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return {};
        }

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        BOOL hasProcess = Process32FirstW(snapshot, &entry);
        std::vector<ProcessRow> rows;
        rows.reserve(128);

        while (hasProcess) {
            ProcessRow row{};
            row.pid = entry.th32ProcessID;
            row.ppid = entry.th32ParentProcessID;
            row.threads = entry.cntThreads;
            row.name = narrow(entry.szExeFile);

            uint64_t procTime = 0;
            SIZE_T workingSet = 0;
            HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID);
            if (handle) {
                FILETIME create{}, exit{}, kernel{}, user{};
                if (GetProcessTimes(handle, &create, &exit, &kernel, &user)) {
                    procTime = fileTimeToUint64(kernel) + fileTimeToUint64(user);
                }

                PROCESS_MEMORY_COUNTERS pmc{};
                if (GetProcessMemoryInfo(handle, &pmc, sizeof(pmc))) {
                    workingSet = pmc.WorkingSetSize;
                }
                CloseHandle(handle);
            }

            currentTimes[row.pid] = procTime;

            double cpu = 0.0;
            if (prevSystemTime_ != 0 && systemTime > prevSystemTime_) {
                const auto it = prevProcTimes_.find(row.pid);
                if (it != prevProcTimes_.end() && procTime >= it->second) {
                    const uint64_t procDelta = procTime - it->second;
                    const uint64_t sysDelta = systemTime - prevSystemTime_;
                    if (sysDelta > 0) {
                        cpu = 100.0 * static_cast<double>(procDelta) / static_cast<double>(sysDelta);
                    }
                }
            }

            row.cpuPercent = cpu;
            row.workingSetMb = static_cast<double>(workingSet) / (1024.0 * 1024.0);
            rows.push_back(std::move(row));
            hasProcess = Process32NextW(snapshot, &entry);
        }

        CloseHandle(snapshot);
        prevProcTimes_ = std::move(currentTimes);
        prevSystemTime_ = systemTime;
        lastProcessCount_ = rows.size();

        std::sort(rows.begin(), rows.end(), [](const ProcessRow &a, const ProcessRow &b) {
            if (a.cpuPercent == b.cpuPercent) {
                return a.pid < b.pid;
            }
            return a.cpuPercent > b.cpuPercent;
        });
        return rows;
    }

    // Returns number of processes seen in the last sample.
    size_t lastProcessCount() const { return lastProcessCount_; }

  private:
    uint64_t prevSystemTime_{0};
    std::unordered_map<DWORD, uint64_t> prevProcTimes_{};
    size_t lastProcessCount_{0};
};

// Prints usage/help text.
void printUsage(const char *exe) {
    std::cout << "Usage: " << exe << " [options]\n\n"
              << "  -t, --top             Refresh continuously (top mode)\n"
              << "  -s, --seconds <sec>   Seconds between refreshes (implies -t)\n"
              << "  -n, --numprocs <n>    Max processes to display (default: fit console)\n"
              << "  -k, --kill <pid>      Terminate a process\n"
              << "  -h, -?, --help        Show this help\n";
}

// Parses seconds value and validates positivity.
bool parseSeconds(const std::string &value, double &out) {
    try {
        out = std::stod(value);
        return out > 0.0;
    } catch (...) {
        return false;
    }
}

// Parses a PID as unsigned integer.
bool parsePid(const std::string &value, DWORD &pid) {
    try {
        const unsigned long parsed = std::stoul(value);
        pid = static_cast<DWORD>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

// Parses a positive count for row limiting.
bool parseCount(const std::string &value, size_t &count) {
    try {
        const unsigned long parsed = std::stoul(value);
        if (parsed == 0) {
            return false;
        }
        count = static_cast<size_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

// Terminates a process by PID with error reporting.
bool killProcess(DWORD pid, std::string &error) {
    HANDLE handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!handle) {
        error = "OpenProcess failed: " + formatError(GetLastError());
        return false;
    }
    const BOOL terminated = TerminateProcess(handle, 1);
    CloseHandle(handle);
    if (!terminated) {
        error = "TerminateProcess failed: " + formatError(GetLastError());
        return false;
    }
    return true;
}

// Prints the top-style summary header (uptime, mem, CPU count, procs).
void printSummary(double intervalSeconds, size_t processCount) {
    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);

    const double totalMb = static_cast<double>(mem.ullTotalPhys) / (1024.0 * 1024.0);
    const double usedMb = static_cast<double>(mem.ullTotalPhys - mem.ullAvailPhys) / (1024.0 * 1024.0);

    const uint64_t uptimeMs = GetTickCount64();
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    std::cout << "top-like view (interval " << std::fixed << std::setprecision(2) << intervalSeconds
              << "s) | procs: " << processCount
              << " | uptime: " << formatUptime(uptimeMs) << '\n';

    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &nowTime);
#else
    local = *std::localtime(&nowTime);
#endif
    std::cout << "Time: " << std::put_time(&local, "%Y-%m-%d %H:%M:%S")
              << " | Mem: " << std::setprecision(1) << usedMb << "MB/" << totalMb << "MB"
              << " | Logical CPUs: " << sysInfo.dwNumberOfProcessors << "\n\n";
}

// Prints the process table up to maxRows entries.
void printTable(const std::vector<ProcessRow> &rows, size_t maxRows) {
    std::cout << std::left << std::setw(7) << "PID"
              << std::setw(7) << "PPID"
              << std::right << std::setw(8) << "CPU%"
              << std::setw(12) << "MEM(MB)"
              << std::setw(9) << "THREADS"
              << "  NAME" << '\n';
    std::cout << std::string(60, '-') << '\n';
    std::cout << std::fixed << std::setprecision(1);

    const size_t limit = maxRows > 0 ? std::min(maxRows, rows.size()) : rows.size();
    for (size_t idx = 0; idx < limit; ++idx) {
        const auto &row = rows[idx];
        std::cout << std::left << std::setw(7) << row.pid
                  << std::setw(7) << row.ppid
                  << std::right << std::setw(8) << std::setprecision(1) << row.cpuPercent
                  << std::setw(12) << std::setprecision(1) << row.workingSetMb
                  << std::setw(9) << row.threads
                  << "  " << row.name << '\n';
    }
}

// Parses all CLI options and detects bad/ help cases.
Options parseOptions(int argc, char *argv[], bool &showHelp, bool &badArgs) {
    Options opts{};
    bool secondsProvided = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "-?" || arg == "--help") {
            showHelp = true;
            return opts;
        } else if (arg == "-t" || arg == "--top") {
            opts.topMode = true;
        } else if (arg.rfind("--top=", 0) == 0) {
            const auto value = arg.substr(std::string("--top=").size());
            const auto lower = [value]() {
                std::string v = value;
                std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return v;
            }();
            if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") {
                opts.topMode = true;
            } else if (lower == "0" || lower == "false" || lower == "no" || lower == "off") {
                opts.topMode = false;
            } else {
                badArgs = true;
                return opts;
            }
        } else if (arg == "-s" || arg == "--seconds") {
            if (i + 1 >= argc) {
                badArgs = true;
                return opts;
            }
            double seconds = 0.0;
            if (!parseSeconds(argv[++i], seconds)) {
                badArgs = true;
                return opts;
            }
            opts.intervalSeconds = seconds;
            secondsProvided = true;
        } else if (arg == "-k" || arg == "--kill") {
            if (i + 1 >= argc) {
                badArgs = true;
                return opts;
            }
            DWORD pid = 0;
            if (!parsePid(argv[++i], pid)) {
                badArgs = true;
                return opts;
            }
            opts.killPid = pid;
        } else if (arg == "-n" || arg == "--numprocs") {
            if (i + 1 >= argc) {
                badArgs = true;
                return opts;
            }
            size_t count = 0;
            if (!parseCount(argv[++i], count)) {
                badArgs = true;
                return opts;
            }
            opts.maxProcs = count;
        } else if (arg.rfind("--numprocs=", 0) == 0) {
            size_t count = 0;
            if (!parseCount(arg.substr(std::string("--numprocs=").size()), count)) {
                badArgs = true;
                return opts;
            }
            opts.maxProcs = count;
        } else {
            badArgs = true;
            return opts;
        }
    }

    if (secondsProvided) {
        opts.topMode = true;
    }
    return opts;
}

// Reads the number of visible console rows (0 if unavailable).
size_t detectConsoleRows() {
    const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (hOut == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(hOut, &info)) {
        return 0;
    }
    const SHORT rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    return rows > 0 ? static_cast<size_t>(rows) : 0;
}

// Determines how many process rows to show based on console and user cap.
size_t resolveMaxRows(const Options &opts) {
    size_t rows = detectConsoleRows();
    // summary block (2 lines + trailing blank) + table header (2 lines) + one extra buffer row
    constexpr size_t summaryLines = 3;
    constexpr size_t tableHeaderLines = 2;
    constexpr size_t extraPad = 1;
    const size_t reserved = summaryLines + tableHeaderLines + extraPad;
    if (rows == 0) {
        rows = 24; // fallback when console size unknown
    }
    const size_t visibleCapacity = rows > reserved ? rows - reserved : 0;

    if (opts.maxProcs > 0) {
        // Honor user cap but do not exceed visible rows when known.
        if (visibleCapacity > 0) {
            return std::min(visibleCapacity, opts.maxProcs);
        }
        return opts.maxProcs;
    }
    return visibleCapacity;
}

// Runs a single snapshot: warm up sampling, wait briefly, then print.
void renderOnce(ProcessSampler &sampler, double initialDelaySeconds, const Options &opts) {
    sampler.sample();
    std::this_thread::sleep_for(std::chrono::duration<double>(initialDelaySeconds));
    const auto rows = sampler.sample();
    const size_t maxRows = resolveMaxRows(opts);
    printSummary(initialDelaySeconds, rows.size());
    printTable(rows, maxRows);
}

// Runs continuous top-style refresh until interrupted.
void renderTop(ProcessSampler &sampler, double intervalSeconds, const Options &opts) {
    sampler.sample();
    while (true) {
        std::this_thread::sleep_for(std::chrono::duration<double>(intervalSeconds));
        const auto rows = sampler.sample();
        system("cls");
        const size_t maxRows = resolveMaxRows(opts);
        printSummary(intervalSeconds, rows.size());
        printTable(rows, maxRows);
    }
}

// Entry point: parse options, handle kill, then render snapshot or top mode.
int main(int argc, char *argv[]) {
    std::ios::sync_with_stdio(false);

    bool showHelp = false;
    bool badArgs = false;
    Options opts = parseOptions(argc, argv, showHelp, badArgs);

    if (showHelp) {
        printUsage(argv[0]);
        return 0;
    }
    if (badArgs) {
        printUsage(argv[0]);
        return 1;
    }

    if (opts.killPid) {
        std::string error;
        if (killProcess(*opts.killPid, error)) {
            std::cout << "Killed PID " << *opts.killPid << "\n";
        } else {
            std::cerr << error << "\n";
            return 1;
        }
        if (!opts.topMode) {
            return 0;
        }
    }

    ProcessSampler sampler;
    constexpr double initialDeltaSeconds = 0.2;

    if (opts.topMode) {
        renderTop(sampler, opts.intervalSeconds, opts);
    } else {
        renderOnce(sampler, initialDeltaSeconds, opts);
    }
    return 0;
}
