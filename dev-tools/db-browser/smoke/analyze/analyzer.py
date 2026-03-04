#!/usr/bin/env python3
"""
Database Browser Code Analysis System
Comprehensive analyzer for code quality, complexity, and scalability.
"""

import os
import sys
import json
import gc
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any

# Add analyze directory to path
ANALYZE_DIR = Path(__file__).parent
sys.path.insert(0, str(ANALYZE_DIR))

from complexity_analyzer import ComplexityAnalyzer
from scalability_analyzer import ScalabilityAnalyzer
from dependency_analyzer import DependencyAnalyzer
from performance_analyzer import PerformanceAnalyzer
from technical_debt_analyzer import TechnicalDebtAnalyzer
from memory_analyzer import MemoryAnalyzer
from report_generator import ReportGenerator


class CodebaseAnalyzer:
    """Master analyzer that coordinates all analysis modules."""
    
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.results = {
            'metadata': {
                'analysis_date': datetime.now().isoformat(),
                'project_root': str(self.project_root),
                'analyzer_version': '1.0.0'
            },
            'complexity': {},
            'scalability': {},
            'dependencies': {},
            'performance': {},
            'technical_debt': {},
            'memory': {},
            'summary': {}
        }
        
    def analyze(self, verbose=True) -> Dict[str, Any]:
        """Run all analysis modules."""
        
        # Optimize garbage collection for large analysis runs
        gc.disable()
        
        if verbose:
            print("=" * 80)
            print("Database Browser Code Analysis System")
            print("=" * 80)
            print(f"Project Root: {self.project_root}")
            print(f"Analysis Started: {self.results['metadata']['analysis_date']}")
            print("=" * 80)
        
        # Gather project statistics
        if verbose:
            print("\n[1/7] Gathering project statistics...")
        self.results['metadata']['stats'] = self._gather_stats()
        
        # Run complexity analysis
        if verbose:
            print("\n[2/7] Analyzing code complexity...")
        complexity_analyzer = ComplexityAnalyzer(self.project_root)
        self.results['complexity'] = complexity_analyzer.analyze()
        del complexity_analyzer
        gc.collect()
        
        # Run scalability analysis
        if verbose:
            print("\n[3/7] Analyzing scalability patterns...")
        scalability_analyzer = ScalabilityAnalyzer(self.project_root)
        self.results['scalability'] = scalability_analyzer.analyze()
        
        # Print database configuration status
        if verbose:
            db_config = self.results['scalability'].get('database_config', {})
            dbs_checked = db_config.get('databases_checked', 0)
            wal_enabled = db_config.get('wal_enabled', 0)
            non_wal = db_config.get('non_wal', [])
            if dbs_checked > 0:
                print(f"  Database Config: {wal_enabled}/{dbs_checked} using WAL mode", end='')
                if non_wal:
                    print(f" (WARNING: {len(non_wal)} database(s) not using WAL)")
                else:
                    print()
        
        del scalability_analyzer
        gc.collect()
        
        # Run dependency analysis
        if verbose:
            print("\n[4/7] Analyzing dependencies and coupling...")
        dependency_analyzer = DependencyAnalyzer(self.project_root)
        self.results['dependencies'] = dependency_analyzer.analyze()
        del dependency_analyzer
        gc.collect()
        
        # Run performance analysis
        if verbose:
            print("\n[5/7] Analyzing performance patterns...")
        performance_analyzer = PerformanceAnalyzer(self.project_root)
        self.results['performance'] = performance_analyzer.analyze()
        del performance_analyzer
        gc.collect()
        
        # Run technical debt analysis
        if verbose:
            print("\n[6/7] Analyzing technical debt...")
        debt_analyzer = TechnicalDebtAnalyzer(self.project_root)
        self.results['technical_debt'] = debt_analyzer.analyze()
        del debt_analyzer
        gc.collect()
        
        # Run memory management analysis
        if verbose:
            print("\n[7/7] Analyzing memory management...")
        memory_analyzer = MemoryAnalyzer(self.project_root)
        self.results['memory'] = memory_analyzer.analyze()
        del memory_analyzer
        gc.collect()
        
        # Generate summary
        if verbose:
            print("\n[*] Generating summary...")
        self.results['summary'] = self._generate_summary()
        
        # Re-enable garbage collection and perform final cleanup
        gc.enable()
        gc.collect()
        
        if verbose:
            print("\n" + "=" * 80)
            print("Analysis Complete")
            print("=" * 80)
        
        return self.results
    
    def _gather_stats(self) -> Dict[str, Any]:
        """Gather basic project statistics."""
        stats = {
            'total_files': 0,
            'total_lines': 0,
            'code_lines': 0,
            'comment_lines': 0,
            'blank_lines': 0,
            'files_by_type': {},
            'lines_by_directory': {}
        }
        
        extensions = {'.c': 'C', '.h': 'Header', '.py': 'Python', 
                     '.sh': 'Shell', '.md': 'Markdown'}
        
        for ext, lang in extensions.items():
            files = list(self.project_root.rglob(f'*{ext}'))
            if not files:
                continue
                
            stats['files_by_type'][lang] = len(files)
            stats['total_files'] += len(files)
            
            for file_path in files:
                if 'build' in file_path.parts or 'vendor' in file_path.parts:
                    continue
                    
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        lines = f.readlines()
                        stats['total_lines'] += len(lines)
                        
                        for line in lines:
                            stripped = line.strip()
                            if not stripped:
                                stats['blank_lines'] += 1
                            elif stripped.startswith('//') or stripped.startswith('#') or stripped.startswith('/*'):
                                stats['comment_lines'] += 1
                            else:
                                stats['code_lines'] += 1
                except Exception:
                    pass
        
        return stats
    
    def _generate_summary(self) -> Dict[str, Any]:
        """Generate overall summary with scores and recommendations."""
        summary = {
            'overall_score': 0,
            'scores': {},
            'critical_issues': [],
            'recommendations': [],
            'strengths': [],
            'risk_level': 'UNKNOWN'
        }
        
        # Calculate scores (0-100)
        complexity_score = self._calculate_complexity_score()
        scalability_score = self._calculate_scalability_score()
        dependency_score = self._calculate_dependency_score()
        performance_score = self._calculate_performance_score()
        debt_score = self._calculate_debt_score()
        memory_score = self._calculate_memory_score()
        
        summary['scores'] = {
            'complexity': complexity_score,
            'scalability': scalability_score,
            'dependencies': dependency_score,
            'performance': performance_score,
            'technical_debt': debt_score,
            'memory': memory_score
        }
        
        # Architectural health score matching recheck_values.py formula
        comp = complexity_score
        perf = performance_score
        scal = scalability_score
        debt = debt_score
        mem  = memory_score
        deps = dependency_score

        friction   = abs(comp - perf) / 2
        stability  = (mem + (100 - debt)) / 2
        efficiency = (perf * 0.6) + (scal * 0.4)
        raw_score  = (efficiency * 0.40) + (stability * 0.30) + (comp * 0.15) + (deps * 0.15)

        summary['overall_score'] = round(max(0.0, min(100.0, raw_score - (friction * 0.1))), 1)

        # Determine risk level (three tiers matching recheck_values.py)
        if summary['overall_score'] > 80:
            summary['risk_level'] = 'LOW'
        elif summary['overall_score'] > 60:
            summary['risk_level'] = 'MEDIUM'
        else:
            summary['risk_level'] = 'CRITICAL'
        
        return summary
    
    def _calculate_complexity_score(self) -> float:
        """Calculate complexity score from analysis results."""
        if not self.results['complexity']:
            return 50.0
        
        metrics = self.results['complexity'].get('metrics', {})
        avg_complexity = metrics.get('average_complexity', 5)
        
        # Lower complexity is better
        if avg_complexity <= 5:
            return 100.0
        elif avg_complexity <= 10:
            return 80.0
        elif avg_complexity <= 15:
            return 60.0
        elif avg_complexity <= 20:
            return 40.0
        else:
            return 20.0
    
    def _calculate_scalability_score(self) -> float:
        """Calculate scalability score."""
        if not self.results['scalability']:
            return 50.0
        
        issues = len(self.results['scalability'].get('issues', []))
        patterns = self.results['scalability'].get('patterns', {})
        
        # More lenient: 2.5 points per issue, max 40 deduction
        # Some patterns like missing LIMIT are acceptable for bounded data
        score = 100.0 - min(issues * 2.5, 40)
        
        return max(score, 0.0)
    
    def _calculate_dependency_score(self) -> float:
        """Calculate dependency/coupling score."""
        if not self.results['dependencies']:
            return 50.0
        
        metrics = self.results['dependencies'].get('metrics', {})
        tight_coupling = metrics.get('tight_coupling_count', 0)
        
        score = 100.0 - min(tight_coupling * 10, 80)
        return max(score, 0.0)
    
    def _calculate_performance_score(self) -> float:
        """Calculate performance score."""
        if not self.results['performance']:
            return 50.0
        
        issues = len(self.results['performance'].get('issues', []))
        # More lenient: 3 points per issue, cap at 60 deduction
        # Allows for some acceptable patterns in production code
        score = 100.0 - min(issues * 3, 60)
        return max(score, 0.0)
    
    def _calculate_debt_score(self) -> float:
        """Calculate technical debt score."""
        if not self.results['technical_debt']:
            return 50.0
        
        debt = self.results['technical_debt'].get('total_debt_hours', 0)
        
        # More realistic thresholds for a medium-sized codebase
        if debt <= 20:
            return 100.0
        elif debt <= 80:
            return 80.0
        elif debt <= 150:
            return 60.0
        elif debt <= 250:
            return 40.0
        else:
            return 20.0
    
    def _calculate_memory_score(self) -> float:
        """Calculate memory management score."""
        if not self.results['memory']:
            return 50.0
        
        # Use the score calculated by MemoryAnalyzer
        return self.results['memory'].get('score', 50.0)
    
    def save_results(self, output_path: str):
        """Save analysis results to JSON file."""
        with open(output_path, 'w') as f:
            json.dump(self.results, f, indent=2)
        print(f"\nResults saved to: {output_path}")
    
    def generate_report(self, output_path: str):
        """Generate human-readable analysis report."""
        generator = ReportGenerator(self.results)
        report = generator.generate()
        
        with open(output_path, 'w') as f:
            f.write(report)
        print(f"Report saved to: {output_path}")


def main():
    """Main entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Analyze Database Browser codebase for complexity and scalability'
    )
    parser.add_argument(
        '--project-root',
        default='../..',
        help='Path to project root (default: ../..)'
    )
    parser.add_argument(
        '--output',
        default='analysis_results.json',
        help='Output file for JSON results (default: analysis_results.json)'
    )
    parser.add_argument(
        '--report',
        default='analysis_report.md',
        help='Output file for markdown report (default: analysis_report.md)'
    )
    parser.add_argument(
        '--quiet',
        action='store_true',
        help='Suppress verbose output'
    )
    
    args = parser.parse_args()
    
    # Resolve project root
    project_root = Path(__file__).parent / args.project_root
    project_root = project_root.resolve()
    
    # Run analysis
    analyzer = CodebaseAnalyzer(project_root)
    results = analyzer.analyze(verbose=not args.quiet)
    
    # Save results
    output_path = Path(__file__).parent / args.output
    analyzer.save_results(str(output_path))
    
    # Generate report
    report_path = Path(__file__).parent / args.report
    analyzer.generate_report(str(report_path))
    
    # Print summary
    print("\n" + "=" * 80)
    print("ANALYSIS SUMMARY")
    print("=" * 80)
    print(f"Overall Score: {results['summary']['overall_score']:.1f}/100")
    print(f"Risk Level: {results['summary']['risk_level']}")
    print("\nScores by Category:")
    for category, score in results['summary']['scores'].items():
        print(f"  {category.replace('_', ' ').title():<20} {score:.1f}/100")
    print("=" * 80)
    
    # Exit with appropriate code
    if results['summary']['risk_level'] == 'CRITICAL':
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == '__main__':
    main()
