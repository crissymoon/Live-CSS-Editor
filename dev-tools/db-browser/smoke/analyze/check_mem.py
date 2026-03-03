import json

data = json.load(open('analysis_results.json'))
mem = data['memory']

print("Memory Issues:")
for t in ['DOUBLE_FREE_RISK', 'UNGUARDED_ALLOCATION', 'BUFFER_OVERFLOW_RISK', 
          'FIXED_BUFFER_OVERUSE', 'LARGE_STACK_ALLOCATION', 'UNSAFE_REALLOC']:
    count = sum(1 for i in mem['issues'] if i.get('type') == t)
    print(f"  {t}: {count}")

print(f"\nMemory Score: {mem['score']}")
print(f"Unguarded: {mem['unguarded_allocations']}")
print(f"Buffer Risks: {mem['buffer_risks']}")
