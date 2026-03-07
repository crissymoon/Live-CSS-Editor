// main_priv.h -- Common includes for all src/main/ module files.
// Every main/*.mm includes this first.
#pragma once

#import <Cocoa/Cocoa.h>
#include <mach-o/dyld.h>
#include <OpenGL/gl3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "../app_state.h"
#include "../top-of-gui/chrome.h"
#include "../top-of-gui/native_chrome.h"
#include "../webview.h"
#include "../cmds-and-server/server_manager.h"
#include "../cmds-and-server/cmd_server.h"

#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/stat.h>

// Defined in fps_counter.cpp.
void fps_host_tick(AppState& st, double now_sec);
