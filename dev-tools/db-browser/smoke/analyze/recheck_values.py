# Correct Formula for Architectural Health Score Calculation
import json
import os

def calculate_architectural_health(metrics):

    comp = metrics['Complexity']
    perf = metrics['Performance']
    scal = metrics['Scalability']
    debt = metrics['Technical Debt']
    mem  = metrics['Memory']
    deps = metrics['Dependencies']

    friction = abs(comp - perf) / 2

    stability = (mem + (100 - debt)) / 2

    efficiency = (perf * 0.6) + (scal * 0.4)

    raw_score = (efficiency * 0.40) + (stability * 0.30) + (comp * 0.15) + (deps * 0.15)
    final_score = raw_score - (friction * 0.1)

    return round(max(0, min(100, final_score)), 1)

def get_risk_level(score):
    if score > 80: return "HEALTHY (Low Risk)"
    if score > 60: return "STABLE (Moderate Risk)"
    return "CRITICAL (High Risk)"

def load_current_metrics():
    """Load actual metrics from analysis_results.json"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    results_path = os.path.join(script_dir, 'analysis_results.json')
    
    if os.path.exists(results_path):
        with open(results_path, 'r') as f:
            data = json.load(f)
            scores = data.get('summary', {}).get('scores', {})
            return {
                'Complexity': scores.get('complexity', 50.0),
                'Scalability': scores.get('scalability', 50.0),
                'Dependencies': scores.get('dependencies', 50.0),
                'Performance': scores.get('performance', 50.0),
                'Technical Debt': scores.get('technical_debt', 50.0),
                'Memory': scores.get('memory', 50.0)
            }
    return None

# Load from analysis results or use fallback
current_metrics = load_current_metrics()
if current_metrics is None:
    current_metrics = {
        'Complexity': 100.0, 'Scalability': 50.0, 'Dependencies': 100.0,
        'Performance': 20.0, 'Technical Debt': 20.0, 'Memory': 20.0
    }

target_metrics = {
    'Complexity': 75.0,    # Slightly de-coupled C code
    'Scalability': 85.0,   # WAL mode & Multi-Silo DBs
    'Dependencies': 90.0,  # Minimal external libs
    'Performance': 80.0,   # Batch processing in C
    'Technical Debt': 40.0,# Allow for some flexibility/abstraction
    'Memory': 85.0         # Strict sqlite3_finalize usage
}

current_score = calculate_architectural_health(current_metrics)
target_score = calculate_architectural_health(target_metrics)

print("")
print("=" * 50)
print("GAP ANALYSIS (Current vs Target)")
print("=" * 50)
print(f"{'Category':<20} {'Current':>10} {'Target':>10} {'Gap':>10}")
print("-" * 50)
for key in current_metrics:
    curr = current_metrics[key]
    targ = target_metrics[key]
    gap = targ - curr
    sign = "+" if gap > 0 else ""
    print(f"{key:<20} {curr:>10.1f} {targ:>10.1f} {sign}{gap:>9.1f}")
print("-" * 50)
print(f"{'Overall Score':<20} {current_score:>10.1f} {target_score:>10.1f} {'+' if target_score > current_score else ''}{target_score - current_score:>9.1f}")
print(f"{'Risk Level':<20} {get_risk_level(current_score):>31}")
print(f"{'Target Risk':<20} {get_risk_level(target_score):>31}")
print("=" * 50)
