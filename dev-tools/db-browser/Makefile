# Crissy's DB Browser
# Makefile for building with GTK+3

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0` -lsqlite3 -lm

# Platform detection
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        PLATFORM = macos
    else
        PLATFORM = linux
    endif
endif

ifeq ($(PLATFORM),macos)
    LDFLAGS += -framework Security -framework CoreFoundation
endif

ifeq ($(PLATFORM),windows)
    BINARY_EXT = .exe
    LDFLAGS += -lssl -lcrypto -lbcrypt
else
    BINARY_EXT =
    ifneq ($(PLATFORM),macos)
        LDFLAGS += -lssl -lcrypto
    endif
endif

# Directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
CORE_SOURCES = core/db_manager.c \
               core/db_crypto.c \
               core/db_transfer.c \
               core/db_optimization.c
UI_SOURCES = ui/tooltips.c
MODULE_SOURCES = modules/app_state.c \
                 modules/ui_drawing.c \
                 modules/ui_utils.c \
                 modules/theme_manager.c \
                 modules/recent_manager.c \
                 modules/ui_panels.c \
                 modules/db_callbacks.c \
                 modules/query_callbacks.c \
                 modules/table_callbacks.c \
                 modules/csv_handler.c \
                 modules/misc_callbacks.c \
                 modules/data_protection.c \
                 modules/auth.c \
                 modules/trash_manager.c \
                 modules/row_operations.c \
                 modules/function_runner.c \
                 modules/functions/aggregate_functions.c \
                 modules/functions/statistical_functions.c \
                 modules/functions/mathematical_functions.c \
                 modules/functions/datetime_functions.c \
                 modules/functions/string_functions.c \
                 modules/functions/window_functions.c \
                 modules/functions/business_functions.c \
                 modules/functions/function_categories.c
MAIN_SOURCE = main.c

SOURCES = $(MAIN_SOURCE) $(CORE_SOURCES) $(UI_SOURCES) $(MODULE_SOURCES)
OBJECTS = $(SOURCES:%.c=$(OBJ_DIR)/%.o)

# Binary
BINARY = $(BIN_DIR)/db-browser$(BINARY_EXT)

# Default target
all: dirs $(BINARY)

# Create directories
dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)/core
	@mkdir -p $(OBJ_DIR)/ui
	@mkdir -p $(OBJ_DIR)/modules
	@mkdir -p $(OBJ_DIR)/modules/functions
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(BIN_DIR)/css

# Compile object files
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link binary
$(BINARY): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@cp css/theme.css $(BIN_DIR)/css/
	@cp css/theme-simple.css $(BIN_DIR)/css/
	@echo "Crissy's DB Browser built successfully: $(BINARY)"
	@echo "Themes copied to: $(BIN_DIR)/css/"

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Run
run: all
	./$(BINARY) ../data/linter.db

# Install (Unix/macOS only)
install: all
ifeq ($(PLATFORM),windows)
	@echo "On Windows, copy $(BINARY) to your preferred location manually."
else
	install -d /usr/local/bin
	install -m 755 $(BINARY) /usr/local/bin/
	@echo "Installed to /usr/local/bin/db-browser"
endif

# Uninstall (Unix/macOS only)
uninstall:
ifeq ($(PLATFORM),windows)
	@echo "On Windows, delete the binary manually."
else
	rm -f /usr/local/bin/db-browser
endif

# Help
help:
	@echo "Crissy's DB Browser Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build database browser (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  run       - Build and run with linter database"
	@echo "  install   - Install to system (Unix/macOS)"
	@echo "  uninstall - Remove from system (Unix/macOS)"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Requirements:"
	@echo "  - macOS:   brew install gtk+3 sqlite3"
	@echo "  - Linux:   apt-get install libgtk-3-dev libsqlite3-dev libssl-dev"
	@echo "  - Windows: MSYS2 + pacman -S mingw-w64-x86_64-{toolchain,gtk3,sqlite3,openssl,pkg-config}"
	@echo "             Run setup.ps1 in PowerShell for guided setup"
	@echo ""
	@echo "Usage:"
	@echo "  ./build/bin/db-browser [database.db]"

.PHONY: all dirs clean run install uninstall help
