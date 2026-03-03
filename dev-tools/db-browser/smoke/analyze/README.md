# Database Browser Code Analysis System

Comprehensive analysis tool for evaluating code complexity, scalability, dependencies, performance, and technical debt in the Database Browser project.

## Features

- **Complexity Analysis**: Measures cyclomatic complexity, function length, nesting depth
- **Scalability Analysis**: Identifies patterns and potential bottlenecks
- **Dependency Analysis**: Maps dependencies, detects circular references, measures coupling
- **Performance Analysis**: Identifies performance anti-patterns and inefficiencies
- **Technical Debt Analysis**: Quantifies technical debt and generates remediation estimates
- **Memory Management Analysis**: Monitors allocations, detects leaks, identifies buffer risks

## Quick Start

```bash
# Run analysis with default settings
./run_analysis.sh

# Customize output locations
./run_analysis.sh --output my_results.json --report my_report.md

# Run quietly (suppress verbose output)
./run_analysis.sh --quiet
```

## Usage

### Basic Usage

```bash
python3 analyzer.py
```

### Advanced Options

```bash
python3 analyzer.py \
  --project-root ../.. \
  --output analysis_results.json \
  --report analysis_report.md \
  --quiet
```

### Options

- `--project-root PATH`: Path to project root (default: `../..`)
- `--output FILE`: JSON output file (default: `analysis_results.json`)
- `--report FILE`: Markdown report file (default: `analysis_report.md`)
- `--quiet`: Suppress verbose output

## Output Files

### analysis_results.json

Machine-readable JSON file containing:
- Project statistics
- Complexity metrics and issues
- Scalability patterns and issues
- Dependency graph and metrics
- Performance issues
- Technical debt items
- Overall scores and risk level

### analysis_report.md

Human-readable Markdown report with:
- Executive summary with scores
- Detailed breakdowns for each category
- Issue listings and hotspots
- Actionable recommendations

## Understanding Scores

Scores range from 0-100, where higher is better:

- **80-100**: Excellent - Low risk
- **60-79**: Good - Medium risk
- **40-59**: Needs Improvement - High risk
- **0-39**: Critical - Immediate attention required

## Analysis Categories

### 1. Complexity (20% weight)

Measures:
- Cyclomatic complexity
- Function length
- Nesting depth
- Parameter count

Issues flagged:
- Functions with complexity > 15
- Functions longer than 100 lines
- Nesting depth > 5 levels

### 2. Scalability (20% weight)

Analyzes:
- Architecture patterns (modular, layered)
- Scalability patterns (caching, pooling, async)
- Potential bottlenecks (nested loops, N+1 queries)
- Resource management

Issues flagged:
- Nested loops (O(n²) complexity)
- Missing pagination
- Unbounded allocations
- String concatenation in loops

### 3. Dependencies (15% weight)

Examines:
- Module coupling
- Dependency graphs
- Circular dependencies
- Hub files (high fan-in/fan-out)

Issues flagged:
- Circular dependencies
- High coupling (>10 dependencies)
- Tight coupling patterns

### 4. Performance (15% weight)

Identifies:
- Inefficient algorithms
- Memory allocation patterns
- I/O in loops
- Missing optimizations

Issues flagged:
- Allocations in loops
- Linear searches on large datasets
- Repeated strlen calls
- I/O operations in loops

### 5. Technical Debt (15% weight)

Tracks:
- TODO/FIXME comments
- Code duplication
- Missing documentation
- Deprecated functions

Estimates remediation time in hours.

### 6. Memory Management (15% weight)

Monitors:
- Memory allocation/deallocation balance
- Memory leak detection (malloc without free)
- Buffer overflow risks (unsafe functions)
- Unguarded allocations (missing NULL checks)
- Double-free vulnerabilities
- Large stack allocations

Detects patterns:
- Memory pools usage
- Arena allocation
- Reference counting
- Cleanup functions

Issues flagged:
- Memory leaks
- Unsafe string functions (strcpy, strcat, sprintf)
- Missing NULL checks after allocation
- Potential double-free issues
- Fixed-size buffer overuse
- Large stack buffers (>4KB)

## Integration

### CI/CD Integration

The analyzer returns appropriate exit codes:

- `0`: Success (LOW or MEDIUM risk)
- `1`: Failure (HIGH or CRITICAL risk)

Example CI workflow:

```bash
#!/bin/bash
cd dev-tools/db-browser/smoke/analyze
./run_analysis.sh
if [ $? -ne 0 ]; then
    echo "Code quality check failed!"
    exit 1
fi
```

### Pre-commit Hook

Add to `.git/hooks/pre-commit`:

```bash
#!/bin/bash
cd dev-tools/db-browser/smoke/analyze
python3 analyzer.py --quiet
```

## Customization

Each analyzer module can be customized by editing the corresponding Python file:

- `complexity_analyzer.py` - Adjust complexity thresholds
- `scalability_analyzer.py` - Add custom patterns
- `dependency_analyzer.py` - Configure coupling metrics
- `performance_analyzer.py` - Define performance checks
- `technical_debt_analyzer.py` - Set debt estimation rules
- `memory_analyzer.py` - Configure memory checks and patterns

## Requirements

- Python 3.6+
- No external dependencies (uses standard library only)

## Architecture

```
analyzer.py                    # Main orchestrator
├── complexity_analyzer.py     # Cyclomatic complexity, function metrics
├── scalability_analyzer.py    # Architecture patterns, bottlenecks
├── dependency_analyzer.py     # Coupling analysis, dependency graphs
├── performance_analyzer.py    # Performance anti-patterns
├── technical_debt_analyzer.py # Debt tracking and estimation
├── memory_analyzer.py         # Memory management and leak detection
└── report_generator.py        # Markdown report generation
```

## Example Report Excerpt

```markdown
## Executive Summary

**Overall Score:** 72.3/100  
**Risk Level:** ⚠️ MEDIUM

### Score Breakdown

| Category | Score | Status |
|----------|-------|--------|
| Complexity | 75.0/100 | Good |
| Scalability | 68.5/100 | Good |
| Dependencies | 82.0/100 | Excellent |
| Performance | 70.0/100 | Good |
| Technical Debt | 65.0/100 | Good |
| Memory Management | 77.0/100 | Good |
```

## Troubleshooting

### "No module named X"

Ensure you're running from the analyze directory:

```bash
cd dev-tools/db-browser/smoke/analyze
python3 analyzer.py
```

### Permission Denied

Make the script executable:

```bash
chmod +x analyzer.py run_analysis.sh
```

### Import Errors

Check that all analyzer modules are in the same directory.

## Contributing

To add new analysis features:

1. Create a new analyzer module (e.g., `security_analyzer.py`)
2. Implement the `analyze()` method
3. Add it to `analyzer.py` orchestrator
4. Update score calculation in `_generate_summary()`
5. Add report section to `report_generator.py`

## License

Same as parent project.
