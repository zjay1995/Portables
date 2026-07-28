"""Thin wrapper around ``arduino-cli`` plus serial-port discovery."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import threading
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable, Sequence

LineSink = Callable[[str], None]

# Keeps console windows from flashing up on Windows for every sub-process.
_NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0) if sys.platform.startswith("win") else 0


class ToolchainError(Exception):
    """arduino-cli is missing, or a command it ran failed."""


@dataclass(frozen=True)
class SerialPort:
    device: str
    description: str = ""

    @property
    def label(self) -> str:
        return f"{self.device} - {self.description}" if self.description else self.device

    @staticmethod
    def device_of(label: str) -> str:
        """Recover the device name from a ``label`` shown in the UI."""
        return label.split(" - ", 1)[0].strip()


def _exe_name() -> str:
    return "arduino-cli.exe" if sys.platform.startswith("win") else "arduino-cli"


def find_arduino_cli(configured: str = "") -> Path | None:
    """Locate arduino-cli: explicit setting, bundled copy, PATH, usual installs."""
    if configured:
        candidate = Path(configured).expanduser()
        if candidate.is_file():
            return candidate
        if candidate.is_dir() and (candidate / _exe_name()).is_file():
            return candidate / _exe_name()

    here = Path(getattr(sys, "_MEIPASS", Path(__file__).resolve().parent.parent))
    for candidate in (here / "tools" / _exe_name(), here / _exe_name()):
        if candidate.is_file():
            return candidate

    found = shutil.which(_exe_name())
    if found:
        return Path(found)

    for candidate in _install_locations():
        if candidate.is_file():
            return candidate
    return None


def _install_locations() -> Iterable[Path]:
    name = _exe_name()
    if sys.platform.startswith("win"):
        for env in ("LOCALAPPDATA", "PROGRAMFILES", "PROGRAMFILES(X86)", "USERPROFILE"):
            base = os.environ.get(env)
            if not base:
                continue
            yield Path(base) / "Arduino15" / name
            yield Path(base) / "Arduino CLI" / name
            yield Path(base) / "arduino-cli" / name
    else:
        yield Path("/usr/local/bin") / name
        yield Path.home() / "bin" / name
        yield Path.home() / ".local" / "bin" / name


def list_ports() -> list[SerialPort]:
    """Serial ports, via pyserial when available, else via arduino-cli."""
    ports = _list_ports_pyserial()
    if ports is None:
        ports = _list_ports_cli()
    return sorted(ports, key=lambda p: p.device)


def _list_ports_pyserial() -> list[SerialPort] | None:
    try:
        from serial.tools import list_ports as pyserial_ports  # type: ignore
    except Exception:
        return None
    out = []
    for info in pyserial_ports.comports():
        description = info.description or ""
        if description in ("n/a", "N/A"):
            description = info.manufacturer or ""
        out.append(SerialPort(info.device, description))
    return out


def _list_ports_cli() -> list[SerialPort]:
    cli = find_arduino_cli()
    if not cli:
        return []
    try:
        result = subprocess.run(
            [str(cli), "board", "list", "--format", "json"],
            capture_output=True,
            text=True,
            timeout=30,
            creationflags=_NO_WINDOW,
        )
        payload = json.loads(result.stdout or "[]")
    except (OSError, ValueError, subprocess.SubprocessError):
        return []

    # arduino-cli 0.x returns a list; 1.x wraps it in {"detected_ports": [...]}.
    entries = payload.get("detected_ports", []) if isinstance(payload, dict) else payload
    out = []
    for entry in entries or []:
        port = entry.get("port", entry) or {}
        device = port.get("address")
        if not device:
            continue
        boards = entry.get("matching_boards") or []
        description = boards[0].get("name") if boards else (port.get("label") or "")
        out.append(SerialPort(device, description or ""))
    return out


class CommandRunner:
    """Runs arduino-cli commands, streaming their output line by line.

    A single runner owns at most one live sub-process so the GUI can cancel a
    build that is taking too long (or was started against the wrong port).
    """

    def __init__(self, cli_path: Path):
        self.cli_path = Path(cli_path)
        self._process: subprocess.Popen | None = None
        self._lock = threading.Lock()
        self._cancelled = False

    def run(self, args: Sequence[str], sink: LineSink, cwd: Path | None = None) -> int:
        """Run ``arduino-cli <args>``; returns the exit code (``-1`` if cancelled)."""
        command = [str(self.cli_path), *args]
        sink("> " + " ".join(_quote(a) for a in command))
        with self._lock:
            self._cancelled = False
            try:
                self._process = subprocess.Popen(
                    command,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    stdin=subprocess.DEVNULL,
                    text=True,
                    encoding="utf-8",
                    errors="replace",
                    bufsize=1,
                    cwd=str(cwd) if cwd else None,
                    creationflags=_NO_WINDOW,
                )
            except OSError as exc:
                raise ToolchainError(f"Could not start arduino-cli: {exc}") from exc
            process = self._process

        assert process.stdout is not None
        for line in process.stdout:
            sink(line.rstrip("\r\n"))
        code = process.wait()

        with self._lock:
            self._process = None
            cancelled = self._cancelled
        return -1 if cancelled else code

    def cancel(self) -> None:
        """Terminate the running command, if any."""
        with self._lock:
            self._cancelled = True
            process = self._process
        if process and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()

    @property
    def busy(self) -> bool:
        with self._lock:
            return self._process is not None and self._process.poll() is None

    # -- individual commands ------------------------------------------------

    def version(self) -> str:
        try:
            result = subprocess.run(
                [str(self.cli_path), "version"],
                capture_output=True,
                text=True,
                timeout=30,
                creationflags=_NO_WINDOW,
            )
        except (OSError, subprocess.SubprocessError) as exc:
            raise ToolchainError(f"Could not run arduino-cli: {exc}") from exc
        return (result.stdout or result.stderr or "").strip()

    def update_index(self, sink: LineSink) -> int:
        return self.run(
            [
                "core",
                "update-index",
                "--additional-urls",
                ESP32_INDEX_URL,
                "--no-color",
            ],
            sink,
        )

    def install_core(self, core: str, sink: LineSink) -> int:
        return self.run(
            ["core", "install", core, "--additional-urls", ESP32_INDEX_URL, "--no-color"],
            sink,
        )

    def install_library(self, library: str, sink: LineSink) -> int:
        return self.run(["lib", "install", library, "--no-color"], sink)

    def compile(
        self,
        sketch_dir: Path,
        fqbn: str,
        output_dir: Path,
        build_path: Path,
        sink: LineSink,
    ) -> int:
        return self.run(
            [
                "compile",
                "--fqbn",
                fqbn,
                "--output-dir",
                str(output_dir),
                "--build-path",
                str(build_path),
                "--warnings",
                "none",
                "--no-color",
                str(sketch_dir),
            ],
            sink,
        )

    def upload(self, fqbn: str, port: str, input_dir: Path, sink: LineSink) -> int:
        # No --verify: the esp32 uploader has no verify pattern, and esptool
        # already hash-checks every written block.
        return self.run(
            [
                "upload",
                "--fqbn",
                fqbn,
                "--port",
                port,
                "--input-dir",
                str(input_dir),
                "--no-color",
            ],
            sink,
        )


ESP32_INDEX_URL = "https://espressif.github.io/arduino-esp32/package_esp32_index.json"


def _quote(arg: str) -> str:
    return f'"{arg}"' if " " in arg else arg
