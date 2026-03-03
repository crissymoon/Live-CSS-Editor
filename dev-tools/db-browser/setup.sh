#!/bin/bash
# Setup script for GTK+ Database Browser

echo "============================================================"
echo "  GTK+ Database Browser - Setup"
echo "============================================================"
echo ""

# Check OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "[INFO] macOS detected"
    PKG_MANAGER="brew"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "[INFO] Linux detected"
    PKG_MANAGER="apt-get"
else
    echo "[ERROR] Unsupported OS: $OSTYPE"
    exit 1
fi

# Check if GTK+3 is installed
echo ""
echo "[STEP 1] Checking GTK+3..."
if pkg-config --exists gtk+-3.0; then
    GTK_VERSION=$(pkg-config --modversion gtk+-3.0)
    echo "[OK] GTK+3 is installed (version $GTK_VERSION)"
else
    echo "[WARN] GTK+3 not found"
    echo ""
    read -p "Install GTK+3 now? (y/n): " install_gtk

    if [[ "$install_gtk" == "y" ]]; then
        echo ""
        echo "[INFO] Installing GTK+3..."

        if [[ "$PKG_MANAGER" == "brew" ]]; then
            brew install gtk+3
        elif [[ "$PKG_MANAGER" == "apt-get" ]]; then
            sudo apt-get update
            sudo apt-get install -y libgtk-3-dev
        fi

        if pkg-config --exists gtk+-3.0; then
            echo "[SUCCESS] GTK+3 installed"
        else
            echo "[ERROR] GTK+3 installation failed"
            exit 1
        fi
    else
        echo ""
        echo "[INFO] Skipping GTK+3 installation"
        echo ""
        echo "To install manually:"
        if [[ "$PKG_MANAGER" == "brew" ]]; then
            echo "  brew install gtk+3"
        else
            echo "  sudo apt-get install libgtk-3-dev"
        fi
        exit 0
    fi
fi

# Check SQLite
echo ""
echo "[STEP 2] Checking SQLite..."
if command -v sqlite3 &> /dev/null; then
    SQLITE_VERSION=$(sqlite3 --version | cut -d' ' -f1)
    echo "[OK] SQLite is installed (version $SQLITE_VERSION)"
else
    echo "[WARN] SQLite not found (should be system default)"
fi

# Compile
echo ""
echo "[STEP 3] Compiling database browser..."
make clean
make

if [ -f "build/bin/db-browser" ]; then
    echo ""
    echo "============================================================"
    echo "  Setup Complete - Crissy's DB Browser"
    echo "============================================================"
    echo ""
    echo "Binary: build/bin/db-browser"
    echo "Theme:  css/theme.css"
    echo ""
    echo "Usage:"
    echo "  ./build/bin/db-browser <database.db>"
    echo ""
    echo "============================================================"
else
    echo ""
    echo "[ERROR] Compilation failed"
    echo ""
    echo "Check the error messages above"
    exit 1
fi
