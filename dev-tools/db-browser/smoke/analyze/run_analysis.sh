#!/bin/bash
# Quick analysis runner for Database Browser

cd "$(dirname "$0")"

echo "=========================================="
echo "Database Browser Code Analysis"
echo "=========================================="
echo ""

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 is required but not found."
    exit 1
fi

# Make analyzer executable
chmod +x analyzer.py

# Run analysis
python3 analyzer.py "$@"

exit_code=$?

# Run gap analysis
python3 recheck_values.py

if [ $exit_code -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "Analysis completed successfully!"
    echo "=========================================="
    echo ""
    echo "Results saved to:"
    echo "  - analysis_results.json (machine-readable)"
    echo "  - analysis_report.md (human-readable)"
    echo ""
else
    echo ""
    echo "=========================================="
    echo "Analysis completed with warnings/errors"
    echo "=========================================="
fi

exit $exit_code
