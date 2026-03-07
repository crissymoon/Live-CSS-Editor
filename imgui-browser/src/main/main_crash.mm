// main_crash.mm -- Crash/signal handling implementation.
// g_debug_dir and g_crash_log are set once at startup by main() before any
// signal can fire.  xcm_write_crash is async-signal-safe enough for a best-
// effort log (fopen is not, but we accept the risk for diagnostics).

#include "main_crash.h"
#import <Foundation/Foundation.h>
#include <cstdio>
#include <ctime>
#include <signal.h>
#include <execinfo.h>
#include <cstdlib>

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
    void* frames[64];
    int   nframes = backtrace(frames, 64);
    char** syms   = backtrace_symbols(frames, nframes);
    fprintf(f, "--- backtrace (%d frames) ---\n", nframes);
    for (int i = 0; i < nframes; i++)
        fprintf(f, "  %s\n", syms ? syms[i] : "(unavailable)");
    if (syms) free(syms);
    fprintf(f, "========================================\n");
    fclose(f);
}

static void xcm_signal_handler(int sig) {
    const char* name = "UNKNOWN";
    switch (sig) {
        case SIGABRT: name = "SIGABRT (abort/assertion)"; break;
        case SIGSEGV: name = "SIGSEGV (segmentation fault)"; break;
        case SIGBUS:  name = "SIGBUS (bus error)"; break;
        case SIGILL:  name = "SIGILL (illegal instruction)"; break;
        case SIGFPE:  name = "SIGFPE (floating point exception)"; break;
        case SIGPIPE: name = "SIGPIPE (broken pipe -- ignored)"; return;
    }
    xcm_write_crash(name, "see backtrace above");
    signal(sig, SIG_DFL);
    raise(sig);
}

static void xcm_objc_exception_handler(NSException* e) {
    NSString* msg = [NSString stringWithFormat:@"name=%@ reason=%@\nstack=\n%@",
                     e.name, e.reason,
                     e.callStackSymbols
                         ? [e.callStackSymbols componentsJoinedByString:@"\n"]
                         : @"(none)"];
    xcm_write_crash("Uncaught ObjC exception", msg.UTF8String);
}

void xcm_install_signal_handlers() {
    signal(SIGABRT, xcm_signal_handler);
    signal(SIGSEGV, xcm_signal_handler);
    signal(SIGBUS,  xcm_signal_handler);
    signal(SIGILL,  xcm_signal_handler);
    signal(SIGFPE,  xcm_signal_handler);
    signal(SIGPIPE, xcm_signal_handler);  // returns without raising
}

void xcm_install_objc_exception_handler() {
    NSSetUncaughtExceptionHandler(xcm_objc_exception_handler);
}
