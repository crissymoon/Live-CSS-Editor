"""
model-to-model-risk package init.

Because the directory name contains hyphens, direct `import model-to-model-risk`
is not valid Python syntax. Recommended usage:

    import importlib.util, sys
    from pathlib import Path

    _mod_dir = Path('/path/to/prompt_inj_guard/model-to-model-risk')
    sys.path.insert(0, str(_mod_dir))
    from risk_detector import RiskDetector
    from session_guard import SessionGuard

Or use the exports below after inserting the directory to sys.path.
"""

from .risk_detector  import RiskDetector, DEFAULT_PATTERNS_PATH, DEFAULT_AI_DETECTOR_DIR
from .session_guard  import SessionGuard, GuardState, SessionSummary

__all__ = [
    "RiskDetector",
    "SessionGuard",
    "GuardState",
    "SessionSummary",
    "DEFAULT_PATTERNS_PATH",
    "DEFAULT_AI_DETECTOR_DIR",
]
__version__ = "1.0.0"
