#!/usr/bin/env python3
"""
Dependency Analyzer
Analyzes module dependencies, coupling, and cohesion.
"""

import re
from pathlib import Path
from typing import Dict, List, Any, Set


class DependencyAnalyzer:
    """Analyzes code dependencies and coupling."""
    
    def __init__(self, project_root: Path):
        self.project_root = Path(project_root)
        
    def analyze(self) -> Dict[str, Any]:
        """Perform dependency analysis."""
        results = {
            'metrics': {},
            'dependencies': {},
            'issues': [],
            'graph': {}
        }
        
        # Build dependency graph
        c_files = list(self.project_root.rglob('*.c'))
        h_files = list(self.project_root.rglob('*.h'))
        all_files = [f for f in (c_files + h_files) if 'build' not in f.parts and 'vendor' not in f.parts]
        
        for file_path in all_files:
            deps = self._extract_dependencies(file_path)
            file_key = str(file_path.relative_to(self.project_root))
            results['dependencies'][file_key] = deps
            results['graph'][file_key] = {'depends_on': deps, 'depended_by': []}
        
        # Calculate reverse dependencies
        for file_key, deps in results['dependencies'].items():
            for dep in deps:
                if dep in results['graph']:
                    results['graph'][dep]['depended_by'].append(file_key)
        
        # Calculate metrics
        results['metrics'] = self._calculate_metrics(results['graph'])
        
        # Find issues
        results['issues'] = self._find_issues(results['graph'], results['metrics'])
        
        return results
    
    def _extract_dependencies(self, file_path: Path) -> List[str]:
        """Extract #include dependencies from a file."""
        dependencies = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                
                # Find local includes
                includes = re.findall(r'#include\s+"([^"]+)"', content)
                dependencies.extend(includes)
        
        except Exception:
            pass
        
        return dependencies
    
    def _calculate_metrics(self, graph: Dict[str, Any]) -> Dict[str, Any]:
        """Calculate coupling and cohesion metrics."""
        metrics = {
            'total_files': len(graph),
            'avg_dependencies': 0,
            'max_dependencies': 0,
            'avg_dependents': 0,
            'max_dependents': 0,
            'tight_coupling_count': 0,
            'hub_files': [],
            'isolated_files': []
        }
        
        if not graph:
            return metrics
        
        dep_counts = []
        dependent_counts = []
        
        for file_key, data in graph.items():
            dep_count = len(data['depends_on'])
            dependent_count = len(data['depended_by'])
            
            dep_counts.append(dep_count)
            dependent_counts.append(dependent_count)
            
            # Identify tightly coupled files (high fan-in and fan-out)
            if dep_count > 5 and dependent_count > 5:
                metrics['tight_coupling_count'] += 1
            
            # Identify hub files (depended on by many)
            if dependent_count > 10:
                metrics['hub_files'].append({
                    'file': file_key,
                    'dependents': dependent_count
                })
            
            # Identify isolated files
            if dep_count == 0 and dependent_count == 0:
                metrics['isolated_files'].append(file_key)
        
        metrics['avg_dependencies'] = sum(dep_counts) / len(dep_counts)
        metrics['max_dependencies'] = max(dep_counts)
        metrics['avg_dependents'] = sum(dependent_counts) / len(dependent_counts)
        metrics['max_dependents'] = max(dependent_counts)
        
        # Sort hub files by dependent count
        metrics['hub_files'].sort(key=lambda x: x['dependents'], reverse=True)
        
        return metrics
    
    def _find_issues(self, graph: Dict[str, Any], metrics: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Find dependency-related issues."""
        issues = []
        
        # Check for circular dependencies
        circles = self._find_circular_dependencies(graph)
        for circle in circles:
            issues.append({
                'type': 'CIRCULAR_DEPENDENCY',
                'severity': 'HIGH',
                'files': circle,
                'message': f'Circular dependency detected: {" -> ".join(circle)}'
            })
        
        # Check for files with too many dependencies
        for file_key, data in graph.items():
            dep_count = len(data['depends_on'])
            if dep_count > 10:
                issues.append({
                    'type': 'HIGH_COUPLING',
                    'severity': 'MEDIUM',
                    'file': file_key,
                    'count': dep_count,
                    'message': f'File depends on {dep_count} other files'
                })
        
        return issues
    
    def _find_circular_dependencies(self, graph: Dict[str, Any]) -> List[List[str]]:
        """Find circular dependencies using DFS."""
        circles = []
        visited = set()
        rec_stack = set()
        
        def dfs(node: str, path: List[str]):
            visited.add(node)
            rec_stack.add(node)
            path.append(node)
            
            if node in graph:
                for dep in graph[node]['depends_on']:
                    if dep not in visited:
                        dfs(dep, path.copy())
                    elif dep in rec_stack:
                        # Found a circle
                        circle_start = path.index(dep)
                        circle = path[circle_start:] + [dep]
                        if circle not in circles:
                            circles.append(circle)
            
            rec_stack.remove(node)
        
        for node in graph.keys():
            if node not in visited:
                dfs(node, [])
        
        return circles
