#!/usr/bin/env python3
"""
Technical Debt Analyzer
Analyzes code for technical debt indicators.
"""

import re
from pathlib import Path
from typing import Dict, List, Any


class TechnicalDebtAnalyzer:
    """Analyzes technical debt in the codebase."""
    
    def __init__(self, project_root: Path):
        self.project_root = Path(project_root)
        
    def analyze(self) -> Dict[str, Any]:
        """Perform technical debt analysis."""
        results = {
            'total_debt_hours': 0,
            'debt_by_type': {},
            'items': [],
            'hotspots': []
        }
        
        # Analyze all source files
        source_files = list(self.project_root.rglob('*.c')) + list(self.project_root.rglob('*.h'))
        source_files = [f for f in source_files if 'build' not in f.parts and 'vendor' not in f.parts]
        
        for file_path in source_files:
            debt_items = self._analyze_file_debt(file_path)
            results['items'].extend(debt_items)
        
        # Calculate total debt
        for item in results['items']:
            results['total_debt_hours'] += item['estimated_hours']
            debt_type = item['type']
            if debt_type not in results['debt_by_type']:
                results['debt_by_type'][debt_type] = 0
            results['debt_by_type'][debt_type] += item['estimated_hours']
        
        # Identify hotspot files
        results['hotspots'] = self._identify_hotspots(results['items'])
        
        return results
    
    def _analyze_file_debt(self, file_path: Path) -> List[Dict[str, Any]]:
        """Analyze technical debt in a single file."""
        debt_items = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.splitlines()
                
                # Check for TODO/FIXME/HACK comments
                for i, line in enumerate(lines, 1):
                    if re.search(r'//\s*(TODO|FIXME|HACK|XXX)', line, re.IGNORECASE):
                        match = re.search(r'(TODO|FIXME|HACK|XXX)\s*:?\s*(.+)', line, re.IGNORECASE)
                        message = match.group(2).strip() if match else 'No description'
                        
                        debt_type = match.group(1).upper() if match else 'TODO'
                        hours = 2 if debt_type == 'TODO' else (4 if debt_type == 'FIXME' else 1)
                        
                        debt_items.append({
                            'type': debt_type,
                            'file': str(file_path.relative_to(self.project_root)),
                            'line': i,
                            'message': message,
                            'estimated_hours': hours
                        })
                
                # Check for duplicated code blocks
                if self._has_code_duplication(content):
                    debt_items.append({
                        'type': 'CODE_DUPLICATION',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': 'Potential code duplication detected',
                        'estimated_hours': 4
                    })
                
                # Check for magic numbers
                magic_numbers = re.findall(r'\b(\d{3,})\b', content)
                if len(magic_numbers) > 10:
                    debt_items.append({
                        'type': 'MAGIC_NUMBERS',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': f'{len(magic_numbers)} magic numbers should be constants',
                        'estimated_hours': 2
                    })
                
                # Check for missing error handling
                malloc_calls = len(re.findall(r'\bmalloc\s*\(', content))
                null_checks = len(re.findall(r'if\s*\([^)]*==\s*NULL|if\s*\(!\s*\w+\s*\)', content))
                if malloc_calls > null_checks + 2:
                    debt_items.append({
                        'type': 'MISSING_ERROR_HANDLING',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': f'{malloc_calls - null_checks} allocations without NULL check',
                        'estimated_hours': 3
                    })
                
                # Check for long functions (already debt)
                long_funcs = re.findall(r'\w+\s+\w+\s*\([^)]*\)\s*\{[^}]{800,}\}', content, re.DOTALL)
                if long_funcs:
                    debt_items.append({
                        'type': 'LONG_FUNCTION',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': f'{len(long_funcs)} functions need refactoring',
                        'estimated_hours': len(long_funcs) * 3
                    })
                
                # Check for deprecated patterns
                if re.search(r'\bgets\s*\(|\bsprintf\s*\(|\bstrcpy\s*\(', content):
                    debt_items.append({
                        'type': 'DEPRECATED_FUNCTION',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': 'Using deprecated unsafe functions',
                        'estimated_hours': 2
                    })
                
                # Check for missing documentation
                function_count = len(re.findall(r'^\s*(?:static\s+)?\w+\s+\w+\s*\([^)]*\)\s*\{', content, re.MULTILINE))
                doc_count = len(re.findall(r'/\*\*|///', content))
                if function_count > 5 and doc_count < function_count * 0.3:
                    debt_items.append({
                        'type': 'MISSING_DOCUMENTATION',
                        'file': str(file_path.relative_to(self.project_root)),
                        'message': f'{function_count} functions lack documentation',
                        'estimated_hours': function_count * 0.5
                    })
        
        except Exception:
            pass
        
        return debt_items
    
    def _has_code_duplication(self, content: str) -> bool:
        """Detect potential code duplication."""
        # Simple heuristic: look for repeated patterns
        lines = [l.strip() for l in content.splitlines() if l.strip() and not l.strip().startswith('//')]
        
        # Count repeated blocks of 5+ lines
        block_size = 5
        blocks = []
        for i in range(len(lines) - block_size):
            block = tuple(lines[i:i+block_size])
            blocks.append(block)
        
        # Check for duplicates
        unique_blocks = set(blocks)
        if len(blocks) > 0 and len(unique_blocks) / len(blocks) < 0.8:
            return True
        
        return False
    
    def _identify_hotspots(self, items: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Identify files with the most technical debt."""
        file_debt = {}
        
        for item in items:
            if 'file' in item:
                file_key = item['file']
                if file_key not in file_debt:
                    file_debt[file_key] = {'file': file_key, 'total_hours': 0, 'item_count': 0}
                
                file_debt[file_key]['total_hours'] += item.get('estimated_hours', 0)
                file_debt[file_key]['item_count'] += 1
        
        hotspots = sorted(file_debt.values(), key=lambda x: x['total_hours'], reverse=True)
        return hotspots[:10]

