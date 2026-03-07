// main_crash.h -- Crash/signal handler declarations.
#pragma once

extern char g_debug_dir[1024];
extern char g_crash_log[1024];

void xcm_write_crash(const char* header, const char* detail);
void xcm_install_signal_handlers();
void xcm_install_objc_exception_handler();
