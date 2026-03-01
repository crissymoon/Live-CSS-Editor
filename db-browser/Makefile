# Crissy's DB Browser
# Makefile for building with GTK+3

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0` -lsqlite3

# Directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
CORE_SOURCES = core/db_manager.c
UI_SOURCES = ui/tooltips.c
MAIN_SOURCE = main.c

SOURCES = $(MAIN_SOURCE) $(CORE_SOURCES) $(UI_SOURCES)
OBJECTS = $(SOURCES:%.c=$(OBJ_DIR)/%.o)

# Binary
BINARY = $(BIN_DIR)/db-browser

# Default target
all: dirs $(BINARY)

# Create directories
dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)/core
	@mkdir -p $(OBJ_DIR)/ui
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
	@echo "Crissy's DB Browser built successfully: $(BINARY)"
	@echo "Theme copied to: $(BIN_DIR)/css/theme.css"

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Run
run: all
	./$(BINARY) ../data/linter.db

# Install
install: all
	install -d /usr/local/bin
	install -m 755 $(BINARY) /usr/local/bin/
	@echo "Installed to /usr/local/bin/db-browser"

# Uninstall
uninstall:
	rm -f /usr/local/bin/db-browser

# Help
help:
	@echo "Crissy's DB Browser Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build database browser (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  run       - Build and run with linter database"
	@echo "  install   - Install to system"
	@echo "  uninstall - Remove from system"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Requirements:"
	@echo "  - GTK+3: brew install gtk+3 (macOS) or apt-get install libgtk-3-dev (Linux)"
	@echo "  - SQLite3: brew install sqlite3 (macOS) or apt-get install libsqlite3-dev (Linux)"
	@echo ""
	@echo "Usage:"
	@echo "  ./build/bin/db-browser [database.db]"

.PHONY: all dirs clean run install uninstall help
