#!/usr/bin/env python3
"""
Report Generator
Generates human-readable analysis reports in Markdown format.
"""

from datetime import datetime
from typing import Dict, Any


class ReportGenerator:
    """Generates markdown reports from analysis results."""
    
    def __init__(self, results: Dict[str, Any]):
        self.results = results
        
    def generate(self) -> str:
        """Generate complete markdown report."""
        sections = [
            self._generate_header(),
            self._generate_executive_summary(),
            self._generate_project_stats(),
            self._generate_complexity_section(),
            self._generate_scalability_section(),
            self._generate_dependency_section(),
            self._generate_performance_section(),
            self._generate_technical_debt_section(),
            self._generate_memory_section(),
            self._generate_recommendations(),
            self._generate_footer()
        ]
        
        return '\n\n'.join(sections)
    
    def _generate_header(self) -> str:
        """Generate report header."""
        return f"""# Database Browser Code Analysis Report

**Generated:** {self.results['metadata']['analysis_date']}  
**Analyzer Version:** {self.results['metadata']['analyzer_version']}  
**Project Root:** {self.results['metadata']['project_root']}
"""
    
    def _generate_executive_summary(self) -> str:
        """Generate executive summary."""
        summary = self.results['summary']
        
        return f"""## Executive Summary

**Overall Score:** {summary['overall_score']:.1f}/100  
**Risk Level:** {summary['risk_level']}

### Score Breakdown

| Category | Score | Status |
|----------|-------|--------|
| Complexity | {summary['scores']['complexity']:.1f}/100 | {self._get_status(summary['scores']['complexity'])} |
| Scalability | {summary['scores']['scalability']:.1f}/100 | {self._get_status(summary['scores']['scalability'])} |
| Dependencies | {summary['scores']['dependencies']:.1f}/100 | {self._get_status(summary['scores']['dependencies'])} |
| Performance | {summary['scores']['performance']:.1f}/100 | {self._get_status(summary['scores']['performance'])} |
| Technical Debt | {summary['scores']['technical_debt']:.1f}/100 | {self._get_status(summary['scores']['technical_debt'])} |
| Memory Management | {summary['scores']['memory']:.1f}/100 | {self._get_status(summary['scores']['memory'])} |
"""
    
    def _generate_project_stats(self) -> str:
        """Generate project statistics section."""
        stats = self.results['metadata'].get('stats', {})
        
        if not stats:
            return "## Project Statistics\n\nNo statistics available."
        
        files_table = '\n'.join([
            f"| {lang} | {count} |"
            for lang, count in stats.get('files_by_type', {}).items()
        ])
        
        return f"""## Project Statistics

### Files Overview

| Language | Files |
|----------|-------|
{files_table}

### Code Metrics

- **Total Lines:** {stats.get('total_lines', 0):,}
- **Code Lines:** {stats.get('code_lines', 0):,}
- **Comment Lines:** {stats.get('comment_lines', 0):,}
- **Blank Lines:** {stats.get('blank_lines', 0):,}
- **Comment Ratio:** {(stats.get('comment_lines', 0) / max(stats.get('code_lines', 1), 1) * 100):.1f}%
"""
    
    def _generate_complexity_section(self) -> str:
        """Generate complexity analysis section."""
        complexity = self.results.get('complexity', {})
        metrics = complexity.get('metrics', {})
        issues = complexity.get('issues', [])
        
        high_complexity = [i for i in issues if i['type'] == 'HIGH_COMPLEXITY']
        long_functions = [i for i in issues if i['type'] == 'LONG_FUNCTION']
        
        sections = [f"""## Code Complexity Analysis

### Metrics

- **Total Functions:** {metrics.get('total_functions', 0)}
- **Average Complexity:** {metrics.get('average_complexity', 0):.2f}
- **Maximum Complexity:** {metrics.get('max_complexity', 0)}
- **High Complexity Functions:** {metrics.get('high_complexity_count', 0)}
- **Average Function Length:** {metrics.get('average_function_length', 0):.1f} lines
- **Long Functions (>100 lines):** {metrics.get('long_function_count', 0)}
"""]
        
        if high_complexity:
            sections.append("### High Complexity Functions\n")
            for issue in high_complexity[:10]:
                sections.append(f"- **{issue['function']}** in `{issue['file']}` (complexity: {issue['complexity']})")
        
        return '\n'.join(sections)
    
    def _generate_scalability_section(self) -> str:
        """Generate scalability analysis section."""
        scalability = self.results.get('scalability', {})
        patterns = scalability.get('patterns', {})
        issues = scalability.get('issues', [])
        arch = scalability.get('architecture', {})
        db_config = scalability.get('database_config', {})
        
        sections = [f"""## Scalability Analysis

### Architecture

- **Modular Structure:** {'Yes' if arch.get('modular') else 'No'}
- **Layered Design:** {'Yes' if arch.get('layered') else 'No'}
- **Module Count:** {len(arch.get('modules', []))}
"""]
        
        # Add database configuration section
        if db_config.get('databases_checked', 0) > 0:
            wal_count = db_config.get('wal_enabled', 0)
            total_dbs = db_config.get('databases_checked', 0)
            non_wal = db_config.get('non_wal', [])
            
            sections.append(f"""
### Database Configuration

- **Databases Checked:** {total_dbs}
- **WAL Mode Enabled:** {wal_count}
- **Non-WAL Databases:** {len(non_wal)}
""")
            
            if non_wal:
                sections.append("\n#### Databases Not Using WAL Mode\n")
                for db in non_wal:
                    sections.append(f"- `{db['path']}` (journal mode: {db['journal_mode']})")
        
        sections.append(f"""
### Patterns Detected

| Pattern | Count |
|---------|-------|
| Global State Usage | {patterns.get('global_state', 0)} |
| Singleton Pattern | {patterns.get('singleton_usage', 0)} |
| Memory Pooling | {patterns.get('memory_pooling', 0)} |
| Lazy Loading | {patterns.get('lazy_loading', 0)} |
| Caching | {patterns.get('caching', 0)} |
| Async Patterns | {patterns.get('async_patterns', 0)} |
""")
        
        if issues:
            sections.append(f"\n### Issues Found ({len(issues)})\n")
            issue_types = {}
            for issue in issues:
                issue_type = issue['type']
                issue_types[issue_type] = issue_types.get(issue_type, 0) + 1
            
            for issue_type, count in sorted(issue_types.items(), key=lambda x: x[1], reverse=True):
                sections.append(f"- **{issue_type.replace('_', ' ').title()}:** {count}")
        
        return '\n'.join(sections)
    
    def _generate_dependency_section(self) -> str:
        """Generate dependency analysis section."""
        dependencies = self.results.get('dependencies', {})
        metrics = dependencies.get('metrics', {})
        issues = dependencies.get('issues', [])
        
        sections = [f"""## Dependency Analysis

### Metrics

- **Total Files:** {metrics.get('total_files', 0)}
- **Average Dependencies:** {metrics.get('avg_dependencies', 0):.1f}
- **Maximum Dependencies:** {metrics.get('max_dependencies', 0)}
- **Average Dependents:** {metrics.get('avg_dependents', 0):.1f}
- **Maximum Dependents:** {metrics.get('max_dependents', 0)}
- **Tight Coupling Count:** {metrics.get('tight_coupling_count', 0)}
"""]
        
        hub_files = metrics.get('hub_files', [])
        if hub_files:
            sections.append("\n### Hub Files (Most Depended On)\n")
            for hub in hub_files[:5]:
                sections.append(f"- `{hub['file']}` ({hub['dependents']} dependents)")
        
        circular = [i for i in issues if i['type'] == 'CIRCULAR_DEPENDENCY']
        if circular:
            sections.append(f"\n### Circular Dependencies\n")
            for issue in circular:
                sections.append(f"- {' → '.join(issue['files'])}")
        
        return '\n'.join(sections)
    
    def _generate_performance_section(self) -> str:
        """Generate performance analysis section."""
        performance = self.results.get('performance', {})
        issues = performance.get('issues', [])
        hotspots = performance.get('hotspots', [])
        
        sections = [f"""## Performance Analysis

### Issues Found

**Total Issues:** {len(issues)}
"""]
        
        if issues:
            issue_by_severity = {}
            for issue in issues:
                sev = issue.get('severity', 'LOW')
                issue_by_severity[sev] = issue_by_severity.get(sev, 0) + 1
            
            sections.append("\n| Severity | Count |")
            sections.append("|----------|-------|")
            for sev in ['HIGH', 'MEDIUM', 'LOW']:
                if sev in issue_by_severity:
                    sections.append(f"| {sev} | {issue_by_severity[sev]} |")
        
        if hotspots:
            sections.append("\n### Performance Hotspots\n")
            for hotspot in hotspots[:5]:
                sections.append(f"- `{hotspot['file']}` ({hotspot['issue_count']} issues, severity score: {hotspot['severity_score']})")
        
        return '\n'.join(sections)
    
    def _generate_technical_debt_section(self) -> str:
        """Generate technical debt section."""
        debt = self.results.get('technical_debt', {})
        total_hours = debt.get('total_debt_hours', 0)
        debt_by_type = debt.get('debt_by_type', {})
        hotspots = debt.get('hotspots', [])
        
        sections = [f"""## Technical Debt Analysis

### Summary

**Total Estimated Debt:** {total_hours:.1f} hours ({total_hours / 8:.1f} days)
"""]
        
        if debt_by_type:
            sections.append("\n### Debt by Type\n")
            sections.append("| Type | Hours |")
            sections.append("|------|-------|")
            for debt_type, hours in sorted(debt_by_type.items(), key=lambda x: x[1], reverse=True):
                sections.append(f"| {debt_type.replace('_', ' ').title()} | {hours:.1f} |")
        
        if hotspots:
            sections.append("\n### Debt Hotspots\n")
            for hotspot in hotspots[:5]:
                sections.append(f"- `{hotspot['file']}` ({hotspot['total_hours']:.1f} hours, {hotspot['item_count']} items)")
        
        return '\n'.join(sections)
    
    def _generate_memory_section(self) -> str:
        """Generate memory management section."""
        memory = self.results.get('memory', {})
        score = memory.get('score', 0)
        issues = memory.get('issues', [])
        patterns = memory.get('memory_patterns', {})
        recommendations = memory.get('recommendations', [])
        
        sections = [f"""## Memory Management Analysis

### Summary

**Memory Score:** {score:.1f}/100  
**Total Allocations:** {memory.get('allocations_total', 0)}  
**Total Frees:** {memory.get('frees_total', 0)}  
**Potential Leaks:** {memory.get('potential_leaks', 0)}  
**Unguarded Allocations:** {memory.get('unguarded_allocations', 0)}  
**Buffer Risks:** {memory.get('buffer_risks', 0)}
"""]
        
        # Memory patterns
        pattern_status = []
        if patterns.get('uses_memory_pools'):
            pattern_status.append("Memory Pools")
        if patterns.get('uses_arena_allocation'):
            pattern_status.append("Arena Allocation")
        if patterns.get('uses_reference_counting'):
            pattern_status.append("Reference Counting")
        if patterns.get('has_cleanup_functions'):
            pattern_status.append("Cleanup Functions")
        
        if pattern_status:
            sections.append("\n### Good Patterns Detected\n")
            for pattern in pattern_status:
                sections.append(f"- {pattern}")
        
        # Issue breakdown by type
        issue_by_type = {}
        for issue in issues:
            issue_type = issue.get('type', 'UNKNOWN')
            issue_by_type[issue_type] = issue_by_type.get(issue_type, 0) + 1
        
        if issue_by_type:
            sections.append("\n### Issues by Type\n")
            sections.append("| Issue Type | Count |")
            sections.append("|------------|-------|")
            for issue_type, count in sorted(issue_by_type.items(), key=lambda x: x[1], reverse=True):
                sections.append(f"| {issue_type.replace('_', ' ').title()} | {count} |")
        
        # High severity issues
        high_severity = [i for i in issues if i.get('severity') == 'HIGH']
        if high_severity:
            sections.append(f"\n### High Severity Issues ({len(high_severity)})\n")
            for issue in high_severity[:10]:
                sections.append(f"- **{issue['file']}** (Line {issue.get('line', 'N/A')}): {issue['message']}")
        
        # Recommendations
        if recommendations:
            sections.append("\n### Memory Recommendations\n")
            for rec in recommendations:
                sections.append(f"- {rec}")
        
        return '\n'.join(sections)
    
    def _generate_recommendations(self) -> str:
        """Generate recommendations section."""
        scalability = self.results.get('scalability', {})
        recommendations = scalability.get('recommendations', [])
        
        if not recommendations:
            return """## Recommendations

No specific recommendations at this time. The codebase appears to be in good shape!
"""
        
        sections = ["## Recommendations\n"]
        for i, rec in enumerate(recommendations, 1):
            sections.append(f"{i}. {rec}")
        
        return '\n'.join(sections)
    
    def _generate_footer(self) -> str:
        """Generate report footer."""
        return f"""---

## About This Report

This automated analysis report was generated to help identify potential issues in code complexity, 
scalability, dependencies, performance, and technical debt. The scores and recommendations are based 
on static analysis and should be reviewed by developers familiar with the codebase.

**Note:** This is an automated analysis. Human judgment is required to determine which issues are 
truly important for your specific use case.
"""
    
    def _get_status(self, score: float) -> str:
        """Get status string for a score."""
        if score >= 80:
            return 'Excellent'
        elif score >= 60:
            return 'Good'
        elif score >= 40:
            return 'Needs Improvement'
        else:
            return 'Critical'
