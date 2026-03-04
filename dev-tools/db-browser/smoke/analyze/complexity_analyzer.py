#!/usr/bin/env python3
"""
Code Complexity Analyzer
Analyzes cyclomatic complexity, function length, nesting depth, and other metrics.
"""

import re
from pathlib import Path
from typing import Dict, List, Any


class ComplexityAnalyzer:
    """Analyzes code complexity metrics."""
    
    def __init__(self, project_root: Path):
        self.project_root = Path(project_root)
        
    def analyze(self) -> Dict[str, Any]:
        """Perform complexity analysis."""
        results = {
            'metrics': {},
            'files': [],
            'functions': [],
            'issues': []
        }
        
        # Analyze C source files (skip build, vendor, and legacy)
        c_files = list(self.project_root.rglob('*.c'))
        c_files = [f for f in c_files
                   if 'build'  not in f.parts
                   and 'vendor' not in f.parts
                   and 'legacy' not in f.parts]
        
        total_complexity = 0
        function_count = 0
        
        for c_file in c_files:
            file_analysis = self._analyze_file(c_file)
            results['files'].append(file_analysis)
            
            for func in file_analysis['functions']:
                results['functions'].append(func)
                total_complexity += func['complexity']
                function_count += 1
                
                # Flag high complexity functions
                if func['complexity'] > 15:
                    results['issues'].append({
                        'type': 'HIGH_COMPLEXITY',
                        'severity': 'HIGH' if func['complexity'] > 25 else 'MEDIUM',
                        'file': str(c_file.relative_to(self.project_root)),
                        'function': func['name'],
                        'complexity': func['complexity'],
                        'message': f"Function has cyclomatic complexity of {func['complexity']}"
                    })
                
                # Flag long functions
                if func['lines'] > 100:
                    results['issues'].append({
                        'type': 'LONG_FUNCTION',
                        'severity': 'MEDIUM',
                        'file': str(c_file.relative_to(self.project_root)),
                        'function': func['name'],
                        'lines': func['lines'],
                        'message': f"Function is {func['lines']} lines long"
                    })
                
                # Flag deep nesting
                if func['max_nesting'] > 5:
                    results['issues'].append({
                        'type': 'DEEP_NESTING',
                        'severity': 'MEDIUM',
                        'file': str(c_file.relative_to(self.project_root)),
                        'function': func['name'],
                        'nesting': func['max_nesting'],
                        'message': f"Function has nesting depth of {func['max_nesting']}"
                    })

                # God function: meets 2 or more severe thresholds simultaneously
                god_hits = sum([
                    func['lines']         > 150,
                    func['complexity']    > 20,
                    func['max_nesting']   > 4,
                    func['parameter_count'] > 5,
                ])
                if god_hits >= 2:
                    reasons = []
                    if func['lines']           > 150: reasons.append(f"{func['lines']} lines")
                    if func['complexity']      > 20:  reasons.append(f"complexity {func['complexity']}")
                    if func['max_nesting']     > 4:   reasons.append(f"nesting depth {func['max_nesting']}")
                    if func['parameter_count'] > 5:   reasons.append(f"{func['parameter_count']} params")
                    results['issues'].append({
                        'type': 'GOD_FUNCTION',
                        'severity': 'CRITICAL' if god_hits >= 3 else 'HIGH',
                        'file': str(c_file.relative_to(self.project_root)),
                        'function': func['name'],
                        'lines': func['lines'],
                        'complexity': func['complexity'],
                        'nesting': func['max_nesting'],
                        'parameter_count': func['parameter_count'],
                        'god_hits': god_hits,
                        'message': f"God function ({god_hits}/4 thresholds exceeded): {', '.join(reasons)}"
                    })
        
        # Calculate overall metrics
        results['metrics'] = {
            'total_functions': function_count,
            'average_complexity': total_complexity / function_count if function_count > 0 else 0,
            'max_complexity': max((f['complexity'] for f in results['functions']), default=0),
            'high_complexity_count': sum(1 for f in results['functions'] if f['complexity'] > 15),
            'average_function_length': sum(f['lines'] for f in results['functions']) / function_count if function_count > 0 else 0,
            'long_function_count': sum(1 for f in results['functions'] if f['lines'] > 100),
            'god_function_count': sum(1 for i in results['issues'] if i['type'] == 'GOD_FUNCTION'),
        }
        
        return results
    
    def _analyze_file(self, file_path: Path) -> Dict[str, Any]:
        """Analyze a single C file."""
        result = {
            'path': str(file_path.relative_to(self.project_root)),
            'functions': [],
            'lines': 0
        }
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                result['lines'] = len(content.splitlines())
                
                # Find functions
                functions = self._extract_functions(content)
                for func_name, func_code, start_line in functions:
                    func_analysis = {
                        'name': func_name,
                        'line': start_line,
                        'lines': len(func_code.splitlines()),
                        'complexity': self._calculate_cyclomatic_complexity(func_code),
                        'max_nesting': self._calculate_max_nesting(func_code),
                        'parameter_count': self._count_parameters(func_code)
                    }
                    result['functions'].append(func_analysis)
        
        except Exception as e:
            result['error'] = str(e)
        
        return result
    
    def _extract_functions(self, content: str) -> List[tuple]:
        """Extract function definitions from C code."""
        functions = []
        
        # Match function definitions (simplified pattern)
        pattern = r'^\s*(?:static\s+)?(?:const\s+)?[\w\s\*]+\s+(\w+)\s*\([^)]*\)\s*\{'
        
        lines = content.splitlines()
        i = 0
        
        while i < len(lines):
            match = re.match(pattern, lines[i])
            if match:
                func_name = match.group(1)
                start_line = i + 1
                
                # Find function end
                brace_count = 1
                func_lines = [lines[i]]
                i += 1
                
                while i < len(lines) and brace_count > 0:
                    line = lines[i]
                    func_lines.append(line)
                    brace_count += line.count('{') - line.count('}')
                    i += 1
                
                func_code = '\n'.join(func_lines)
                functions.append((func_name, func_code, start_line))
            else:
                i += 1
        
        return functions
    
    def _calculate_cyclomatic_complexity(self, code: str) -> int:
        """Calculate cyclomatic complexity for a function."""
        complexity = 1  # Base complexity
        
        # Count decision points
        decision_keywords = [
            r'\bif\s*\(',
            r'\bwhile\s*\(',
            r'\bfor\s*\(',
            r'\bcase\s+',
            r'\b\?\s*',  # Ternary operator
            r'\&\&',     # Logical AND
            r'\|\|'      # Logical OR
        ]
        
        for keyword in decision_keywords:
            complexity += len(re.findall(keyword, code))
        
        return complexity
    
    def _calculate_max_nesting(self, code: str) -> int:
        """Calculate maximum nesting depth."""
        max_depth = 0
        current_depth = 0
        
        for char in code:
            if char == '{':
                current_depth += 1
                max_depth = max(max_depth, current_depth)
            elif char == '}':
                current_depth = max(0, current_depth - 1)
        
        return max_depth
    
    def _count_parameters(self, code: str) -> int:
        """Count function parameters."""
        # Extract function signature
        match = re.search(r'\([^)]*\)', code)
        if not match:
            return 0
        
        params = match.group(0)[1:-1].strip()
        if not params or params == 'void':
            return 0
        
        return len([p for p in params.split(',') if p.strip()])
