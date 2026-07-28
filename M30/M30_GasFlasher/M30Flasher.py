"""Entry point for the M30 Gas Flasher desktop application.

Run from source with:   python M30Flasher.py
Build a Windows .exe:   build_windows_exe.bat
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from m30flasher.gui import main  # noqa: E402

if __name__ == "__main__":
    raise SystemExit(main())
