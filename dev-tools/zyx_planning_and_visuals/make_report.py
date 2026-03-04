# Make Report 
import subprocess

def run_shell_script(script_path): # Get Scan Output
    try:
        result = subprocess.run(
            [script_path],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout, result.stderr
    except subprocess.CalledProcessError as e:
        print(f"Script failed with return code {e.returncode}")
        return e.stdout, e.stderr
    except FileNotFoundError:
        print(f"Error: The script file was not found at {script_path}")
        return "", f"FileNotFoundError: {script_path}"

def py_out(py_script):
    import subprocess
    result = subprocess.run(
        ["python3", py_script],
        capture_output=True,
        text=True
    ); return result.stdout
 

# ************************ PROJECT DATA START _____
TECH_STACK = [
    'Python', 
    'JavaScript', 
    'PHP', 
    'C', 
    'HTML', 
    'CSS', 
    'Shell', 
    'Go'
]

TEST_FILES = [
    '/Users/mac/Documents/live-css/search.py', # 1
    '/Users/mac/Documents/live-css/py_audit.py', # 2
    '/Users/mac/Documents/live-css/lines_count.py', # 3
    '/Users/mac/Documents/live-css/god_funcs.py', # 4
    '/Users/mac/Documents/live-css/dev-tools/db-browser/smoke/analyze/run_analysis.sh' # 5
]
# ^-TEST_FILES
# 1. Searches files --not needed to report but keep here for ref
# 2. Scans the live-css project for Python code quality concerns
# 3. Counts files for long lines
# 4. Checks for god-funcs
# 5. Complete GAP Analysis of the DB System

MAP_FILES = [
    '/Users/mac/Documents/live-css/model-map.json',
]

HEADER_INO = """
Project: Crissy's Style Tool -- ck live-css dir

Author:----Crissy Deutsch
Company:---XcaliburMoon Web Development
Website:---https://xcaliburmoon.net/
License:---MIT
"""

OVERVIEW = """---Overview---
This is a scan tool for this project..."""

# ************************ PROJECT DATA STOP^^^^^


# ************************ RUN SCANS & REPORTS START _____
def pull_db_report(): 
    script_file = "./dev-tools/db-browser/smoke/analyze/run_analysis.sh"
    stdout, stderr = run_shell_script(script_file)
    s = f"{stdout}\n{stderr}"
    lst = s.split('\n'); print(f"\n{'*'*70}\n\nDB-Browser Test: {script_file}\n\n{'*'*70}\n")
    for x in range(51, 64):print(lst[x])


def pull_audit_report(): # 2. Scans the live-css project for Python code quality concerns
    f = "/Users/mac/Documents/live-css/py_audit.py"; s = py_out(f)
    s1 = s[-225:]; s2 = s1[:-12]
    print(f"\n---PY AUDIT--->>> File: {f}\n{s2}\n")

def print_output_by_num(x):
    if x == 5: return print("Cannot run .sh from this call..")
    script_file = str(TEST_FILES[x-1])
    stdout, stderr = run_shell_script(script_file)
    print(f"{stdout}\n{stderr}")

# print_output_by_num(4) # Check for god-funcs

print(f"{HEADER_INO}\n{OVERVIEW}\n")
print(f"Tech Stack: {str(TECH_STACK).replace("'","")[1:-1]}\n")
# pull_audit_report() # 2. Scans the live-css project for Python code quality concerns --not ready for this yet --is in a later phase
pull_db_report() # DB Browser Test Report
print("")
    
    

