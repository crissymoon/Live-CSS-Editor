#!/bin/bash
#
# Crissy's DB Browser - Comprehensive Build Script
# Validates theme, checks dependencies, compiles, and logs everything
#
# Usage: ./build-with-validation.sh [custom-css/theme.css]
#

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# Logging
LOG_DIR="logs"
LOG_FILE="${LOG_DIR}/build-$(date +%Y%m%d-%H%M%S).log"
THEME_FILE="css/theme.css"
CUSTOM_THEME=""

# Create logs directory
mkdir -p "$LOG_DIR"

# Function to log messages
log() {
    local level="$1"
    shift
    local message="$@"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] [$level] $message" | tee -a "$LOG_FILE"
}

# Function to print colored output
print_status() {
    local color="$1"
    local symbol="$2"
    local message="$3"
    echo -e "${color}${symbol}${NC} ${message}"
    log "INFO" "$message"
}

# Function to validate CSS theme
validate_theme() {
    local theme_file="$1"
    print_status "$CYAN" "→" "Validating theme: $theme_file"

    # Check if file exists
    if [ ! -f "$theme_file" ]; then
        print_status "$RED" "✗" "Theme file not found: $theme_file"
        log "ERROR" "Theme file not found: $theme_file"
        return 1
    fi

    # Check for invalid properties (GTK CSS doesn't support these)
    local invalid_props=("text-transform" "letter-spacing" "@media")
    local has_errors=0

    for prop in "${invalid_props[@]}"; do
        if grep -q "$prop" "$theme_file"; then
            print_status "$RED" "✗" "Invalid CSS property found: $prop (not supported in GTK CSS)"
            log "ERROR" "Invalid property '$prop' found in $theme_file"
            has_errors=1
        fi
    done

    # Check for border-radius (XCM standard: NO border-radius)
    local radius_count=$(grep -c "border-radius: [^0]" "$theme_file" || true)
    if [ "$radius_count" -gt 0 ]; then
        print_status "$YELLOW" "⚠" "Warning: Found $radius_count non-zero border-radius values (XCM standard requires 0)"
        log "WARN" "Non-zero border-radius found: $radius_count instances"
    fi

    # Check for required selectors
    local required_selectors=("window" "button" "label" "dialog")
    for selector in "${required_selectors[@]}"; do
        if ! grep -q "^${selector}" "$theme_file"; then
            print_status "$YELLOW" "⚠" "Warning: Required selector '$selector' not found"
            log "WARN" "Missing selector: $selector"
        fi
    done

    # Check file size (should be reasonable)
    local file_size=$(wc -c < "$theme_file")
    if [ "$file_size" -gt 100000 ]; then
        print_status "$YELLOW" "⚠" "Warning: Theme file is large (${file_size} bytes). Consider optimizing."
        log "WARN" "Large theme file: ${file_size} bytes"
    fi

    # Validate color codes
    local invalid_colors=$(grep -oE "#[0-9a-fA-F]{0,5}[^0-9a-fA-F]" "$theme_file" | grep -v "#[0-9a-fA-F]\{6\}" | wc -l || true)
    if [ "$invalid_colors" -gt 0 ]; then
        print_status "$YELLOW" "⚠" "Warning: Found potentially invalid color codes"
        log "WARN" "Invalid color codes detected"
    fi

    if [ "$has_errors" -eq 1 ]; then
        return 1
    fi

    print_status "$GREEN" "✓" "Theme validation passed"
    log "SUCCESS" "Theme validation completed"
    return 0
}

# Function to check dependencies
check_dependencies() {
    print_status "$CYAN" "→" "Checking build dependencies..."

    local missing_deps=()

    # Check for GCC
    if ! command -v gcc &> /dev/null; then
        missing_deps+=("gcc")
    else
        local gcc_version=$(gcc --version | head -n1)
        print_status "$GREEN" "✓" "Found $gcc_version"
    fi

    # Check for pkg-config
    if ! command -v pkg-config &> /dev/null; then
        missing_deps+=("pkg-config")
    else
        print_status "$GREEN" "✓" "Found pkg-config"
    fi

    # Check for GTK+3
    if ! pkg-config --exists gtk+-3.0; then
        missing_deps+=("gtk+3")
    else
        local gtk_version=$(pkg-config --modversion gtk+-3.0)
        print_status "$GREEN" "✓" "Found GTK+ 3: version $gtk_version"
        log "INFO" "GTK+ version: $gtk_version"
    fi

    # Check for SQLite3
    if ! command -v sqlite3 &> /dev/null; then
        missing_deps+=("sqlite3")
    else
        local sqlite_version=$(sqlite3 --version | awk '{print $1}')
        print_status "$GREEN" "✓" "Found SQLite3: version $sqlite_version"
    fi

    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_status "$RED" "✗" "Missing dependencies: ${missing_deps[*]}"
        log "ERROR" "Missing dependencies: ${missing_deps[*]}"
        echo ""
        echo "Install missing dependencies:"
        echo "  brew install ${missing_deps[*]}"
        return 1
    fi

    print_status "$GREEN" "✓" "All dependencies satisfied"
    log "SUCCESS" "Dependency check passed"
    return 0
}

# Function to check source files
check_sources() {
    print_status "$CYAN" "→" "Checking source files..."

    local required_files=(
        "main.c"
        "core/db_manager.c"
        "core/db_manager.h"
        "ui/tooltips.c"
        "ui/tooltips.h"
        "Makefile"
    )

    local missing_files=()

    for file in "${required_files[@]}"; do
        if [ ! -f "$file" ]; then
            missing_files+=("$file")
        fi
    done

    if [ ${#missing_files[@]} -ne 0 ]; then
        print_status "$RED" "✗" "Missing source files: ${missing_files[*]}"
        log "ERROR" "Missing files: ${missing_files[*]}"
        return 1
    fi

    print_status "$GREEN" "✓" "All source files present"
    log "SUCCESS" "Source file check passed"
    return 0
}

# Function to clean build
clean_build() {
    print_status "$CYAN" "→" "Cleaning previous build..."

    if [ -d "build" ]; then
        rm -rf build
        log "INFO" "Removed build directory"
    fi

    print_status "$GREEN" "✓" "Build cleaned"
}

# Function to compile
compile() {
    print_status "$CYAN" "→" "Compiling database browser..."

    # Run make and capture output
    if make 2>&1 | tee -a "$LOG_FILE"; then
        print_status "$GREEN" "✓" "Compilation successful"
        log "SUCCESS" "Compilation completed"

        # Check binary size
        if [ -f "build/bin/db-browser" ]; then
            local binary_size=$(ls -lh build/bin/db-browser | awk '{print $5}')
            print_status "$GREEN" "→" "Binary size: $binary_size"
            log "INFO" "Binary size: $binary_size"
        fi

        return 0
    else
        print_status "$RED" "✗" "Compilation failed"
        log "ERROR" "Compilation failed"
        return 1
    fi
}

# Function to copy theme
copy_theme() {
    local theme_src="$1"
    print_status "$CYAN" "->" "Copying theme to build directory..."

    if [ ! -d "build/bin/css" ]; then
        mkdir -p build/bin/css
        if [ $? -ne 0 ]; then
            print_status "$RED" "x" "Failed to create build/bin/css directory"
            log "ERROR" "mkdir -p build/bin/css failed"
            return 1
        fi
    fi

    cp "$theme_src" build/bin/css/theme.css
    if [ $? -ne 0 ]; then
        print_status "$RED" "x" "cp failed: $theme_src -> build/bin/css/theme.css"
        log "ERROR" "cp failed: $theme_src -> build/bin/css/theme.css"
        return 1
    fi

    if [ -f "build/bin/css/theme.css" ]; then
        local theme_size
        theme_size=$(ls -lh build/bin/css/theme.css | awk '{print $5}')
        print_status "$GREEN" "v" "Theme copied successfully ($theme_size)"
        log "SUCCESS" "Theme copied: $theme_size"
        return 0
    else
        print_status "$RED" "x" "Failed to copy theme"
        log "ERROR" "Theme copy failed"
        return 1
    fi
}

# Function to run tests
run_tests() {
    print_status "$CYAN" "->" "Running post-build validation..."

    # Check if binary is executable
    if [ ! -x "build/bin/db-browser" ]; then
        print_status "$RED" "x" "Binary is not executable"
        log "ERROR" "Binary not executable"
        return 1
    fi

    # Check if theme file exists in build at the correct path
    if [ ! -f "build/bin/css/theme.css" ]; then
        print_status "$RED" "x" "Theme file missing from build/bin/css/"
        log "ERROR" "Theme missing at build/bin/css/theme.css"
        return 1
    fi

    print_status "$GREEN" "v" "Post-build validation passed"
    log "SUCCESS" "Validation passed"
    return 0
}

# Function to print summary
print_summary() {
    echo ""
    echo "  Crissy's DB Browser - Build Complete"
    echo ""
    echo "  Binary  : build/bin/db-browser"
    echo "  Theme   : build/bin/css/theme.css"
    echo "  Log     : $LOG_FILE"
    echo ""
    echo "  Run with: ./build/bin/db-browser <database.db>"
    echo ""
}

# Main build process
main() {
    echo "Crissy's DB Browser - Build System"
    echo ""

    log "INFO" "Build started"
    log "INFO" "Working directory: $(pwd)"

    # Check for custom theme argument
    if [ $# -eq 1 ]; then
        CUSTOM_THEME="$1"
        THEME_FILE="$CUSTOM_THEME"
        print_status "$CYAN" "→" "Using custom theme: $CUSTOM_THEME"
        log "INFO" "Custom theme specified: $CUSTOM_THEME"
    fi

    # Run all checks
    if ! validate_theme "$THEME_FILE"; then
        print_status "$RED" "✗" "Theme validation failed. Fix errors and try again."
        log "ERROR" "Build aborted: Theme validation failed"
        exit 1
    fi

    if ! check_dependencies; then
        print_status "$RED" "✗" "Dependency check failed. Install missing dependencies."
        log "ERROR" "Build aborted: Missing dependencies"
        exit 1
    fi

    if ! check_sources; then
        print_status "$RED" "✗" "Source file check failed."
        log "ERROR" "Build aborted: Missing source files"
        exit 1
    fi

    # Build
    clean_build

    if ! compile; then
        print_status "$RED" "✗" "Build failed. Check logs for details."
        log "ERROR" "Build aborted: Compilation failed"
        exit 1
    fi

    if ! copy_theme "$THEME_FILE"; then
        print_status "$RED" "✗" "Failed to copy theme."
        log "ERROR" "Build aborted: Theme copy failed"
        exit 1
    fi

    if ! run_tests; then
        print_status "$RED" "✗" "Post-build validation failed."
        log "ERROR" "Build aborted: Validation failed"
        exit 1
    fi

    # Success!
    log "SUCCESS" "Build completed successfully"
    print_summary
}

# Run main
main "$@"
