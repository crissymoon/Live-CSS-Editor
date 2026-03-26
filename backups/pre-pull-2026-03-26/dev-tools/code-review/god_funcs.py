#!/usr/bin/env python3
"""
God Functions Scanner
Scans the entire live-css project for god functions across multiple languages.

A "god function" is a function that violates multiple complexity thresholds:
  - Lines > 150
  - Cyclomatic complexity > 20
  - Nesting depth > 4
  - Parameters > 5

Functions hitting 2+ thresholds are flagged as god functions.
Functions hitting 3+ thresholds are flagged as CRITICAL.

Usage:
    python3 god_funcs.py [--json] [--verbose]
"""

import os
import re
import sys
import json
from pathlib import Path
from typing import Dict, List, Any, Tuple

from scan_config import merge_skip_dir_names, merge_skip_file_names, merge_skip_relative_paths, should_skip_relative_path

# Directories to skip
# Note: db-broswer is checked seperately - there is a smoke fiolder in it - ck w/this: /Users/mac/Documents/live-css/dev-tools/db-browser/smoke/analyze/run_analysis.sh
BASE_SKIP_DIRS = {
    'dev-browser', 'node_modules', 'vendor', 'build', '.git', '__pycache__',
    '.venv', 'venv', 'env', 'WidevineCdm', 'widevine', 'legacy', 'smoke', 'email_smoke', 'db-browser', 'test', 'tests', 'docs', 'examples', 'scripts', 'multi_test', 'zyx_planning_and_visuals'
}

# Individual files to skip (basenames)
BASE_SKIP_FILES = {
    'god_funcs.py', 'test_risk_detector.py', 'lines_count.py', 'lines_count_test.py', 'complexity_count.py', 'complexity_count_test.py', 'nesting_count.py', 'nesting_count_test.py', 'params_count.py', 'params_count_test.py', 'run_god_scan.sh', 'test_risk_detector.py', 'test_god_funcs.py', 'god_funcs_test.py', 'test_accuracy.py', 'accuracy_test.py'
}

# File extensions to scan
# Skip the GO XCM-AUTH (.go) - handle seperate
SCAN_EXTENSIONS = {'.c', '.php', '.py', '.js'}


class GodFunctionScanner:
    """Scans for god functions across multiple languages."""
    
    # Thresholds for god function detection
    MAX_LINES = 150
    MAX_COMPLEXITY = 20
    MAX_NESTING = 4
    MAX_PARAMS = 5
    
    def __init__(self, root_path: str):
        self.root = Path(root_path)
        self.results = {
            'god_functions': [],
            'summary': {
                'total_files_scanned': 0,
                'total_functions_analyzed': 0,
                'god_function_count': 0,
                'critical_count': 0,
                'high_count': 0,
                'by_language': {}
            }
        }
    
    def scan(self) -> Dict[str, Any]:
        """Scan the entire project for god functions."""
        for file_path in self._get_files():
            ext = file_path.suffix.lower()
            
            if ext == '.c':
                self._scan_c_file(file_path)
            elif ext == '.php':
                self._scan_php_file(file_path)
            elif ext == '.py':
                self._scan_python_file(file_path)
            elif ext == '.js':
                self._scan_js_file(file_path)
            
            self.results['summary']['total_files_scanned'] += 1
        
        # Update summary counts
        self.results['summary']['god_function_count'] = len(self.results['god_functions'])
        self.results['summary']['critical_count'] = sum(
            1 for g in self.results['god_functions'] if g['severity'] == 'CRITICAL'
        )
        self.results['summary']['high_count'] = sum(
            1 for g in self.results['god_functions'] if g['severity'] == 'HIGH'
        )
        
        return self.results
    
    def _get_files(self):
        """Yield all scannable files in the project."""
        skip_dir_names = merge_skip_dir_names(BASE_SKIP_DIRS)
        skip_relative_paths = merge_skip_relative_paths()
        skip_file_names = merge_skip_file_names(BASE_SKIP_FILES)
        for root, dirs, files in os.walk(self.root):
            root_path = Path(root)
            # Filter out skip directories
            dirs[:] = [
                d for d in dirs
                if d not in skip_dir_names
                and not should_skip_relative_path(root_path / d, self.root, skip_relative_paths)
            ]

            for fname in files:
                if fname in skip_file_names:
                    continue
                fpath = Path(root) / fname
                if fpath.suffix.lower() in SCAN_EXTENSIONS:
                    yield fpath
    
    def _check_god_function(self, file_path: Path, func_name: str,
                            lines: int, complexity: int, nesting: int,
                            params: int, lang: str):
        """Check if a function is a god function and record it."""
        self.results['summary']['total_functions_analyzed'] += 1
        
        hits = sum([
            lines > self.MAX_LINES,
            complexity > self.MAX_COMPLEXITY,
            nesting > self.MAX_NESTING,
            params > self.MAX_PARAMS,
        ])
        
        if hits >= 2:
            reasons = []
            if lines > self.MAX_LINES:
                reasons.append(f"{lines} lines (>{self.MAX_LINES})")
            if complexity > self.MAX_COMPLEXITY:
                reasons.append(f"complexity {complexity} (>{self.MAX_COMPLEXITY})")
            if nesting > self.MAX_NESTING:
                reasons.append(f"nesting {nesting} (>{self.MAX_NESTING})")
            if params > self.MAX_PARAMS:
                reasons.append(f"{params} params (>{self.MAX_PARAMS})")
            
            rel_path = str(file_path.relative_to(self.root))
            severity = 'CRITICAL' if hits >= 3 else 'HIGH'
            
            self.results['god_functions'].append({
                'file': rel_path,
                'function': func_name,
                'language': lang,
                'severity': severity,
                'hits': hits,
                'lines': lines,
                'complexity': complexity,
                'nesting': nesting,
                'params': params,
                'reasons': reasons
            })
            
            # Update by-language counter
            if lang not in self.results['summary']['by_language']:
                self.results['summary']['by_language'][lang] = 0
            self.results['summary']['by_language'][lang] += 1
    
    # --- C Language ---
    
    def _scan_c_file(self, file_path: Path):
        """Scan a C file for god functions."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            functions = self._extract_c_functions(content)
            for func_name, func_body in functions:
                stripped = self._strip_comments_c(func_body)
                lines = len([l for l in stripped.splitlines() if l.strip()])
                complexity = self._calc_complexity_c(stripped)
                nesting = self._calc_nesting(stripped)
                params = self._count_params_c(func_body)
                
                self._check_god_function(file_path, func_name, lines,
                                         complexity, nesting, params, 'C')
        except Exception:
            pass
    
    def _extract_c_functions(self, content: str) -> List[Tuple[str, str]]:
        """Extract function name and body from C code."""
        functions = []
        # Match: type name(params) { ... }
        pattern = r'(?:static\s+)?(?:inline\s+)?[\w\*\s]+\s+(\w+)\s*\([^)]*\)\s*\{'
        
        for match in re.finditer(pattern, content):
            func_name = match.group(1)
            # Skip common non-functions
            if func_name in ('if', 'while', 'for', 'switch', 'else'):
                continue

            brace_start = match.end() - 1
            body = self._extract_brace_block(content, brace_start)
            if body:
                # Prepend signature so param-counting methods can find it
                full_text = content[match.start():brace_start] + body
                functions.append((func_name, full_text))

        return functions

    def _calc_complexity_c(self, code: str) -> int:
        """Calculate cyclomatic complexity for C code."""
        complexity = 1
        # Count decision points
        patterns = [
            r'\bif\s*\(',
            r'\belse\s+if\s*\(',
            r'\bfor\s*\(',
            r'\bwhile\s*\(',
            r'\bcase\s+[^:]+:',
            r'\bcatch\s*\(',
            r'\?\s*[^:]+:',  # ternary
            r'&&',
            r'\|\|',
        ]
        for p in patterns:
            complexity += len(re.findall(p, code))
        return complexity
    
    def _count_params_c(self, func_body: str) -> int:
        """Count parameters in C function (from first line)."""
        first_line = func_body.split('{')[0] if '{' in func_body else func_body
        match = re.search(r'\(([^)]*)\)', first_line)
        if match:
            params = match.group(1).strip()
            if not params or params == 'void':
                return 0
            return len([p for p in params.split(',') if p.strip()])
        return 0
    
    # --- PHP Language ---
    
    def _scan_php_file(self, file_path: Path):
        """Scan a PHP file for god functions."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            functions = self._extract_php_functions(content)
            for func_name, func_body in functions:
                stripped = self._strip_comments_php(func_body)
                lines = len([l for l in stripped.splitlines() if l.strip()])
                complexity = self._calc_complexity_php(stripped)
                nesting = self._calc_nesting(stripped)
                params = self._count_params_php(func_body)
                
                self._check_god_function(file_path, func_name, lines,
                                         complexity, nesting, params, 'PHP')
        except Exception:
            pass
    
    def _extract_php_functions(self, content: str) -> List[Tuple[str, str]]:
        """Extract function/method definitions from PHP code."""
        functions = []
        # function name(...) { or public/private/protected function name(...)
        pattern = r'(?:public|private|protected|static|\s)+function\s+(\w+)\s*\([^)]*\)\s*(?::\s*\??\w+)?\s*\{'
        
        for match in re.finditer(pattern, content):
            func_name = match.group(1)
            brace_start = match.end() - 1
            body = self._extract_brace_block(content, brace_start)
            if body:
                # Prepend signature so param-counting methods can find it
                full_text = content[match.start():brace_start] + body
                functions.append((func_name, full_text))

        return functions

    def _calc_complexity_php(self, code: str) -> int:
        """Calculate cyclomatic complexity for PHP code."""
        complexity = 1
        patterns = [
            r'\bif\s*\(',
            r'\belseif\s*\(',
            r'\belse\s+if\s*\(',
            r'\bfor\s*\(',
            r'\bforeach\s*\(',
            r'\bwhile\s*\(',
            r'\bcase\s+[^:]+:',
            r'\bcatch\s*\(',
            r'\?\s*[^:]+:',
            r'&&',
            r'\|\|',
            r'\band\b',
            r'\bor\b',
        ]
        for p in patterns:
            complexity += len(re.findall(p, code))
        return complexity
    
    def _count_params_php(self, func_body: str) -> int:
        """Count parameters in PHP function."""
        match = re.search(r'function\s+\w+\s*\(([^)]*)\)', func_body)
        if match:
            params = match.group(1).strip()
            if not params:
                return 0
            return len([p for p in params.split(',') if p.strip()])
        return 0
    
    # --- Python Language ---
    
    def _scan_python_file(self, file_path: Path):
        """Scan a Python file for god functions."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            functions = self._extract_python_functions(content)
            for func_name, func_body in functions:
                stripped = self._strip_comments_python(func_body)
                lines = len([l for l in stripped.splitlines() if l.strip()])
                complexity = self._calc_complexity_python(stripped)
                nesting = self._calc_nesting_python(stripped)
                params = self._count_params_python(func_body)
                
                self._check_god_function(file_path, func_name, lines,
                                         complexity, nesting, params, 'Python')
        except Exception:
            pass
    
    def _extract_python_functions(self, content: str) -> List[Tuple[str, str]]:
        """Extract function definitions from Python code."""
        functions = []
        lines = content.splitlines()
        i = 0
        
        while i < len(lines):
            line = lines[i]
            match = re.match(r'^(\s*)def\s+(\w+)\s*\(', line)
            if match:
                indent = len(match.group(1))
                func_name = match.group(2)
                func_lines = [line]
                i += 1
                
                # Collect function body (strictly deeper indent only)
                while i < len(lines):
                    next_line = lines[i]
                    if next_line.strip() == '':
                        func_lines.append(next_line)
                        i += 1
                        continue
                    next_indent = len(next_line) - len(next_line.lstrip())
                    if next_indent > indent:
                        func_lines.append(next_line)
                        i += 1
                    else:
                        break
                
                functions.append((func_name, '\n'.join(func_lines)))
            else:
                i += 1
        
        return functions
    
    def _calc_complexity_python(self, code: str) -> int:
        """Calculate cyclomatic complexity for Python code."""
        complexity = 1
        patterns = [
            r'\bif\s+',
            r'\belif\s+',
            r'\bfor\s+',
            r'\bwhile\s+',
            r'\bexcept\s*[:\(]',
            r'\band\b',
            r'\bor\b',
            # Note: inline ternary "x if c else y" is already counted by \bif\s+ above;
            # a separate ternary pattern would double-count every regular if.
        ]
        for p in patterns:
            complexity += len(re.findall(p, code))
        return complexity
    
    def _calc_nesting_python(self, code: str) -> int:
        """Calculate max nesting for Python using a stack (indent-unit agnostic)."""
        max_nesting = 0
        # Stack of indent column values; seeded on the first non-blank line (def line).
        indent_stack: list = []

        for line in code.splitlines():
            if not line.strip():
                continue
            indent = len(line) - len(line.lstrip())
            if not indent_stack:
                indent_stack.append(indent)   # base level is the def line
                continue
            if indent > indent_stack[-1]:
                indent_stack.append(indent)
            else:
                # Pop back to the matching or enclosing level
                while len(indent_stack) > 1 and indent < indent_stack[-1]:
                    indent_stack.pop()
            # Depth relative to the function's own line
            nesting = len(indent_stack) - 1
            max_nesting = max(max_nesting, nesting)

        return max_nesting
    
    def _count_params_python(self, func_body: str) -> int:
        """Count parameters in Python function."""
        match = re.search(r'def\s+\w+\s*\(([^)]*)\)', func_body)
        if match:
            params = match.group(1).strip()
            if not params:
                return 0
            # Filter out self, cls, *args, **kwargs
            param_list = [p.strip().split('=')[0].strip() for p in params.split(',')]
            param_list = [p for p in param_list if p and p not in ('self', 'cls') and not p.startswith('*')]
            return len(param_list)
        return 0
    
    # --- JavaScript Language ---
    
    def _scan_js_file(self, file_path: Path):
        """Scan a JavaScript file for god functions."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            functions = self._extract_js_functions(content)
            for func_name, func_body in functions:
                stripped = self._strip_comments_js(func_body)
                lines = len([l for l in stripped.splitlines() if l.strip()])
                complexity = self._calc_complexity_js(stripped)
                nesting = self._calc_nesting(stripped)
                params = self._count_params_js(func_body)
                
                self._check_god_function(file_path, func_name, lines,
                                         complexity, nesting, params, 'JavaScript')
        except Exception:
            pass
    
    def _extract_js_functions(self, content: str) -> List[Tuple[str, str]]:
        """Extract function definitions from JavaScript code."""
        functions = []
        
        # Named functions: function name(...) {
        pattern1 = r'function\s+(\w+)\s*\([^)]*\)\s*\{'
        for match in re.finditer(pattern1, content):
            func_name = match.group(1)
            brace_start = match.end() - 1
            body = self._extract_brace_block(content, brace_start)
            if body:
                full_text = content[match.start():brace_start] + body
                functions.append((func_name, full_text))

        # Arrow functions with parens: const/let/var name = (...) => {
        pattern_arrow = r'(?:const|let|var)\s+(\w+)\s*=\s*(?:async\s*)?\([^)]*\)\s*=>\s*\{'
        for match in re.finditer(pattern_arrow, content):
            func_name = match.group(1)
            brace_start = match.end() - 1
            body = self._extract_brace_block(content, brace_start)
            if body:
                full_text = content[match.start():brace_start] + body
                functions.append((func_name, full_text))

        # Arrow functions with single bare param: const/let/var name = param => {
        pattern_arrow_single = r'(?:const|let|var)\s+(\w+)\s*=\s*(?:async\s*)?\w+\s*=>\s*\{'
        for match in re.finditer(pattern_arrow_single, content):
            func_name = match.group(1)
            brace_start = match.end() - 1
            body = self._extract_brace_block(content, brace_start)
            if body:
                full_text = content[match.start():brace_start] + body
                functions.append((func_name, full_text))

        # Method definitions: name(...) { or name: function(...) {
        pattern2 = r'(\w+)\s*(?::\s*function)?\s*\([^)]*\)\s*\{'
        for match in re.finditer(pattern2, content):
            func_name = match.group(1)
            if func_name in ('if', 'while', 'for', 'switch', 'catch', 'function'):
                continue
            brace_start = match.end() - 1
            body = self._extract_brace_block(content, brace_start)
            if body and len(body.splitlines()) > 20:  # Only significant methods
                full_text = content[match.start():brace_start] + body
                functions.append((func_name, full_text))
        
        return functions
    
    def _calc_complexity_js(self, code: str) -> int:
        """Calculate cyclomatic complexity for JavaScript code."""
        complexity = 1
        patterns = [
            r'\bif\s*\(',        # covers both if and else if -- do not add else-if separately
            r'\bfor\s*\(',
            r'\bwhile\s*\(',
            r'\bcase\s+[^:]+:',
            r'\bcatch\s*\(',
            r'\?\s*[^:]+:',
            r'&&',
            r'\|\|',
            r'\?\?',  # nullish coalescing
        ]
        for p in patterns:
            complexity += len(re.findall(p, code))
        return complexity
    
    def _count_params_js(self, func_body: str) -> int:
        """Count parameters in JavaScript function."""
        # Named function declaration: function name(params)
        match = re.search(r'function\s+\w+\s*\(([^)]*)\)', func_body)
        if match:
            params = match.group(1).strip()
            return 0 if not params else len([p for p in params.split(',') if p.strip()])
        # Arrow function with parens: const/let/var name = (...) =>
        match = re.search(r'(?:const|let|var)\s+\w+\s*=\s*(?:async\s*)?\(([^)]*)\)\s*=>', func_body)
        if match:
            params = match.group(1).strip()
            return 0 if not params else len([p for p in params.split(',') if p.strip()])
        # Arrow function with single bare param: const/let/var name = param =>
        if re.search(r'(?:const|let|var)\s+\w+\s*=\s*(?:async\s*)?\w+\s*=>', func_body):
            return 1
        # Method shorthand: name(params) { -- search only the signature portion
        match = re.search(r'^\w+\s*\(([^)]*)\)', func_body)
        if match:
            params = match.group(1).strip()
            return 0 if not params else len([p for p in params.split(',') if p.strip()])
        return 0
    
    # --- Shared Helpers ---
    
    def _extract_brace_block(self, content: str, start: int) -> str:
        """Extract a brace-delimited block starting at position start."""
        if start >= len(content) or content[start] != '{':
            return ''
        
        depth = 0
        end = start
        
        for i in range(start, len(content)):
            if content[i] == '{':
                depth += 1
            elif content[i] == '}':
                depth -= 1
                if depth == 0:
                    end = i + 1
                    break
        
        return content[start:end] if depth == 0 else ''
    
    def _calc_nesting(self, code: str) -> int:
        """Calculate max brace nesting depth."""
        max_depth = 0
        depth = 0
        
        for char in code:
            if char == '{':
                depth += 1
                max_depth = max(max_depth, depth)
            elif char == '}':
                depth -= 1
        
        return max_depth

    # --- Comment stripping helpers ---

    @staticmethod
    def _strip_comments_c(code: str) -> str:
        """Strip C-style block and line comments, preserving newlines."""
        code = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), code, flags=re.DOTALL)
        code = re.sub(r'//[^\n]*', '', code)
        return code

    @staticmethod
    def _strip_comments_php(code: str) -> str:
        """Strip PHP block comments, // and # line comments."""
        code = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), code, flags=re.DOTALL)
        code = re.sub(r'//[^\n]*', '', code)
        code = re.sub(r'#[^\n]*', '', code)
        return code

    @staticmethod
    def _strip_comments_python(code: str) -> str:
        """Strip Python # comments and triple-quoted docstrings."""
        code = re.sub(r'(\"\"\".*?\"\"\"|\'{3}.*?\'{3})', lambda m: '\n' * m.group(0).count('\n'), code, flags=re.DOTALL)
        code = re.sub(r'#[^\n]*', '', code)
        return code

    @staticmethod
    def _strip_comments_js(code: str) -> str:
        """Strip JS block and line comments, preserving newlines."""
        code = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), code, flags=re.DOTALL)
        code = re.sub(r'//[^\n]*', '', code)
        return code


def print_report(results: Dict[str, Any], verbose: bool = False):
    """Print a human-readable report."""
    summary = results['summary']
    gods = results['god_functions']
    
    print("")
    print("=" * 60)
    print("GOD FUNCTIONS REPORT")
    print("=" * 60)
    print("")
    print(f"Files scanned:      {summary['total_files_scanned']}")
    print(f"Functions analyzed: {summary['total_functions_analyzed']}")
    print(f"God functions found: {summary['god_function_count']}")
    print(f"  CRITICAL: {summary['critical_count']}")
    print(f"  HIGH:     {summary['high_count']}")
    print("")
    
    if summary['by_language']:
        print("By language:")
        for lang, count in sorted(summary['by_language'].items()):
            print(f"  {lang}: {count}")
        print("")
    
    if gods:
        print("-" * 60)
        print("DETECTED GOD FUNCTIONS")
        print("-" * 60)
        
        # Sort by severity (CRITICAL first), then by hits
        gods_sorted = sorted(gods, key=lambda g: (0 if g['severity'] == 'CRITICAL' else 1, -g['hits']))
        
        for g in gods_sorted:
            sev_marker = "!!!" if g['severity'] == 'CRITICAL' else " ! "
            print(f"\n{sev_marker} [{g['severity']}] {g['function']}()")
            print(f"    File: {g['file']}")
            print(f"    Language: {g['language']}")
            print(f"    Thresholds exceeded: {g['hits']}/4")
            print(f"    Reasons:")
            for reason in g['reasons']:
                print(f"      - {reason}")
            
            if verbose:
                print(f"    Raw metrics: lines={g['lines']}, complexity={g['complexity']}, nesting={g['nesting']}, params={g['params']}")
    else:
        print("No god functions detected.")
    
    print("")
    print("=" * 60)
    print("")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Scan for god functions in the codebase')
    parser.add_argument('--json', action='store_true', help='Output as JSON')
    parser.add_argument('--verbose', '-v', action='store_true', help='Show detailed metrics')
    parser.add_argument('path', nargs='?', default='.', help='Path to scan (default: current directory)')
    args = parser.parse_args()
    
    # Determine root path
    script_dir = Path(__file__).parent.resolve()
    if args.path == '.':
        root_path = script_dir
    else:
        root_path = Path(args.path).resolve()
    
    scanner = GodFunctionScanner(str(root_path))
    results = scanner.scan()
    
    if args.json:
        print(json.dumps(results, indent=2))
    else:
        print_report(results, verbose=args.verbose)
    
    # Exit with error code if critical god functions found
    if results['summary']['critical_count'] > 0:
        sys.exit(2)
    elif results['summary']['god_function_count'] > 0:
        sys.exit(1)
    sys.exit(0)


if __name__ == '__main__':
    main()
