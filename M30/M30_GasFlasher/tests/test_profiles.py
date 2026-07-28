"""Tests for parsing the M30 sensor profile headers."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from m30flasher.profiles import GasProfile, load_profiles, parse_profile  # noqa: E402

REPO_M30 = Path(__file__).resolve().parents[2]


def real_sensors_dir() -> Path | None:
    candidates = sorted(REPO_M30.glob("M30_VerK_*/inc/sensors"), reverse=True)
    return candidates[0] if candidates else None


class ParseProfileTests(unittest.TestCase):
    def write(self, text: str, name: str = "EC_XX_10ppm.h") -> Path:
        import tempfile

        directory = Path(tempfile.mkdtemp())
        path = directory / name
        path.write_text(text, encoding="utf-8")
        return path

    def test_parses_all_fields(self) -> None:
        path = self.write(
            """#pragma once
// =====================================================
// Sensor Profile: EC - XX (Example), 0-10 ppm
// =====================================================
// Sensor type : Electrochemical
// OSHA PEL    : 5 ppm TWA
#define SENSOR_BT_NAME        "ESP32 EC-XX"
#define SENSOR_GAS_NAME       "XX"
#define SENSOR_UNIT_LABEL     "ppm"
#define SENSOR_RES_DIGITS     2        // 0.01 ppm
#define SENSOR_RANGE_FS       10.0f
#define SENSOR_OVERRANGE      11.0f
#define SENSOR_ADS_GAIN       GAIN_FOUR
#define SENSOR_RANGE_VALUES   { 5.0f, 10.0f }
#define SENSOR_CALVALUE_VALUES { 1.0f, 5.0f }
#define SENSOR_ALARM_VALUES   { 0.0f, 5.0f, 10.0f }   // 5 = PEL
#define SENSOR_PEL_VALUES     { 0.0f, 5.0f }
"""
        )
        profile = parse_profile(path)
        self.assertEqual(profile.gas, "XX")
        self.assertEqual(profile.unit, "ppm")
        self.assertEqual(profile.bt_name, "ESP32 EC-XX")
        self.assertEqual(profile.ads_gain, "GAIN_FOUR")
        self.assertEqual(profile.res_digits, 2)
        self.assertEqual(profile.range_fs, 10.0)
        self.assertEqual(profile.overrange, 11.0)
        self.assertEqual(profile.ranges, [5.0, 10.0])
        self.assertEqual(profile.calvalues, [1.0, 5.0])
        self.assertEqual(profile.alarms, [0.0, 5.0, 10.0])
        self.assertEqual(profile.pels, [0.0, 5.0])
        self.assertEqual(profile.include_path, "sensors/EC_XX_10ppm.h")
        self.assertEqual(profile.family, "EC")
        self.assertEqual(profile.range_label, "0-10 ppm")
        self.assertIn(("OSHA PEL", "5 ppm TWA"), profile.notes)
        self.assertIn("EC - XX (Example), 0-10 ppm", profile.title)

    def test_trailing_comment_is_not_part_of_the_value(self) -> None:
        path = self.write(
            '#define SENSOR_BT_NAME "ESP32 // not a comment"  // real comment\n'
            "#define SENSOR_GAS_NAME \"YY\"\n"
        )
        profile = parse_profile(path)
        self.assertEqual(profile.bt_name, "ESP32 // not a comment")

    def test_line_continuation(self) -> None:
        path = self.write(
            "#define SENSOR_GAS_NAME \"ZZ\"\n"
            "#define SENSOR_ALARM_VALUES { 0.0f, \\\n"
            "                              1.0f, 2.0f }\n"
        )
        profile = parse_profile(path)
        self.assertEqual(profile.alarms, [0.0, 1.0, 2.0])

    def test_missing_gas_name_falls_back_to_file_stem(self) -> None:
        path = self.write("#define SENSOR_BT_NAME \"ESP32\"\n", name="EC_TEMPLATE.h")
        profile = parse_profile(path)
        self.assertEqual(profile.gas, "EC_TEMPLATE")
        self.assertTrue(profile.is_template)

    def test_composite_head_lists_every_slot(self) -> None:
        path = self.write(
            '#define SENSOR_GAS_NAME "CO"\n'
            "// Slot map:\n"
            '//   addGas:    Gas("CO",  1.0)\n'
            '//              Gas("H2S", 1.0)\n'
            '//              Gas("O2",  1.0)\n'
            '//              Gas("LEL", 1.0)\n',
            name="EC_4IN1.h",
        )
        profile = parse_profile(path)
        self.assertEqual(profile.slots, ["CO", "H2S", "O2", "LEL"])
        self.assertEqual(profile.display_gas, "CO+H2S+O2+LEL")
        self.assertIn("Gas slots      : CO, H2S, O2, LEL", profile.details())

    def test_single_gas_head_shows_just_the_gas(self) -> None:
        path = self.write('#define SENSOR_GAS_NAME "CO"\n//   addGas: Gas("CO", 1.0)\n')
        self.assertEqual(parse_profile(path).display_gas, "CO")

    def test_slot_names_that_are_ranges_do_not_replace_the_gas(self) -> None:
        # PID_M10 documents Gas("100") / Gas("1000") — range slots, not gases.
        path = self.write(
            '#define SENSOR_GAS_NAME "VOC"\n'
            '//   addGas: Gas("100", 1.0)\n'
            '//           Gas("1000", 1.0)\n'
        )
        self.assertEqual(parse_profile(path).display_gas, "VOC")

    def test_details_render_off_for_zero_alarm(self) -> None:
        path = self.write(
            '#define SENSOR_GAS_NAME "CO"\n'
            "#define SENSOR_RES_DIGITS 1\n"
            "#define SENSOR_RANGE_FS 300.0f\n"
            "#define SENSOR_ALARM_VALUES { 0.0f, 50.0f }\n"
        )
        details = parse_profile(path).details()
        self.assertIn("Alarm (high) : Off, 50.0", details)


class RealProfileTests(unittest.TestCase):
    """Runs against the actual profile headers shipped in this repository."""

    def setUp(self) -> None:
        self.sensors = real_sensors_dir()
        if not self.sensors:
            self.skipTest("no M30_VerK_* sketch in the repository")

    def test_every_profile_parses_with_a_gas_and_gain(self) -> None:
        profiles = load_profiles(self.sensors)
        self.assertGreater(len(profiles), 10)
        for profile in profiles:
            with self.subTest(profile=profile.key):
                self.assertTrue(profile.gas)
                self.assertTrue(profile.bt_name, "profile has no SENSOR_BT_NAME")
                self.assertTrue(profile.ads_gain.startswith("GAIN_"))
                self.assertIsNotNone(profile.range_fs)
                self.assertTrue(profile.unit)

    def test_templates_hidden_by_default(self) -> None:
        keys = {p.key for p in load_profiles(self.sensors)}
        all_keys = {p.key for p in load_profiles(self.sensors, include_templates=True)}
        self.assertNotIn("EC_TEMPLATE", keys)
        self.assertIn("EC_TEMPLATE", all_keys)

    def test_profiles_sorted_by_family_then_gas(self) -> None:
        profiles = load_profiles(self.sensors)
        self.assertEqual(profiles, sorted(profiles, key=GasProfile.sort_key))
        self.assertEqual(profiles[0].family, "PID")


if __name__ == "__main__":
    unittest.main()
