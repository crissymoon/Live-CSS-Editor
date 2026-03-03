# Database Browser

A feature-rich SQLite database management application with GTK+3 UI.

## Features

### Core Capabilities
- SQLite database management with full CRUD operations
- SQL query execution with syntax highlighting
- Table and column operations
- Transaction management
- AES-256 encryption for sensitive databases
- Write-Ahead Logging (WAL) for improved performance
- Custom collation sequences (NOCASE_UTF8, NATURAL)

### Advanced Features
- **Search Helpers**: Tree traversal and hierarchical data operations
- **Table Versioning**: Schema change tracking and migration support
- **Trash Manager**: Recoverable delete operations
- **Query Builder**: Visual SQL query construction
- **Function Library**: 50+ built-in SQL functions

### Memory Management
- Comprehensive memory analysis system
- Leak detection and prevention
- Buffer overflow protection
- Safe allocation patterns

### Analysis System
- Code complexity analysis
- Scalability assessment
- Performance profiling
- Technical debt tracking
- Memory management scoring

## Recent Improvements

### Technical Debt Reduction
Added two new subsystems to improve code maintainability:

1. **Search Helpers** (core/search_helpers.h/c)
   - Tree operations for hierarchical data
   - Recursive ancestor/descendant queries
   - Path generation and sibling discovery
   - Query optimization and index suggestions
   - 597 lines of well-documented code

2. **Table Versioning** (core/table_versioning.h/c)
   - Schema change tracking
   - Version history management
   - Migration support with rollback
   - Schema comparison and diff
   - 659 lines of code with full API coverage

See [TECH_DEBT_IMPROVEMENTS.md](TECH_DEBT_IMPROVEMENTS.md) for details.

### Memory Safety Improvements
- Fixed 15+ unguarded allocations
- Replaced unsafe string functions (sprintf, strcat)
- Moved large stack buffers to heap
- Memory score improved from 0.0 to 64.0/100

See [OPTIMIZATION_SUMMARY.md](OPTIMIZATION_SUMMARY.md) for complete details.

## Building

### Prerequisites
- GCC or Clang
- GTK+3 development libraries
- SQLite3 development libraries
- pkg-config

### macOS
```bash
brew install gtk+3 sqlite3 pkg-config
```

### Ubuntu/Debian
```bash
sudo apt-get install libgtk-3-dev libsqlite3-dev pkg-config
```

### Compile
```bash
cd dev-tools/db-browser
make clean
make
```

### Run
```bash
./build/bin/db-browser
```

## Project Structure

```
dev-tools/db-browser/
├── core/                   # Core database functionality
│   ├── db_manager.c/h     # Database connection and operations
│   ├── db_crypto.c/h      # AES encryption/decryption
│   ├── db_transfer.c/h    # Import/export operations
│   ├── db_optimization.c/h # WAL and performance tuning
│   ├── search_helpers.c/h  # Tree traversal and queries
│   └── table_versioning.c/h # Schema version tracking
├── modules/               # Feature modules
│   ├── trash_manager.c/h  # Recoverable deletes
│   ├── row_operations.c/h # Add/delete rows
│   ├── auth.c/h          # Authentication
│   └── functions/        # SQL function library
├── ui/                   # User interface
│   └── tooltips.c/h      # Context-sensitive help
├── smoke/                # Analysis system
│   └── analyze/         # Code quality tools
├── examples/            # Usage examples
└── Makefile            # Build configuration
```

## Analysis System

Run comprehensive code analysis:

```bash
python3 smoke/analyze/analyzer.py
```

This generates:
- `smoke/analyze/analysis_results.json` - Detailed metrics
- `smoke/analyze/analysis_report.md` - Human-readable report

### Analysis Modules
1. **Complexity** - Cyclomatic complexity, function length
2. **Scalability** - Architecture patterns, bottlenecks
3. **Dependencies** - Coupling analysis
4. **Performance** - Query optimization, algorithm efficiency
5. **Technical Debt** - Documentation gaps, long functions
6. **Memory** - Leak detection, buffer safety

## Demo Programs

### Search Helpers & Versioning
```bash
cd dev-tools/db-browser
gcc -o examples/demo examples/search_and_versioning_demo.c \
    core/search_helpers.c core/table_versioning.c -lsqlite3 -I.
./examples/demo
```

## Documentation

- [SEARCH_AND_VERSIONING.md](SEARCH_AND_VERSIONING.md) - New feature documentation
- [TECH_DEBT_IMPROVEMENTS.md](TECH_DEBT_IMPROVEMENTS.md) - Technical debt reduction
- [OPTIMIZATION_SUMMARY.md](OPTIMIZATION_SUMMARY.md) - Database optimizations

## Current Scores

```
Overall Score: 64.2/100 (MEDIUM risk)

Category Breakdown:
  Complexity:     100.0/100 (Excellent)
  Scalability:     50.0/100 (Needs Improvement)
  Dependencies:   100.0/100 (Excellent)
  Performance:     44.0/100 (Needs Improvement)
  Technical Debt:  20.0/100 (Critical)
  Memory:          64.0/100 (Good)
```

## Contributing

When adding new features:
1. Follow existing code style
2. Add NULL checks for all allocations
3. Use safe string functions (snprintf, not sprintf)
4. Document public APIs in header files
5. Include usage examples
6. Run analysis before committing

## License

See [LICENSE](LICENSE) for details.