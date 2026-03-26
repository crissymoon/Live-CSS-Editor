#!/bin/bash

REPO_URL="https://github.com/crissymoon/Live-CSS-Editor.git"

# Ensure we are in the project directory
cd "$(dirname "$0")"

echo "--- push.sh: working in $PWD"
echo "--- push.sh: remote $REPO_URL"

# Create .gitignore if it doesn't exist
if [ ! -f .gitignore ]; then
    cat > .gitignore << 'EOF'
# Environment and secrets
.env
.env.*
*.env
secrets.*
config.local.*

# macOS
.DS_Store
.AppleDouble
.LSOverride

# Editor/IDE directories
.vscode/
.idea/
*.suo
*.ntvs*
*.njsproj
*.sln

# Logs and temp files
*.log
*.tmp
*.swp
*.swo
*~

# PHP cache
/vendor/

# Build output
/dist/
/build/

# Python virtual environments
dev-browser/venv/
dev-browser/.venv/
.venv/

# Python compiled files
__pycache__/
*.pyc
*.pyo
*.pyd

# Tauri build output
**/src-tauri/target/
**/src-tauri/www/
**/src-tauri/gen/
EOF
    echo ".gitignore created."
fi

# Initialize git if not already a repo
if [ ! -d .git ]; then
    git init
    echo "Git repository initialized."
fi

# Set or update remote
if git remote get-url origin &>/dev/null; then
    git remote set-url origin "$REPO_URL"
    echo "Remote 'origin' updated to $REPO_URL"
else
    git remote add origin "$REPO_URL"
    echo "Remote 'origin' added: $REPO_URL"
fi

echo "--- git remote -v:"
git remote -v

# Regenerate README.md before staging
if [ -f make_readme.py ]; then
    echo "--- Updating README.md..."
    python3 make_readme.py
fi

# Stage all files
echo "--- Running: git add ."
add_out="$(git add . 2>&1)"
add_rc=$?
if [ -n "$add_out" ]; then
    echo "$add_out"
fi
if [ $add_rc -ne 0 ]; then
    echo "ERROR: git add failed (exit $add_rc)" >&2
    exit 1
fi
echo "--- git add complete"

# Show what is staged
echo "--- Staged files:"
git diff --cached --name-status 2>/dev/null || true

# Commit (skip if nothing to commit)
if git diff --cached --quiet; then
    echo "--- Nothing new to commit. Working tree clean."
else
    echo "--- Running: git commit"
    git commit -m "Update Live CSS Editor"
    if [ $? -ne 0 ]; then
        echo "ERROR: git commit failed" >&2
        exit 1
    fi
fi

# Push using the current branch name
BRANCH="$(git rev-parse --abbrev-ref HEAD)"
echo "--- Running: git push -v -u origin $BRANCH"
push_out="$(git push -v -u origin "$BRANCH" 2>&1)"
push_rc=$?
echo "$push_out"
if [ $push_rc -ne 0 ]; then
    echo "ERROR: git push failed (exit $push_rc)" >&2
    exit 1
fi

echo "--- Done: pushed to $REPO_URL ($BRANCH)."
