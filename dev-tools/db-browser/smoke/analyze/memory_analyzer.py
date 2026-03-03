#!/usr/bin/env python3
"""
Memory Management Analyzer
Monitors and analyzes memory management patterns and issues.
"""

import re
from pathlib import Path
from typing import Dict, List, Any, Set, Tuple


class MemoryAnalyzer:
    """Analyzes memory management patterns and potential issues."""
    
    def __init__(self, project_root: Path):
        self.project_root = Path(project_root)
        
    def analyze(self) -> Dict[str, Any]:
        """Perform memory management analysis."""
        results = {
            'score': 100,
            'issues': [],
            'allocations_total': 0,
            'frees_total': 0,
            'potential_leaks': 0,
            'unguarded_allocations': 0,
            'buffer_risks': 0,
            'memory_patterns': {
                'uses_memory_pools': False,
                'uses_arena_allocation': False,
                'uses_reference_counting': False,
                'has_cleanup_functions': False
            },
            'recommendations': []
        }
        
        source_files = list(self.project_root.rglob('*.c')) + list(self.project_root.rglob('*.h'))
        source_files = [f for f in source_files if 'build' not in f.parts and 'vendor' not in f.parts]
        
        for file_path in source_files:
            file_issues = self._analyze_file_memory(file_path)
            results['issues'].extend(file_issues)
            
            # Update patterns
            content = self._read_file(file_path)
            if content:
                self._detect_memory_patterns(content, results['memory_patterns'])
        
        # Calculate metrics
        for issue in results['issues']:
            if issue['type'] == 'MEMORY_LEAK':
                results['potential_leaks'] += 1
            elif issue['type'] == 'UNGUARDED_ALLOCATION':
                results['unguarded_allocations'] += 1
            elif issue['type'] in ['BUFFER_OVERFLOW_RISK', 'UNSAFE_COPY']:
                results['buffer_risks'] += 1
            
            if issue['type'] in ['ALLOCATION', 'REALLOCATION']:
                results['allocations_total'] += issue.get('count', 1)
            elif issue['type'] == 'FREE':
                results['frees_total'] += issue.get('count', 1)
        
        # Calculate score
        results['score'] = self._calculate_memory_score(results)
        
        # Generate recommendations
        results['recommendations'] = self._generate_recommendations(results)
        
        return results
    
    def _analyze_file_memory(self, file_path: Path) -> List[Dict[str, Any]]:
        """Analyze memory management in a single file."""
        issues = []
        content = self._read_file(file_path)
        if not content:
            return issues
        
        rel_path = str(file_path.relative_to(self.project_root))
        lines = content.splitlines()
        
        # Track allocations and frees per function
        function_allocs = self._extract_function_allocations(content)
        
        # Check for memory leaks (malloc without corresponding free)
        for func_name, allocs in function_allocs.items():
            if allocs['malloc_count'] > allocs['free_count'] and not allocs['returns_pointer']:
                issues.append({
                    'type': 'MEMORY_LEAK',
                    'severity': 'HIGH',
                    'file': rel_path,
                    'function': func_name,
                    'message': f"Function allocates {allocs['malloc_count']} times but only frees {allocs['free_count']} times",
                    'allocations': allocs['malloc_count'],
                    'frees': allocs['free_count']
                })
        
        # Check for unguarded allocations
        for i, line in enumerate(lines, 1):
            if re.search(r'\b(malloc|calloc|realloc)\s*\(', line):
                # Check if next few lines have NULL check
                next_lines = lines[i:min(i+5, len(lines))]
                has_null_check = any(re.search(r'if\s*\([^)]*==\s*NULL|if\s*\(!\s*\w+\)', l) for l in next_lines)
                
                if not has_null_check:
                    issues.append({
                        'type': 'UNGUARDED_ALLOCATION',
                        'severity': 'MEDIUM',
                        'file': rel_path,
                        'line': i,
                        'message': 'Memory allocation without NULL check'
                    })
        
        # Check for buffer overflow risks
        unsafe_funcs = ['strcpy', 'strcat', 'sprintf', 'gets', 'scanf']
        for i, line in enumerate(lines, 1):
            for unsafe_func in unsafe_funcs:
                if re.search(rf'\b{unsafe_func}\s*\(', line):
                    issues.append({
                        'type': 'BUFFER_OVERFLOW_RISK',
                        'severity': 'HIGH',
                        'file': rel_path,
                        'line': i,
                        'message': f'Using unsafe function {unsafe_func}() - use safe alternative',
                        'function': unsafe_func
                    })
        
        # Check for potential double free
        free_pattern = r'\bfree\s*\(\s*(\w+)\s*\)'
        freed_vars = []
        for i, line in enumerate(lines, 1):
            matches = re.findall(free_pattern, line)
            for var in matches:
                if var in freed_vars:
                    issues.append({
                        'type': 'DOUBLE_FREE_RISK',
                        'severity': 'HIGH',
                        'file': rel_path,
                        'line': i,
                        'message': f'Variable {var} may be freed multiple times',
                        'variable': var
                    })
                freed_vars.append(var)
        
        # Check for fixed-size buffer usage
        fixed_buffers = re.findall(r'char\s+(\w+)\s*\[\s*(\d+)\s*\]', content)
        if len(fixed_buffers) > 5:
            issues.append({
                'type': 'FIXED_BUFFER_OVERUSE',
                'severity': 'LOW',
                'file': rel_path,
                'message': f'{len(fixed_buffers)} fixed-size buffers - consider dynamic allocation',
                'count': len(fixed_buffers)
            })
        
        # Check for large stack allocations
        for i, line in enumerate(lines, 1):
            match = re.search(r'char\s+\w+\s*\[\s*(\d+)\s*\]', line)
            if match:
                size = int(match.group(1))
                if size > 4096:
                    issues.append({
                        'type': 'LARGE_STACK_ALLOCATION',
                        'severity': 'MEDIUM',
                        'file': rel_path,
                        'line': i,
                        'message': f'Large stack allocation ({size} bytes) - consider heap allocation',
                        'size': size
                    })
        
        # Check for realloc without NULL check on failure
        for i, line in enumerate(lines, 1):
            if re.search(r'\w+\s*=\s*realloc\s*\(\s*\w+\s*,', line):
                next_lines = lines[i:min(i+5, len(lines))]
                has_null_check = any(re.search(r'if\s*\([^)]*==\s*NULL', l) for l in next_lines)
                
                if not has_null_check:
                    issues.append({
                        'type': 'UNSAFE_REALLOC',
                        'severity': 'MEDIUM',
                        'file': rel_path,
                        'line': i,
                        'message': 'realloc without NULL check - original pointer may be lost'
                    })
        
        return issues
    
    def _extract_function_allocations(self, content: str) -> Dict[str, Dict[str, Any]]:
        """Extract memory allocation patterns per function."""
        functions = {}
        
        # Find all functions
        func_pattern = r'(\w+)\s+(\w+)\s*\(([^)]*)\)\s*\{'
        func_matches = list(re.finditer(func_pattern, content))
        
        for i, match in enumerate(func_matches):
            func_name = match.group(2)
            start_pos = match.end()
            
            # Find end of function (matching closing brace)
            end_pos = self._find_function_end(content, start_pos)
            if end_pos == -1:
                continue
            
            func_body = content[start_pos:end_pos]
            
            # Count allocations and frees
            malloc_count = len(re.findall(r'\b(malloc|calloc|realloc)\s*\(', func_body))
            free_count = len(re.findall(r'\bfree\s*\(', func_body))
            
            # Check if function returns a pointer
            return_type = match.group(1)
            returns_pointer = '*' in return_type or 'ptr' in return_type.lower()
            
            # Check for return statements with allocated memory
            has_return_alloc = bool(re.search(r'return\s+\w+', func_body))
            
            functions[func_name] = {
                'malloc_count': malloc_count,
                'free_count': free_count,
                'returns_pointer': returns_pointer or has_return_alloc
            }
        
        return functions
    
    def _find_function_end(self, content: str, start_pos: int) -> int:
        """Find the closing brace of a function."""
        brace_count = 1
        pos = start_pos
        
        while pos < len(content) and brace_count > 0:
            if content[pos] == '{':
                brace_count += 1
            elif content[pos] == '}':
                brace_count -= 1
            pos += 1
        
        return pos if brace_count == 0 else -1
    
    def _detect_memory_patterns(self, content: str, patterns: Dict[str, bool]):
        """Detect common memory management patterns."""
        if re.search(r'struct\s+\w*pool\w*|memory_pool|mem_pool', content, re.IGNORECASE):
            patterns['uses_memory_pools'] = True
        
        if re.search(r'arena|allocator', content, re.IGNORECASE):
            patterns['uses_arena_allocation'] = True
        
        if re.search(r'ref_count|refcount|addref|release', content, re.IGNORECASE):
            patterns['uses_reference_counting'] = True
        
        if re.search(r'cleanup|destroy|finalize|dispose', content, re.IGNORECASE):
            patterns['has_cleanup_functions'] = True
    
    def _calculate_memory_score(self, results: Dict[str, Any]) -> float:
        """Calculate overall memory management score."""
        score = 100.0
        
        # Deduct for critical issues
        score -= results['potential_leaks'] * 10
        score -= results['unguarded_allocations'] * 2
        score -= results['buffer_risks'] * 5
        
        # Deduct for high severity issues
        high_severity = sum(1 for i in results['issues'] if i.get('severity') == 'HIGH')
        score -= high_severity * 8
        
        # Deduct for medium severity issues
        medium_severity = sum(1 for i in results['issues'] if i.get('severity') == 'MEDIUM')
        score -= medium_severity * 3
        
        # Bonus for good patterns
        patterns = results['memory_patterns']
        if patterns['uses_memory_pools']:
            score += 5
        if patterns['has_cleanup_functions']:
            score += 3
        if patterns['uses_reference_counting']:
            score += 2
        
        # Ensure score stays in valid range
        return max(0.0, min(100.0, score))
    
    def _generate_recommendations(self, results: Dict[str, Any]) -> List[str]:
        """Generate actionable recommendations."""
        recommendations = []
        
        if results['potential_leaks'] > 0:
            recommendations.append(
                f"Fix {results['potential_leaks']} potential memory leaks by ensuring all allocations are freed"
            )
        
        if results['unguarded_allocations'] > 5:
            recommendations.append(
                f"Add NULL checks for {results['unguarded_allocations']} unguarded memory allocations"
            )
        
        if results['buffer_risks'] > 0:
            recommendations.append(
                f"Replace {results['buffer_risks']} unsafe string functions with safe alternatives (strncpy, snprintf, etc.)"
            )
        
        if not results['memory_patterns']['uses_memory_pools'] and results['allocations_total'] > 50:
            recommendations.append(
                "Consider implementing memory pools for frequent allocations to reduce fragmentation"
            )
        
        if not results['memory_patterns']['has_cleanup_functions']:
            recommendations.append(
                "Implement cleanup/destroy functions for consistent resource management"
            )
        
        double_frees = sum(1 for i in results['issues'] if i['type'] == 'DOUBLE_FREE_RISK')
        if double_frees > 0:
            recommendations.append(
                f"Review {double_frees} potential double-free issues and set pointers to NULL after freeing"
            )
        
        large_stack = sum(1 for i in results['issues'] if i['type'] == 'LARGE_STACK_ALLOCATION')
        if large_stack > 0:
            recommendations.append(
                f"Move {large_stack} large stack allocations to heap to prevent stack overflow"
            )
        
        return recommendations
    
    def _read_file(self, file_path: Path) -> str:
        """Read file content safely."""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                return f.read()
        except Exception:
            return ""
