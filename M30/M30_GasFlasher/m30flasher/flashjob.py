"""Build-and-upload orchestration: stage, select gas, compile, upload, log.

Kept free of any GUI code so the whole flow can be unit-tested and, if needed,
driven from a script.
"""

from __future__ import annotations

import csv
import datetime as _dt
from collections import deque
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable

from .profiles import GasProfile
from .settings import Settings
from .sketch import Sketch, stage
from .toolchain import CommandRunner, ToolchainError

LineSink = Callable[[str], None]
StageSink = Callable[[str], None]

BINARY_SUFFIXES = (".bin", ".elf")


class JobCancelled(Exception):
    """The operator pressed Stop."""


@dataclass
class Paths:
    """Work-directory layout for one sketch."""

    work: Path

    def staging(self) -> Path:
        return self.work / "staging"

    def build_path(self, sketch_name: str) -> Path:
        return self.work / "buildpath" / sketch_name

    def artifacts(self, sketch_name: str, profile_key: str, fingerprint: str) -> Path:
        return self.work / "firmware" / sketch_name / profile_key / fingerprint[:12]

    def flash_log(self) -> Path:
        return self.work / "flash_log.csv"


# Turns the raw arduino-cli/esptool wording into something an operator on the
# bench can act on.  First match wins.
_HINTS: tuple[tuple[str, str], ...] = (
    (
        "platform not installed",
        "The ESP32 core is not installed. Run Tools > Install / update ESP32 toolchain.",
    ),
    (
        "not found in any known index",
        "The ESP32 core is not installed. Run Tools > Install / update ESP32 toolchain.",
    ),
    (
        "no such file or directory",
        "A library header is missing. Run Tools > Install / update ESP32 toolchain.",
    ),
    (
        "section exceeds available space",
        "The build does not fit the flash partition. Pick a board preset with a "
        "bigger app partition (Huge App).",
    ),
    (
        "sketch too big",
        "The build does not fit the flash partition. Pick a board preset with a "
        "bigger app partition (Huge App).",
    ),
    (
        "invalid fqbn",
        "The board does not accept one of the FQBN options — the option names "
        "differ per board. Pick an entry from the Board list instead.",
    ),
    (
        "access is denied",
        "The COM port is busy. Close the Arduino Serial Monitor or any terminal "
        "using this unit, then try again.",
    ),
    (
        "permission denied",
        "The COM port is busy or the user lacks access rights to it.",
    ),
    (
        "could not open port",
        "The unit is not on that COM port any more. Press Refresh and pick it again.",
    ),
    (
        "failed to connect",
        "The ESP32 did not enter the bootloader. Hold BOOT, tap EN/RESET, release "
        "BOOT, then upload again — or lower UploadSpeed in the FQBN.",
    ),
    (
        "timed out waiting for packet header",
        "Upload lost sync. Try a shorter/data-capable USB cable or lower "
        "UploadSpeed in the FQBN.",
    ),
    (
        "wrong boot mode detected",
        "The unit booted into run mode. Hold BOOT while starting the upload.",
    ),
)


def hint_for(lines: Iterable[str]) -> str:
    """A remedy for the first known failure signature found in ``lines``."""
    blob = "\n".join(lines).lower()
    for needle, hint in _HINTS:
        if needle in blob:
            return hint
    return ""


def has_artifacts(directory: Path) -> bool:
    """True when ``directory`` holds a compiled sketch ready to upload."""
    if not directory.is_dir():
        return False
    return any(p.suffix in BINARY_SUFFIXES for p in directory.iterdir() if p.is_file())


class FlashJob:
    """Prepares and runs one gas build/upload against one unit."""

    def __init__(self, settings: Settings, runner: CommandRunner, log: LineSink, stage_cb: StageSink):
        self.settings = settings
        self.runner = runner
        self.log = log
        self.stage_cb = stage_cb
        self.paths = Paths(settings.work_path)
        self._cancelled = False
        self._recent: deque[str] = deque(maxlen=400)

    def _sink(self, line: str) -> None:
        """Forward a tool output line to the UI and keep it for hint matching."""
        self._recent.append(line)
        self.log(line)

    # -- steps --------------------------------------------------------------

    def cancel(self) -> None:
        self._cancelled = True
        self.runner.cancel()

    def _check_cancelled(self) -> None:
        if self._cancelled:
            raise JobCancelled()

    def prepare(self, sketch: Sketch, profile: GasProfile) -> tuple[Sketch, Path]:
        """Stage the sketch, select ``profile`` in it, and return the artifact dir."""
        self._check_cancelled()
        self.stage_cb(f"Staging {sketch.name}")
        self.log(f"Source sketch : {sketch.root}")
        self.log(f"Gas profile   : {profile.gas}  ({profile.range_label})  [inc/{profile.include_path}]")

        staged = stage(sketch, self.paths.staging())
        staged.select(profile)

        active = staged.active_include()
        if active != profile.include_path:
            raise ToolchainError(
                f"Gas selection failed: SensorConfig.h reports '{active}' after edit"
            )
        self.log(f"Staged copy   : {staged.root}")
        self.log(f"Active gas    : {active}")

        artifacts = self.paths.artifacts(staged.name, profile.key, staged.fingerprint())
        self.log(f"Firmware dir  : {artifacts}")
        return staged, artifacts

    def compile(self, staged: Sketch, artifacts: Path) -> None:
        self._check_cancelled()
        self.stage_cb("Compiling")
        artifacts.mkdir(parents=True, exist_ok=True)
        build_path = self.paths.build_path(staged.name)
        build_path.mkdir(parents=True, exist_ok=True)
        self._recent.clear()
        code = self.runner.compile(
            staged.root, self.settings.fqbn, artifacts, build_path, self._sink
        )
        self._raise_for(code, "Compile")
        if not has_artifacts(artifacts):
            raise ToolchainError("Compile reported success but produced no .bin file")

    def upload(self, artifacts: Path, port: str) -> None:
        self._check_cancelled()
        if not port:
            raise ToolchainError("No serial port selected — plug in the unit and press Refresh")
        if not has_artifacts(artifacts):
            raise ToolchainError("No compiled firmware for this gas yet — run Build & Upload first")
        self.stage_cb(f"Uploading to {port}")
        self._recent.clear()
        code = self.runner.upload(self.settings.fqbn, port, artifacts, self._sink)
        self._raise_for(code, "Upload")

    def _raise_for(self, code: int, step: str) -> None:
        if code == -1 or self._cancelled:
            raise JobCancelled()
        if code != 0:
            message = f"{step} failed (arduino-cli exit code {code})"
            hint = hint_for(self._recent)
            raise ToolchainError(f"{message}\n{hint}" if hint else message)

    # -- traceability -------------------------------------------------------

    def record(self, sketch: Sketch, profile: GasProfile, port: str, result: str) -> None:
        """Append one line to the flash log so units can be traced later."""
        path = self.paths.flash_log()
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            new_file = not path.exists()
            with path.open("a", newline="", encoding="utf-8") as handle:
                writer = csv.writer(handle)
                if new_file:
                    writer.writerow(
                        [
                            "timestamp",
                            "unit_id",
                            "gas",
                            "range",
                            "profile",
                            "bt_name",
                            "sketch",
                            "fw_version",
                            "fqbn",
                            "port",
                            "result",
                        ]
                    )
                writer.writerow(
                    [
                        _dt.datetime.now().isoformat(timespec="seconds"),
                        self.settings.unit_id,
                        profile.gas,
                        profile.range_label,
                        profile.key,
                        profile.bt_name,
                        sketch.name,
                        sketch.firmware_version(),
                        self.settings.fqbn,
                        port,
                        result,
                    ]
                )
        except OSError as exc:
            self.log(f"(could not write flash log: {exc})")

    # -- whole flows --------------------------------------------------------

    def build_and_upload(
        self, sketch: Sketch, profile: GasProfile, port: str, do_upload: bool = True
    ) -> Path:
        """Stage + compile, then optionally upload.  Returns the artifact dir."""
        staged, artifacts = self.prepare(sketch, profile)
        if has_artifacts(artifacts):
            self.log("Firmware for this gas is already built — skipping compile.")
        else:
            self.compile(staged, artifacts)
        if do_upload:
            self.upload(artifacts, port)
        return artifacts

    def upload_only(self, sketch: Sketch, profile: GasProfile, port: str) -> Path:
        """Upload the cached build for ``profile`` without recompiling."""
        staged, artifacts = self.prepare(sketch, profile)
        self.upload(artifacts, port)
        return artifacts

    def setup_toolchain(self) -> None:
        """Install the ESP32 core and every library the M30 sketch needs."""
        self.stage_cb("Updating package index")
        self._raise_for(self.runner.update_index(self._sink), "Index update")
        self.stage_cb(f"Installing {self.settings.core}")
        self._raise_for(self.runner.install_core(self.settings.core, self._sink), "Core install")
        for library in self.settings.libraries:
            self._check_cancelled()
            self.stage_cb(f"Installing {library}")
            self._raise_for(self.runner.install_library(library, self._sink), f"'{library}' install")
