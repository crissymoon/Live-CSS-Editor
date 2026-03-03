#!/bin/bash
# Enable WAL mode for all databases in the workspace

echo "Enabling WAL mode for workspace databases..."
echo ""

count=0
failed=0

# Find all .db files excluding build/vendor/node_modules
while IFS= read -r db_file; do
    if [ -f "$db_file" ]; then
        echo -n "Processing: $db_file ... "
        
        # Check if database is accessible
        if sqlite3 "$db_file" "PRAGMA journal_mode=WAL;" > /dev/null 2>&1; then
            mode=$(sqlite3 "$db_file" "PRAGMA journal_mode;")
            if [ "$mode" = "wal" ]; then
                echo "OK (WAL enabled)"
                ((count++))
            else
                echo "FAILED (mode: $mode)"
                ((failed++))
            fi
        else
            echo "ERROR (database locked or corrupted)"
            ((failed++))
        fi
    fi
done < <(find . -name "*.db" -type f \
    ! -path "*/build/*" \
    ! -path "*/vendor/*" \
    ! -path "*/node_modules/*" \
    ! -path "*/.git/*")

echo ""
echo "Summary:"
echo "  Successfully enabled WAL: $count databases"
if [ $failed -gt 0 ]; then
    echo "  Failed: $failed databases"
fi
