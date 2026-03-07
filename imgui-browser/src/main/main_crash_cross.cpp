// main_crash_cross.cpp -- Portable crash/signal handler for Linux and Windows.
// On macOS this is replaced by main_crash.mm which adds ObjC exception handling.
// This file must NOT be compiled on Apple targets (excluded by CMakeLists.txt).

#include "main_crash.h"
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <signal.h>

#ifdef _WIN32
#  include <windows.h>
#  define MAKE_DIR(p) CreateDirectoryA(p, nullptr)
#else
#  include <execinfo.h>
#  include <sys/stat.h>
#  define MAKE_DIR(p) mkdir(p, 0755)
#endif

char g_debug_dir[1024] = {};
char g_crash_log[1024] = {};

void xcm_write_crash(const char* header, const char* detail) {
    fprintf(stderr, "\n[CRASH] %s\n%s\n", header, detail);
    fflush(stderr);
    if (!g_crash_log[0]) return;
    FILE* f = fopen(g_crash_log, "a");
    if (!f) return;
    time_t t = time(nullptr);
    char tbuf[64];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    fprintf(f, "========================================\n");
    fprintf(f, "[%s] CRASH: %s\n", tbuf, header);
    fprintf(f, "%s\n", detail);
#ifndef _WIN32
    void* frames[64];
    int   nframes = backtrace(frames, 64);
    char** syms   = backtrace_symbols(frames, nframes);
    fprintf(f, "--- backtrace (%d frames) ---\n", nframes);
    for (int i = 0; i < nframes; i++)
        fprintf(f, "  %s\n", syms ? syms[i] : "(unavailable)");
    if (syms) free(syms);
#endif
    fprintf(f, "========================================\n");
    fclose(f);
}

static void xcm_signal_handler(int sig) {
    const char* name = "UNKNOWN";
    switch (sig) {
        case SIGABRT: name = "SIGABRT (abort/assertion)"; break;
        case SIGSEGV: name = "SIGSEGV (segmentation fault)"; break;
#ifndef _WIN32
        case SIGBUS:  name = "SIGBUS (bus error)"; break;
#endif
        case SIGILL:  name = "SIGILL (illegal instruction)"; break;
        case SIGFPE:  name = "SIGFPE (floating point exception)"; break;
#ifndef _WIN32
        case SIGPIPE: name = "SIGPIPE (broken pipe -- ignored)"; return;
#endif
    }
    xcm_write_crash(name, "see backtrace above");
    signal(sig, SIG_DFL);
    raise(sig);
}

void xcm_install_signal_handlers() {
    signal(SIGABRT, xcm_signal_handler);
    signal(SIGSEGV, xcm_signal_handler);
    signal(SIGILL,  xcm_signal_handler);
    signal(SIGFPE,  xcm_signal_handler);
#ifndef _WIN32
    signal(SIGBUS,  xcm_signal_handler);
    signal(SIGPIPE, xcm_signal_handler);
#endif
}

// No-op on non-Apple: ObjC exceptions do not exist.
void xcm_install_objc_exception_handler() {}
