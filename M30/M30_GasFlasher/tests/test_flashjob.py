"""Tests for the build/upload orchestration, using a stub arduino-cli runner."""

from __future__ import annotations

import csv
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from m30flasher.flashjob import (  # noqa: E402
    FlashJob,
    JobCancelled,
    Paths,
    has_artifacts,
    hint_for,
)
from m30flasher.settings import Settings  # noqa: E402
from m30flasher.sketch import Sketch  # noqa: E402
from m30flasher.toolchain import ToolchainError  # noqa: E402

SELECTOR = """#pragma once
// #include "sensors/EC_CO_300ppm.h"    // CO
#include "sensors/EC_H2S_50ppm.h"       // H2S
"""


class FakeRunner:
    """Stands in for CommandRunner; records calls and fakes compiler output."""

    def __init__(self, compile_code: int = 0, upload_code: int = 0, emit_binary: bool = True):
        self.compile_code = compile_code
        self.upload_code = upload_code
        self.emit_binary = emit_binary
        self.calls: list[tuple] = []
        self.cancelled = False

    def compile(self, sketch_dir, fqbn, output_dir, build_path, sink):
        self.calls.append(("compile", Path(sketch_dir), fqbn, Path(output_dir)))
        sink("Compiling sketch...")
        if self.emit_binary and self.compile_code == 0:
            Path(output_dir).mkdir(parents=True, exist_ok=True)
            (Path(output_dir) / "sketch.ino.bin").write_bytes(b"\x00firmware")
        return self.compile_code

    def upload(self, fqbn, port, input_dir, sink):
        self.calls.append(("upload", fqbn, port, Path(input_dir)))
        sink(f"Writing to {port}")
        return self.upload_code

    def update_index(self, sink):
        self.calls.append(("update_index",))
        return 0

    def install_core(self, core, sink):
        self.calls.append(("install_core", core))
        return 0

    def install_library(self, library, sink):
        self.calls.append(("install_library", library))
        return 0

    def cancel(self):
        self.cancelled = True


class HintTests(unittest.TestCase):
    def test_missing_core(self) -> None:
        hint = hint_for(["Error during build: Platform 'esp32:esp32' not found: platform not installed"])
        self.assertIn("ESP32 core is not installed", hint)

    def test_busy_port(self) -> None:
        hint = hint_for(["could not open port 'COM7': Access is denied."])
        self.assertIn("Close the Arduino Serial Monitor", hint)

    def test_bootloader(self) -> None:
        hint = hint_for(["A fatal error occurred: Failed to connect to ESP32: Wrong boot mode detected"])
        self.assertIn("BOOT", hint)

    def test_partition_overflow(self) -> None:
        hint = hint_for([".text' section exceeds available space in board"])
        self.assertIn("Huge App", hint)

    def test_bad_fqbn_option(self) -> None:
        hint = hint_for(["Error during build: Invalid FQBN: getting build properties"])
        self.assertIn("Board list", hint)

    def test_unknown_failure_has_no_hint(self) -> None:
        self.assertEqual(hint_for(["something went sideways"]), "")


class FlashJobTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp, True)

        self.root = self.tmp / "src" / "M30_VerK_20260531"
        (self.root / "inc" / "sensors").mkdir(parents=True)
        (self.root / f"{self.root.name}.ino").write_text(
            '#define FW_VERSION "M30 VerK 05/31/2026"\n', encoding="utf-8"
        )
        (self.root / "inc" / "SensorConfig.h").write_text(SELECTOR, encoding="utf-8")
        for name, gas in (("EC_CO_300ppm.h", "CO"), ("EC_H2S_50ppm.h", "H2S")):
            (self.root / "inc" / "sensors" / name).write_text(
                f'#define SENSOR_GAS_NAME "{gas}"\n'
                f'#define SENSOR_BT_NAME "ESP32 EC-{gas}"\n'
                "#define SENSOR_RANGE_FS 300.0f\n"
                "#define SENSOR_ADS_GAIN GAIN_FOUR\n",
                encoding="utf-8",
            )
        self.sketch = Sketch(self.root)
        self.settings = Settings(
            sketch_dir=str(self.root),
            work_dir=str(self.tmp / "work"),
            unit_id="SN-0001",
            fqbn="esp32:esp32:esp32",
        )
        self.lines: list[str] = []
        self.steps: list[str] = []

    def make_job(self, runner) -> FlashJob:
        return FlashJob(self.settings, runner, self.lines.append, self.steps.append)

    def profile(self, key: str):
        return next(p for p in self.sketch.profiles() if p.key == key)

    def test_build_and_upload_happy_path(self) -> None:
        runner = FakeRunner()
        job = self.make_job(runner)
        artifacts = job.build_and_upload(self.sketch, self.profile("EC_CO_300ppm"), "COM7")

        self.assertTrue(has_artifacts(artifacts))
        kinds = [call[0] for call in runner.calls]
        self.assertEqual(kinds, ["compile", "upload"])
        self.assertEqual(runner.calls[1][2], "COM7")
        # the compiled sketch is the staged copy, not the repository one
        self.assertEqual(runner.calls[0][1], self.settings.work_path / "staging" / self.sketch.name)
        self.assertEqual(self.sketch.active_include(), "sensors/EC_H2S_50ppm.h")

    def test_gas_selection_is_applied_to_the_staged_copy(self) -> None:
        job = self.make_job(FakeRunner())
        staged, _ = job.prepare(self.sketch, self.profile("EC_CO_300ppm"))
        self.assertEqual(staged.active_include(), "sensors/EC_CO_300ppm.h")
        self.assertIn("Active gas    : sensors/EC_CO_300ppm.h", self.lines)

    def test_second_unit_with_same_gas_skips_the_compile(self) -> None:
        runner = FakeRunner()
        job = self.make_job(runner)
        profile = self.profile("EC_CO_300ppm")
        job.build_and_upload(self.sketch, profile, "COM7")
        job.build_and_upload(self.sketch, profile, "COM8")
        self.assertEqual([c[0] for c in runner.calls], ["compile", "upload", "upload"])

    def test_different_gases_get_different_artifact_folders(self) -> None:
        runner = FakeRunner()
        job = self.make_job(runner)
        first = job.build_and_upload(self.sketch, self.profile("EC_CO_300ppm"), "COM7")
        second = job.build_and_upload(self.sketch, self.profile("EC_H2S_50ppm"), "COM7")
        self.assertNotEqual(first, second)
        self.assertEqual([c[0] for c in runner.calls], ["compile", "upload", "compile", "upload"])

    def test_compile_failure_stops_before_upload(self) -> None:
        runner = FakeRunner(compile_code=1)
        job = self.make_job(runner)
        with self.assertRaises(ToolchainError):
            job.build_and_upload(self.sketch, self.profile("EC_CO_300ppm"), "COM7")
        self.assertEqual([c[0] for c in runner.calls], ["compile"])

    def test_failure_message_carries_the_hint(self) -> None:
        class MissingCoreRunner(FakeRunner):
            def compile(self, sketch_dir, fqbn, output_dir, build_path, sink):
                sink("Error during build: Platform 'esp32:esp32' not found: platform not installed")
                return 1

        job = self.make_job(MissingCoreRunner())
        with self.assertRaises(ToolchainError) as ctx:
            job.build_and_upload(self.sketch, self.profile("EC_CO_300ppm"), "COM7")
        self.assertIn("Install / update ESP32 toolchain", str(ctx.exception))

    def test_compile_without_binary_is_an_error(self) -> None:
        runner = FakeRunner(emit_binary=False)
        job = self.make_job(runner)
        with self.assertRaises(ToolchainError):
            job.build_and_upload(self.sketch, self.profile("EC_CO_300ppm"), "COM7")

    def test_upload_failure_is_reported(self) -> None:
        runner = FakeRunner(upload_code=2)
        job = self.make_job(runner)
        with self.assertRaises(ToolchainError):
            job.build_and_upload(self.sketch, self.profile("EC_CO_300ppm"), "COM7")

    def test_upload_without_port_is_refused(self) -> None:
        job = self.make_job(FakeRunner())
        with self.assertRaises(ToolchainError):
            job.build_and_upload(self.sketch, self.profile("EC_CO_300ppm"), "")

    def test_upload_only_without_a_build_is_refused(self) -> None:
        job = self.make_job(FakeRunner())
        with self.assertRaises(ToolchainError) as ctx:
            job.upload_only(self.sketch, self.profile("EC_CO_300ppm"), "COM7")
        self.assertIn("No compiled firmware", str(ctx.exception))

    def test_cancel_aborts_the_job(self) -> None:
        runner = FakeRunner()
        job = self.make_job(runner)
        job.cancel()
        with self.assertRaises(JobCancelled):
            job.build_and_upload(self.sketch, self.profile("EC_CO_300ppm"), "COM7")
        self.assertTrue(runner.cancelled)

    def test_flash_log_records_the_unit(self) -> None:
        job = self.make_job(FakeRunner())
        profile = self.profile("EC_CO_300ppm")
        job.build_and_upload(self.sketch, profile, "COM7")
        job.record(self.sketch, profile, "COM7", "ok")

        with Paths(self.settings.work_path).flash_log().open(encoding="utf-8") as handle:
            rows = list(csv.DictReader(handle))
        self.assertEqual(len(rows), 1)
        row = rows[0]
        self.assertEqual(row["unit_id"], "SN-0001")
        self.assertEqual(row["gas"], "CO")
        self.assertEqual(row["profile"], "EC_CO_300ppm")
        self.assertEqual(row["bt_name"], "ESP32 EC-CO")
        self.assertEqual(row["fw_version"], "M30 VerK 05/31/2026")
        self.assertEqual(row["port"], "COM7")
        self.assertEqual(row["result"], "ok")

    def test_setup_installs_core_and_every_library(self) -> None:
        runner = FakeRunner()
        job = self.make_job(runner)
        job.setup_toolchain()
        self.assertEqual(runner.calls[0], ("update_index",))
        self.assertEqual(runner.calls[1], ("install_core", self.settings.core))
        installed = [c[1] for c in runner.calls if c[0] == "install_library"]
        self.assertEqual(installed, self.settings.libraries)


if __name__ == "__main__":
    unittest.main()
