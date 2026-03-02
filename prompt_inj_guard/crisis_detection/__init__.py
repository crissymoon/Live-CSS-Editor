"""
crisis_detection -- Regex-based crisis signal detection module.

Exports:
  CrisisDetector  -- main detector class
  DEFAULT_PATTERNS_PATH  -- default path to patterns.json
  DEFAULT_RESOURCES_PATH -- default path to safe_resources.json
"""

from crisis_detection.detector import (
    CrisisDetector,
    DEFAULT_PATTERNS_PATH,
    DEFAULT_RESOURCES_PATH,
)

__all__ = ["CrisisDetector", "DEFAULT_PATTERNS_PATH", "DEFAULT_RESOURCES_PATH"]
__version__ = "1.0.0"
