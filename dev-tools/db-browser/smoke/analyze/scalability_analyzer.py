#!/usr/bin/env python3
"""
Scalability Analyzer
Analyzes code patterns and architecture for scalability concerns.
"""

import re
from pathlib import Path
from typing import Dict, List, Any


class ScalabilityAnalyzer:
    """Analyzes scalability patterns and potential bottlenecks."""
    
    def __init__(self, project_root: Path):
        self.project_root = Path(project_root)
        
    def analyze(self) -> Dict[str, Any]:
        """Perform scalability analysis."""
        results = {
            'patterns': {},
            'issues': [],
            'recommendations': [],
            'architecture': {}
        }
        
        # Analyze architecture
        results['architecture'] = self._analyze_architecture()
        
        # Analyze scalability patterns
        results['patterns'] = self._analyze_patterns()
        
        # Find scalability issues
        c_files = list(self.project_root.rglob('*.c'))
        c_files = [f for f in c_files if 'build' not in f.parts and 'vendor' not in f.parts]
        
        for c_file in c_files:
            issues = self._analyze_file_scalability(c_file)
            results['issues'].extend(issues)
        
        # Generate recommendations
        results['recommendations'] = self._generate_recommendations(results)
        
        return results
    
    def _analyze_architecture(self) -> Dict[str, Any]:
        """Analyze overall architecture."""
        arch = {
            'modular': False,
            'layered': False,
            'modules': [],
            'core_components': []
        }
        
        # Check for modular structure
        modules_dir = self.project_root / 'modules'
        if modules_dir.exists():
            arch['modular'] = True
            arch['modules'] = [d.name for d in modules_dir.iterdir() if d.is_file() and d.suffix == '.c']
        
        # Check for core components
        core_dir = self.project_root / 'core'
        if core_dir.exists():
            arch['layered'] = True
            arch['core_components'] = [f.name for f in core_dir.iterdir() if f.suffix == '.c']
        
        return arch
    
    def _analyze_patterns(self) -> Dict[str, Any]:
        """Analyze scalability patterns in the codebase."""
        patterns = {
            'global_state': 0,
            'singleton_usage': 0,
            'memory_pooling': 0,
            'lazy_loading': 0,
            'caching': 0,
            'async_patterns': 0,
            'connection_pooling': 0
        }
        
        c_files = list(self.project_root.rglob('*.c'))
        c_files = [f for f in c_files if 'build' not in f.parts and 'vendor' not in f.parts]
        
        for c_file in c_files:
            try:
                with open(c_file, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    
                    # Check for global variables
                    global_vars = re.findall(r'^\s*(?:static\s+)?(?:extern\s+)?[\w\*]+\s+\w+\s*=', content, re.MULTILINE)
                    patterns['global_state'] += len(global_vars)
                    
                    # Check for singleton pattern
                    if re.search(r'static\s+\w+\s*\*\s*instance\s*=', content):
                        patterns['singleton_usage'] += 1
                    
                    # Check for memory pooling
                    if 'pool' in content.lower() and 'alloc' in content.lower():
                        patterns['memory_pooling'] += 1
                    
                    # Check for lazy loading
                    if re.search(r'if\s*\(\s*!\s*\w+\s*\)\s*\{[^}]*\w+\s*=', content):
                        patterns['lazy_loading'] += 1
                    
                    # Check for caching
                    if 'cache' in content.lower():
                        patterns['caching'] += 1
                    
                    # Check for async patterns
                    if re.search(r'g_thread|pthread|async', content, re.IGNORECASE):
                        patterns['async_patterns'] += 1
                    
                    # Check for connection pooling
                    if 'connection' in content.lower() and 'pool' in content.lower():
                        patterns['connection_pooling'] += 1
            
            except Exception:
                pass
        
        return patterns
    
    def _analyze_file_scalability(self, file_path: Path) -> List[Dict[str, Any]]:
        """Analyze scalability issues in a single file."""
        issues = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.splitlines()
                
                # Check for O(n^2) patterns
                nested_loops = re.findall(r'for\s*\([^)]+\)\s*\{[^}]*for\s*\([^)]+\)', content, re.DOTALL)
                if len(nested_loops) > 3:
                    issues.append({
                        'type': 'NESTED_LOOPS',
                        'severity': 'MEDIUM',
                        'file': str(file_path.relative_to(self.project_root)),
                        'count': len(nested_loops),
                        'message': f"Found {len(nested_loops)} nested loops which may impact scalability"
                    })
                
                # Check for string concatenation in loops
                if re.search(r'for\s*\([^)]+\)[^}]*strcat|for\s*\([^)]+\)[^}]*sprintf', content, re.DOTALL):
                    issues.append({
                        'type': 'STRING_CONCAT_IN_LOOP',
                        'severity': 'HIGH',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': 'String concatenation in loop detected - performance bottleneck'
                    })
                
                # Check for unbounded data structures
                if re.search(r'malloc|realloc', content) and not re.search(r'if\s*\([^)]*>\s*\d+\)', content):
                    issues.append({
                        'type': 'UNBOUNDED_ALLOCATION',
                        'severity': 'MEDIUM',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': 'Dynamic allocation without apparent size limits'
                    })
                
                # Check for N+1 query pattern
                if re.search(r'while\s*\([^)]*sqlite3_step[^}]*sqlite3_prepare|for\s*\([^)]*\)[^}]*sqlite3_prepare', content, re.DOTALL):
                    issues.append({
                        'type': 'N_PLUS_ONE_QUERY',
                        'severity': 'HIGH',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': 'Potential N+1 query pattern detected'
                    })
                
                # Check for missing pagination
                if 'SELECT' in content and 'LIMIT' not in content.upper():
                    select_count = len(re.findall(r'SELECT\s+', content, re.IGNORECASE))
                    if select_count > 2:
                        issues.append({
                            'type': 'MISSING_PAGINATION',
                            'severity': 'MEDIUM',
                            'file': str(file_path.relative_to(self.project_root)),
                            'message': 'Queries without LIMIT clause may not scale'
                        })
                
                # Check for large static buffers
                large_buffers = re.findall(r'char\s+\w+\[(\d+)\]', content)
                for size in large_buffers:
                    if int(size) > 4096:
                        issues.append({
                            'type': 'LARGE_STATIC_BUFFER',
                            'severity': 'LOW',
                            'file': str(file_path.relative_to(self.project_root)),
                            'size': int(size),
                            'message': f'Large static buffer of {size} bytes allocated'
                        })
                        break  # Only report once per file
        
        except Exception:
            pass
        
        return issues
    
    def _generate_recommendations(self, results: Dict[str, Any]) -> List[str]:
        """Generate scalability recommendations based on analysis."""
        recommendations = []
        
        # Architecture recommendations
        if not results['architecture']['modular']:
            recommendations.append(
                "Consider implementing a modular architecture to improve maintainability and scalability"
            )
        
        # Pattern recommendations
        patterns = results['patterns']
        
        if patterns['global_state'] > 10:
            recommendations.append(
                f"High usage of global state ({patterns['global_state']} instances). "
                "Consider encapsulating state in structures passed to functions."
            )
        
        if patterns['caching'] == 0:
            recommendations.append(
                "No caching patterns detected. Consider implementing caching for frequently accessed data."
            )
        
        if patterns['async_patterns'] == 0:
            recommendations.append(
                "No asynchronous patterns detected. Consider async operations for I/O-bound tasks."
            )
        
        if patterns['memory_pooling'] == 0:
            recommendations.append(
                "No memory pooling detected. Consider implementing memory pools for frequently allocated objects."
            )
        
        # Issue-based recommendations
        issue_types = {}
        for issue in results['issues']:
            issue_type = issue['type']
            issue_types[issue_type] = issue_types.get(issue_type, 0) + 1
        
        if issue_types.get('NESTED_LOOPS', 0) > 3:
            recommendations.append(
                "Multiple nested loops detected. Review algorithms for potential optimization."
            )
        
        if issue_types.get('N_PLUS_ONE_QUERY', 0) > 0:
            recommendations.append(
                "N+1 query patterns detected. Use batch queries or JOIN operations to improve performance."
            )
        
        if issue_types.get('MISSING_PAGINATION', 0) > 0:
            recommendations.append(
                "Add LIMIT clauses to queries to prevent loading excessive data."
            )
        
        return recommendations
