#!/usr/bin/env python3
"""
Performance Analyzer
Analyzes code for performance issues and anti-patterns.
"""

import re
from pathlib import Path
from typing import Dict, List, Any


class PerformanceAnalyzer:
    """Analyzes code for performance issues."""
    
    # Pre-compile regex patterns for better performance
    STRCAT_PATTERN = re.compile(r'strcat|strncat')
    STRLEN_PATTERN = re.compile(r'strlen\s*\(')
    ALLOC_IN_LOOP_PATTERN = re.compile(r'(for|while)\s*\([^)]+\)[^}]*(malloc|calloc|realloc)', re.DOTALL)
    NON_CONST_PARAMS_PATTERN = re.compile(r'\(.*?char\s*\*\s*\w+.*?\)')
    LINEAR_SEARCH_PATTERN = re.compile(r'for\s*\([^)]+\)[^}]*if\s*\([^)]*==', re.DOTALL)
    UNNECESSARY_COPY_PATTERN = re.compile(r'strcpy\s*\(.*?,\s*.*?\)\s*;[^}]*\breturn\b', re.DOTALL)
    
    def __init__(self, project_root: Path):
        self.project_root = Path(project_root)
        
    def analyze(self) -> Dict[str, Any]:
        """Perform performance analysis."""
        results = {
            'issues': [],
            'patterns': {},
            'hotspots': []
        }
        
        # Analyze C files for performance issues
        c_files = list(self.project_root.rglob('*.c'))
        c_files = [f for f in c_files if 'build' not in f.parts and 'vendor' not in f.parts]
        
        for c_file in c_files:
            issues = self._analyze_file_performance(c_file)
            results['issues'].extend(issues)
        
        # Identify potential hotspots
        results['hotspots'] = self._identify_hotspots(results['issues'])
        
        return results
    
    def _analyze_file_performance(self, file_path: Path) -> List[Dict[str, Any]]:
        """Analyze performance issues in a single file."""
        issues = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                
                # Check for inefficient string operations
                if self.STRCAT_PATTERN.search(content):
                    issues.append({
                        'type': 'INEFFICIENT_STRING_OP',
                        'severity': 'LOW',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': 'Using strcat which requires scanning to find end of string'
                    })
                
                # Check for repeated strlen calls
                strlen_matches = self.STRLEN_PATTERN.findall(content)
                strlen_count = len(strlen_matches)
                if strlen_count > 5:
                    issues.append({
                        'type': 'REPEATED_STRLEN',
                        'severity': 'LOW',
                        'file': str(file_path.relative_to(self.project_root)),
                        'count': strlen_count,
                        'message': f'{strlen_count} strlen calls - consider caching length'
                    })
                
                # Check for memory allocation in loops
                if self.ALLOC_IN_LOOP_PATTERN.search(content):
                    issues.append({
                        'type': 'ALLOCATION_IN_LOOP',
                        'severity': 'MEDIUM',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': 'Memory allocation inside loop detected'
                    })
                
                # Check for missing const qualifiers
                non_const_params = self.NON_CONST_PARAMS_PATTERN.findall(content)
                if len(non_const_params) > 3:
                    issues.append({
                        'type': 'MISSING_CONST',
                        'severity': 'LOW',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': 'Many string parameters without const qualifier'
                    })
                
                # Check for linear search patterns
                if self.LINEAR_SEARCH_PATTERN.search(content):
                    issues.append({
                        'type': 'LINEAR_SEARCH',
                        'severity': 'LOW',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': 'Linear search pattern detected - consider hash tables for large datasets'
                    })
                
                # Check for unnecessary copying
                if self.UNNECESSARY_COPY_PATTERN.search(content):
                    issues.append({
                        'type': 'UNNECESSARY_COPY',
                        'severity': 'LOW',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': 'Potential unnecessary string copy before return'
                    })
                
                # Check for missing inline keywords on small functions
                small_funcs = re.findall(r'static\\s+\\w+\\s+(\\w+)\\s*\\([^)]*\\)\\s*\\{[^}]{1,100}\\}', content, re.DOTALL)
                if len(small_funcs) > 3:
                    issues.append({
                        'type': 'MISSING_INLINE',
                        'severity': 'LOW',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': f'{len(small_funcs)} small static functions could be inline'
                    })
                
                # Check for printf in loops (I/O in loops)
                if re.search(r'(for|while)\\s*\\([^)]+\\)[^}]*(printf|fprintf|puts)', content, re.DOTALL):
                    issues.append({
                        'type': 'IO_IN_LOOP',
                        'severity': 'MEDIUM',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': 'I/O operations inside loop - buffer output instead'
                    })
                
                # Check for non-indexed SQLite queries
                if 'sqlite3' in content and 'CREATE INDEX' not in content.upper():
                    if len(re.findall(r'SELECT.*WHERE', content, re.IGNORECASE)) > 3:
                        issues.append({
                            'type': 'MISSING_INDEX',
                            'severity': 'MEDIUM',
                            'file': str(file_path.relative_to(self.project_root)),
                            'message': 'Multiple WHERE queries without apparent index creation'
                        })
        
        except Exception:
            pass
        
        return issues
    
    def _identify_hotspots(self, issues: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Identify files with the most performance issues."""
        file_counts = {}
        
        for issue in issues:
            if 'file' in issue:
                file_key = issue['file']
                if file_key not in file_counts:
                    file_counts[file_key] = {'file': file_key, 'issue_count': 0, 'severity_score': 0}
                
                file_counts[file_key]['issue_count'] += 1
                
                # Add severity score
                severity = issue.get('severity', 'LOW')
                if severity == 'HIGH':
                    file_counts[file_key]['severity_score'] += 3
                elif severity == 'MEDIUM':
                    file_counts[file_key]['severity_score'] += 2
                else:
                    file_counts[file_key]['severity_score'] += 1
        
        hotspots = sorted(file_counts.values(), key=lambda x: x['severity_score'], reverse=True)
        return hotspots[:10]  # Return top 10 hotspots
