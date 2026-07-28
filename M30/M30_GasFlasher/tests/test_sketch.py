"""Tests for sketch discovery, gas selection and staging."""

from __future__ import annotations

import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from m30flasher.sketch import (  # noqa: E402
    Sketch,
    SketchError,
    active_includes,
    discover_sketches,
    known_includes,
    select_profile,
    stage,
)

REPO_M30 = Path(__file__).resolve().parents[2]

SELECTOR = """#pragma once
// --- Uncomment ONE profile ---

// PID
// #include "sensors/PID_M10_100_1000.h"          // PID 100/1000 ppm dual range

// Electrochemical
//#include "sensors/EC_NH3_100ppm.h"           // NH3   0-100 ppm
 #include "sensors/EC_H2S_2000ppm.h"          // H2S   0-2,000 ppm
// #include "sensors/EC_CO_300ppm.h"            // CO    0-300 ppm

#ifndef SENSOR_BT_NAME
  #error "no profile"
#endif
"""


class SelectProfileTests(unittest.TestCase):
    def test_reads_the_single_active_include(self) -> None:
        self.assertEqual(active_includes(SELECTOR), ["sensors/EC_H2S_2000ppm.h"])

    def test_lists_every_known_include(self) -> None:
        self.assertEqual(
            known_includes(SELECTOR),
            [
                "sensors/PID_M10_100_1000.h",
                "sensors/EC_NH3_100ppm.h",
                "sensors/EC_H2S_2000ppm.h",
                "sensors/EC_CO_300ppm.h",
            ],
        )

    def test_selecting_swaps_exactly_one_include(self) -> None:
        out = select_profile(SELECTOR, "sensors/EC_CO_300ppm.h")
        self.assertEqual(active_includes(out), ["sensors/EC_CO_300ppm.h"])
        self.assertEqual(known_includes(out), known_includes(SELECTOR))

    def test_selection_is_idempotent_and_reversible(self) -> None:
        once = select_profile(SELECTOR, "sensors/EC_CO_300ppm.h")
        twice = select_profile(once, "sensors/EC_CO_300ppm.h")
        self.assertEqual(once, twice)
        back = select_profile(once, "sensors/EC_H2S_2000ppm.h")
        self.assertEqual(active_includes(back), ["sensors/EC_H2S_2000ppm.h"])

    def test_trailing_comments_and_surrounding_lines_survive(self) -> None:
        out = select_profile(SELECTOR, "sensors/EC_CO_300ppm.h")
        self.assertIn("// CO    0-300 ppm", out)
        self.assertIn("// H2S   0-2,000 ppm", out)
        self.assertIn('#error "no profile"', out)
        self.assertIn("// --- Uncomment ONE profile ---", out)

    def test_commented_include_without_space_is_recognised_as_inactive(self) -> None:
        out = select_profile(SELECTOR, "sensors/EC_NH3_100ppm.h")
        self.assertEqual(active_includes(out), ["sensors/EC_NH3_100ppm.h"])

    def test_line_endings_are_preserved(self) -> None:
        crlf = SELECTOR.replace("\n", "\r\n")
        out = select_profile(crlf, "sensors/EC_CO_300ppm.h")
        self.assertEqual(out.count("\r\n"), crlf.count("\r\n"))
        self.assertEqual(out.count("\n"), out.count("\r\n"), "a lone LF was introduced")

    def test_unknown_profile_is_rejected(self) -> None:
        with self.assertRaises(SketchError):
            select_profile(SELECTOR, "sensors/EC_NOT_THERE.h")


class FakeSketchTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.tmp, True)
        self.root = self.tmp / "M30_VerK_20260531"
        (self.root / "inc" / "sensors").mkdir(parents=True)
        (self.root / f"{self.root.name}.ino").write_text(
            '#define FW_VERSION "M30 VerK 05/31/2026"\n', encoding="utf-8"
        )
        (self.root / "inc" / "SensorConfig.h").write_text(SELECTOR, encoding="utf-8")
        for name, gas in (
            ("EC_CO_300ppm.h", "CO"),
            ("EC_H2S_2000ppm.h", "H2S"),
            ("EC_NH3_100ppm.h", "NH3"),
            ("PID_M10_100_1000.h", "VOC"),
        ):
            (self.root / "inc" / "sensors" / name).write_text(
                f'#define SENSOR_GAS_NAME "{gas}"\n'
                f'#define SENSOR_BT_NAME "ESP32 {gas}"\n'
                "#define SENSOR_RANGE_FS 100.0f\n"
                "#define SENSOR_ADS_GAIN GAIN_FOUR\n",
                encoding="utf-8",
            )
        (self.root / ".git").mkdir()
        (self.root / ".git" / "HEAD").write_text("ref: refs/heads/main\n", encoding="utf-8")
        self.sketch = Sketch(self.root)

    def test_validate_and_metadata(self) -> None:
        self.sketch.validate()
        self.assertEqual(self.sketch.firmware_version(), "M30 VerK 05/31/2026")
        self.assertEqual(self.sketch.active_include(), "sensors/EC_H2S_2000ppm.h")

    def test_missing_pieces_are_reported(self) -> None:
        (self.root / "inc" / "SensorConfig.h").unlink()
        with self.assertRaises(SketchError):
            self.sketch.validate()

    def test_select_writes_the_file(self) -> None:
        profile = next(p for p in self.sketch.profiles() if p.key == "EC_CO_300ppm")
        self.sketch.select(profile)
        self.assertEqual(self.sketch.active_include(), "sensors/EC_CO_300ppm.h")

    def test_staging_copies_sources_and_leaves_the_original_alone(self) -> None:
        staged = stage(self.sketch, self.tmp / "work")
        self.assertEqual(staged.name, self.sketch.name)
        self.assertFalse((staged.root / ".git").exists(), ".git must not be staged")

        profile = next(p for p in staged.profiles() if p.key == "EC_CO_300ppm")
        staged.select(profile)
        self.assertEqual(staged.active_include(), "sensors/EC_CO_300ppm.h")
        self.assertEqual(
            self.sketch.active_include(),
            "sensors/EC_H2S_2000ppm.h",
            "the repository copy must never be modified",
        )

    def test_staging_twice_clears_stale_files(self) -> None:
        staged = stage(self.sketch, self.tmp / "work")
        (staged.root / "inc" / "sensors" / "STALE.h").write_text("x", encoding="utf-8")
        staged = stage(self.sketch, self.tmp / "work")
        self.assertFalse((staged.root / "inc" / "sensors" / "STALE.h").exists())

    def test_fingerprint_tracks_the_selected_gas(self) -> None:
        staged = stage(self.sketch, self.tmp / "work")
        before = staged.fingerprint()
        self.assertEqual(before, staged.fingerprint(), "fingerprint must be stable")
        staged.select(next(p for p in staged.profiles() if p.key == "EC_CO_300ppm"))
        self.assertNotEqual(before, staged.fingerprint())

    def test_predicted_fingerprint_matches_the_staged_one(self) -> None:
        # The UI looks up cached builds with this prediction; it has to equal
        # what staging + select() actually produces, or the cache never hits.
        predicted = self.sketch.fingerprint("sensors/EC_CO_300ppm.h")
        staged = stage(self.sketch, self.tmp / "work")
        staged.select(next(p for p in staged.profiles() if p.key == "EC_CO_300ppm"))
        self.assertEqual(predicted, staged.fingerprint())

    def test_predicted_fingerprints_differ_per_gas(self) -> None:
        self.assertNotEqual(
            self.sketch.fingerprint("sensors/EC_CO_300ppm.h"),
            self.sketch.fingerprint("sensors/EC_NH3_100ppm.h"),
        )

    def test_discovery_finds_the_sketch(self) -> None:
        found = discover_sketches([self.tmp])
        self.assertEqual([s.name for s in found], ["M30_VerK_20260531"])

    def test_discovery_ignores_non_sketch_folders(self) -> None:
        (self.tmp / "not_a_sketch").mkdir()
        found = discover_sketches([self.tmp])
        self.assertEqual(len(found), 1)


class RealRepositoryTests(unittest.TestCase):
    def setUp(self) -> None:
        self.sketches = discover_sketches([REPO_M30])
        if not self.sketches:
            self.skipTest("no M30 sketch found in the repository")

    def test_repository_sketches_are_usable(self) -> None:
        for sketch in self.sketches:
            with self.subTest(sketch=sketch.name):
                sketch.validate()
                self.assertTrue(sketch.profiles())

    def test_predicted_fingerprint_matches_staging_on_real_sources(self) -> None:
        # The real SensorConfig.h is UTF-8 with en dashes — the prediction has
        # to survive that round trip, otherwise the build cache never hits.
        tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, tmp, True)
        sketch = self.sketches[0]
        listed = set(known_includes(sketch.sensor_config.read_text(encoding="utf-8")))
        target = sorted(listed)[0]

        predicted = sketch.fingerprint(target)
        staged = stage(sketch, tmp)
        staged.select(next(p for p in staged.profiles(True) if p.include_path == target))
        self.assertEqual(predicted, staged.fingerprint())

    def test_every_profile_header_can_be_selected(self) -> None:
        tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, tmp, True)
        sketch = self.sketches[0]
        staged = stage(sketch, tmp)
        text = staged.sensor_config.read_text(encoding="utf-8")
        listed = set(known_includes(text))
        selectable = [p for p in staged.profiles(include_templates=True) if p.include_path in listed]
        self.assertTrue(selectable, "SensorConfig.h lists none of the profile headers")
        for profile in selectable:
            with self.subTest(profile=profile.key):
                staged.select(profile)
                self.assertEqual(staged.active_include(), profile.include_path)


if __name__ == "__main__":
    unittest.main()
