// xcm_paths.cpp -- executable-relative path resolution (pure C++, no ObjC).
#include "xcm_paths.h"

#include <cstdlib>
#include <sys/stat.h>

#if defined(__APPLE__)
#  include <mach-o/dyld.h>
#  include <unistd.h>
#elif defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  include <direct.h>
#else
#  include <unistd.h>
#  include <limits.h>
#endif

namespace xcm {

#if defined(_WIN32)
static const char kSep    = '\\';
static const char kAltSep = '/';
#else
static const char kSep    = '/';
static const char kAltSep = '\\';
#endif

static std::string dir_name(const std::string& path) {
    auto pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? std::string() : path.substr(0, pos);
}

static std::string file_join(const std::string& dir, const std::string& rel) {
    if (dir.empty()) return rel;
    if (!rel.empty() && (rel[0] == '/' || rel[0] == kAltSep)) return rel;
    return dir + kSep + rel;
}

bool file_exists(const std::string& path) {
    if (path.empty()) return false;
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && !S_ISDIR(st.st_mode);
}

bool dir_exists(const std::string& path) {
    if (path.empty()) return false;
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

static bool looks_like_project(const std::string& dir) {
    if (dir.empty()) return false;
    return file_exists(file_join(dir, "CMakeLists.txt")) && dir_exists(file_join(dir, "src"));
}

std::string exe_path() {
    char buf[4096] = {};
#if defined(__APPLE__)
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) return std::string();
#elif defined(_WIN32)
    if (GetModuleFileNameA(nullptr, buf, (DWORD)sizeof(buf)) == 0) return std::string();
#else
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return std::string();
    buf[n] = '\0';
#endif
    return std::string(buf);
}

std::string exe_dir() { return dir_name(exe_path()); }

static std::string walk_up_for_project(std::string dir) {
    // Eight levels is more than enough: MacOS -> Contents -> .app -> build -> root.
    for (int i = 0; i < 8 && !dir.empty(); ++i) {
        if (looks_like_project(dir)) return dir;
        std::string up = dir_name(dir);
        if (up == dir) break;
        dir = up;
    }
    return std::string();
}

std::string project_root() {
    const char* env = std::getenv("XCM_ROOT");
    if (env && *env && looks_like_project(env)) return std::string(env);

    std::string from_exe = walk_up_for_project(exe_dir());
    if (!from_exe.empty()) return from_exe;

    char cwd[4096] = {};
#if defined(_WIN32)
    if (_getcwd(cwd, sizeof(cwd)) != nullptr) {
#else
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
#endif
        std::string from_cwd = walk_up_for_project(cwd);
        if (!from_cwd.empty()) return from_cwd;
    }
    return exe_dir();
}

std::string asset_path(const std::string& rel) { return file_join(project_root(), rel); }

std::string first_existing(std::initializer_list<const char*> rels) {
    for (const char* rel : rels) {
        std::string p = asset_path(rel);
        if (file_exists(p)) return p;
    }
    return std::string();
}

std::string first_existing_dir(std::initializer_list<const char*> rels) {
    for (const char* rel : rels) {
        std::string p = asset_path(rel);
        if (dir_exists(p)) return p;
    }
    return std::string();
}

}  // namespace xcm
