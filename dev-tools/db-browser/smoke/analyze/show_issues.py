#!/usr/bin/env python3
import json
import os

script_dir = os.path.dirname(os.path.abspath(__file__))
data = json.load(open(os.path.join(script_dir, 'analysis_results.json')))

print('=== MEMORY ISSUES (20 -> 85 needed) ===')
mem = data['memory']
print(f'Unguarded allocations: {mem["unguarded_allocations"]}')
print(f'Buffer risks: {mem["buffer_risks"]}')
print(f'Potential leaks: {mem["potential_leaks"]}')
issues_by_type = {}
for i in mem['issues']:
    t = i['type']
    issues_by_type[t] = issues_by_type.get(t, 0) + 1
for t, c in sorted(issues_by_type.items(), key=lambda x: -x[1]):
    print(f'  {t}: {c}')

print()
print('=== PERFORMANCE ISSUES (20 -> 80 needed) ===')
perf = data['performance']
print(f'Total issues: {len(perf["issues"])}')
issues_by_type = {}
for i in perf['issues']:
    t = i['type']
    issues_by_type[t] = issues_by_type.get(t, 0) + 1
for t, c in sorted(issues_by_type.items(), key=lambda x: -x[1]):
    print(f'  {t}: {c}')

print()
print('=== SCALABILITY ISSUES (50 -> 85 needed) ===')
scal = data['scalability']
print(f'Total issues: {len(scal["issues"])}')
issues_by_type = {}
for i in scal['issues']:
    t = i['type']
    issues_by_type[t] = issues_by_type.get(t, 0) + 1
for t, c in sorted(issues_by_type.items(), key=lambda x: -x[1]):
    print(f'  {t}: {c}')

print()
print('=== TECHNICAL DEBT (20 -> 40 needed) ===')
debt = data['technical_debt']
print(f'Total hours: {debt["total_debt_hours"]}')
debt_by_type = {}
for i in debt.get('items', []):
    t = i.get('type', 'unknown')
    debt_by_type[t] = debt_by_type.get(t, 0) + i.get('hours', 0)
for t, h in sorted(debt_by_type.items(), key=lambda x: -x[1]):
    print(f'  {t}: {h:.1f}h')
