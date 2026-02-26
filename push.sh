#!/bin/bash
set -e

REPO_URL="https://github.com/crissymoon/Live-CSS-Editor.git"
BRANCH="main"

# Ensure we are in the project directory
cd "$(dirname "$0")"

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
    echo "Remote 'origin' updated."
else
    git remote add origin "$REPO_URL"
    echo "Remote 'origin' added."
fi

# Stage all files
git add .

# Commit (skip if nothing to commit)
if git diff --cached --quiet; then
    echo "Nothing new to commit. Working tree clean."
else
    git commit -m "Update Live CSS Editor"
fi

# Push
git push -u origin "$BRANCH"

echo "Pushed to $REPO_URL ($BRANCH)."
