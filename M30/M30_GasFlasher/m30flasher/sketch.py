"""Discovery of M30 firmware sketches and editing of the active gas selector.

A "sketch" here is an Arduino sketch folder that follows the M30 layout::

    M30_VerK_20260531/
        M30_VerK_20260531.ino
        inc/SensorConfig.h        <- selects the active gas profile
        inc/sensors/*.h           <- one header per gas

Selecting a gas means commenting out every ``#include "sensors/..."`` line in
``inc/SensorConfig.h`` except the one for the wanted profile.  The tool never
edits the repository copy: it stages the sketch into a work directory first
(see :func:`stage`), so the sources under version control stay untouched.
"""

from __future__ import annotations

import hashlib
import re
import shutil
from dataclasses import dataclass
from pathlib import Path

from .profiles import GasProfile, load_profiles

# Matches an (optionally commented out) profile include in SensorConfig.h.
_INCLUDE_RE = re.compile(
    r"""^(?P<indent>\s*)
         (?P<comment>//\s*)?
         \#include\s+"(?P<path>sensors/[^"]+)"
         (?P<rest>.*)$""",
    re.VERBOSE,
)

# Column the trailing ``// comment`` is padded to when a line is rewritten.
_COMMENT_COLUMN = 41

# Directories that must never be copied into the staging area.
_IGNORED_DIRS = {".git", ".idea", ".vscode", ".claude", "__pycache__", "build", "output"}


class SketchError(Exception):
    """Raised when a sketch folder is missing or malformed."""


@dataclass(frozen=True)
class Sketch:
    """An M30 firmware sketch folder."""

    root: Path

    @property
    def name(self) -> str:
        return self.root.name

    @property
    def ino(self) -> Path:
        return self.root / f"{self.root.name}.ino"

    @property
    def sensor_config(self) -> Path:
        return self.root / "inc" / "SensorConfig.h"

    @property
    def sensors_dir(self) -> Path:
        return self.root / "inc" / "sensors"

    def validate(self) -> None:
        if not self.ino.is_file():
            raise SketchError(f"{self.root}: no {self.root.name}.ino found")
        if not self.sensor_config.is_file():
            raise SketchError(f"{self.root}: no inc/SensorConfig.h found")
        if not self.sensors_dir.is_dir():
            raise SketchError(f"{self.root}: no inc/sensors/ folder found")

    def profiles(self, include_templates: bool = False) -> list[GasProfile]:
        return load_profiles(self.sensors_dir, include_templates=include_templates)

    def firmware_version(self) -> str:
        """The ``FW_VERSION`` string from the .ino, for display and the flash log."""
        try:
            text = self.ino.read_text(encoding="utf-8", errors="replace")
        except OSError:
            return ""
        m = re.search(r'#define\s+FW_VERSION\s+"([^"]*)"', text)
        return m.group(1) if m else ""

    def active_include(self) -> str | None:
        """The ``sensors/*.h`` include currently uncommented, if exactly one is."""
        active = active_includes(self.sensor_config.read_text(encoding="utf-8"))
        return active[0] if len(active) == 1 else None

    def select(self, profile: GasProfile) -> None:
        """Rewrite ``inc/SensorConfig.h`` so only ``profile`` is included."""
        text = self.sensor_config.read_text(encoding="utf-8")
        self.sensor_config.write_text(
            select_profile(text, profile.include_path), encoding="utf-8", newline=""
        )

    def fingerprint(self, selected: str | None = None) -> str:
        """Content hash of every source file — the build cache key.

        With ``selected`` (a ``sensors/*.h`` include path), the hash is the one
        this sketch *would* have once that gas is selected.  That lets the UI
        look up a cached build for any gas without writing anything to disk,
        and it matches what :meth:`select` then produces byte for byte.
        """
        digest = hashlib.sha256()
        config = self.sensor_config.resolve()
        for path in sorted(_source_files(self.root)):
            digest.update(str(path.relative_to(self.root)).replace("\\", "/").encode())
            digest.update(b"\0")
            if selected is not None and path.resolve() == config:
                text = path.read_text(encoding="utf-8")
                digest.update(select_profile(text, selected).encode("utf-8"))
            else:
                digest.update(path.read_bytes())
            digest.update(b"\0")
        return digest.hexdigest()


def active_includes(text: str) -> list[str]:
    """Every profile include in ``text`` that is not commented out."""
    out = []
    for line in text.splitlines():
        m = _INCLUDE_RE.match(line)
        if m and not m.group("comment"):
            out.append(m.group("path"))
    return out


def known_includes(text: str) -> list[str]:
    """Every profile include listed in ``text``, commented out or not."""
    return [m.group("path") for m in (_INCLUDE_RE.match(l) for l in text.splitlines()) if m]


def select_profile(text: str, include_path: str) -> str:
    """Return ``text`` with ``include_path`` the only active profile include.

    Line comments and the overall layout of the file are preserved; only the
    ``//`` prefixes on the include lines change.  Raises :class:`SketchError`
    if the wanted include is not listed in the file.
    """
    lines = text.splitlines(keepends=True)
    found = False
    for i, line in enumerate(lines):
        body = line.rstrip("\r\n")
        newline = line[len(body):]
        m = _INCLUDE_RE.match(body)
        if not m:
            continue
        path = m.group("path")
        rest = m.group("rest").strip()
        code = f'#include "{path}"'
        if path != include_path:
            code = "// " + code
        else:
            found = True
        if rest:
            code = f"{code:<{_COMMENT_COLUMN}} {rest}"
        lines[i] = code + newline
    if not found:
        raise SketchError(
            f'SensorConfig.h has no #include line for "{include_path}" — '
            "add one to the profile list before selecting this gas."
        )
    return "".join(lines)


def _source_files(root: Path):
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if any(part in _IGNORED_DIRS for part in path.relative_to(root).parts[:-1]):
            continue
        if path.suffix.lower() in {".ino", ".cpp", ".c", ".h", ".hpp", ".s", ".properties"}:
            yield path


def _copy_ignore(directory: str, names: list[str]) -> set[str]:
    return {n for n in names if n in _IGNORED_DIRS}


def stage(sketch: Sketch, dest_root: Path) -> Sketch:
    """Copy ``sketch`` into ``dest_root`` so it can be modified and compiled.

    The staged folder keeps the original name (Arduino requires the folder and
    the .ino to match) and is rebuilt from scratch on every call so a stale
    file from a previous gas can never leak into a build.
    """
    sketch.validate()
    dest = dest_root / sketch.name
    if dest.exists():
        shutil.rmtree(dest)
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(sketch.root, dest, ignore=_copy_ignore)
    staged = Sketch(dest)
    staged.validate()
    return staged


def discover_sketches(
    search_roots: list[Path], max_depth: int = 2, prefer: str = "M30"
) -> list[Sketch]:
    """Find every M30-layout sketch under the given roots, best default first.

    Released ``prefer``-model sketches sort ahead of demo and beta folders, and
    within each group the newest (highest dated name) comes first — so the
    combo box opens on the firmware an operator most likely wants.
    """
    found: dict[Path, Sketch] = {}
    for root in search_roots:
        if not root or not root.is_dir():
            continue
        for candidate in _candidate_dirs(root, max_depth):
            sketch = Sketch(candidate)
            try:
                sketch.validate()
            except SketchError:
                continue
            found.setdefault(candidate.resolve(), sketch)
    return sorted(found.values(), key=lambda s: _sketch_rank(s, prefer))


def _sketch_rank(sketch: Sketch, prefer: str) -> tuple:
    name = sketch.name
    lowered = name.lower()
    return (
        0 if lowered.startswith(prefer.lower()) else 1,
        1 if lowered.startswith("demo") else 0,
        1 if "beta" in lowered else 0,
        _ReverseStr(lowered),
    )


class _ReverseStr(str):
    """Sorts strings descending inside an otherwise ascending sort key."""

    def __lt__(self, other):  # type: ignore[override]
        return str.__gt__(self, other)

    def __gt__(self, other):  # type: ignore[override]
        return str.__lt__(self, other)


def _candidate_dirs(root: Path, max_depth: int):
    yield root
    if max_depth <= 0:
        return
    try:
        children = sorted(p for p in root.iterdir() if p.is_dir())
    except OSError:
        return
    for child in children:
        if child.name in _IGNORED_DIRS:
            continue
        yield from _candidate_dirs(child, max_depth - 1)
