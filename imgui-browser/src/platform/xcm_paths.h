// xcm_paths.h -- locating files relative to the running app.
//
// Nothing in this app may hardcode a home directory.  The binary can live in
// three different places depending on how it was built and shipped:
//
//   <root>/build/imgui_browser                                  (flat dev build)
//   <root>/build/imgui_browser.app/Contents/MacOS/imgui_browser (bundle dev build)
//   /Applications/XCM.app/Contents/MacOS/imgui_browser          (installed app)
//
// project_root() walks up from the executable until it finds the source tree,
// so every asset lookup works from all three layouts.
#pragma once
#include <initializer_list>
#include <string>

namespace xcm {

// Absolute path of the running executable ("" if it cannot be determined).
std::string exe_path();

// Directory holding the executable.
std::string exe_dir();

// Root of the imgui-browser project (the directory with CMakeLists.txt + src/).
// Resolution order: $XCM_ROOT, walk up from the executable, walk up from the
// current working directory.  Falls back to exe_dir() when nothing matches.
std::string project_root();

// project_root()/rel, without checking existence.
std::string asset_path(const std::string& rel);

// First candidate that exists as a readable file, or "" when none do.
// Candidates are resolved with asset_path().
std::string first_existing(std::initializer_list<const char*> rels);

// Same as first_existing(), but requires the candidate to be a directory.
std::string first_existing_dir(std::initializer_list<const char*> rels);

// Readable file test.
bool file_exists(const std::string& path);

// Directory test.
bool dir_exists(const std::string& path);

}  // namespace xcm
