"""Parsing of M30 sensor profile headers (``inc/sensors/*.h``).

Each header is a "gas" the analyser can be built for: it defines the
``SENSOR_*`` macros that the .ino turns into the gas library entry, the
range / alarm / PEL / cal-value menu ladders, the ADS1115 gain and the
Bluetooth device name.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path

# Families, in the order they should appear in the picker.
FAMILY_ORDER = ["PID", "EC", "NDIR", "TCD", "OTHER"]

FAMILY_LABELS = {
    "PID": "PID (photoionisation)",
    "EC": "Electrochemical",
    "NDIR": "NDIR (infrared)",
    "TCD": "TCD (thermal conductivity)",
    "OTHER": "Other",
}

_DEFINE_RE = re.compile(r"^\s*#define\s+(SENSOR_[A-Z0-9_]+)\s*(.*)$")
_TITLE_RE = re.compile(r"^\s*//\s*Sensor Profile:\s*(.+?)\s*$")
_NOTE_RE = re.compile(
    r"^\s*//\s*(Sensor type|Part number|Unit|Resolution|Over-range|OSHA PEL|"
    r"NIOSH REL|ACGIH TLV|IDLH|Hardware|Output|Last updated)\s*:\s*(.+?)\s*$"
)

# Full-scale voltage of each ADS1115 gain setting, for the details panel.
ADS_GAIN_INFO = {
    "GAIN_TWOTHIRDS": "+/-6.144 V (0.1875 mV/LSB)",
    "GAIN_ONE": "+/-4.096 V (0.1250 mV/LSB)",
    "GAIN_TWO": "+/-2.048 V (0.0625 mV/LSB)",
    "GAIN_FOUR": "+/-1.024 V (0.0313 mV/LSB)",
    "GAIN_EIGHT": "+/-0.512 V (0.0156 mV/LSB)",
    "GAIN_SIXTEEN": "+/-0.256 V (0.0078 mV/LSB)",
}


def _strip_line_comment(value: str) -> str:
    """Drop a trailing ``//`` comment, ignoring ``//`` inside a string literal."""
    in_string = False
    i = 0
    while i < len(value):
        ch = value[i]
        if ch == '"' and (i == 0 or value[i - 1] != "\\"):
            in_string = not in_string
        elif not in_string and value.startswith("//", i):
            return value[:i]
        i += 1
    return value


def _parse_defines(text: str) -> dict[str, str]:
    defines: dict[str, str] = {}
    pending_name: str | None = None
    pending_value = ""
    for raw in text.splitlines():
        if pending_name is not None:
            chunk = raw.rstrip()
            more = chunk.endswith("\\")
            pending_value += " " + _strip_line_comment(chunk.rstrip("\\")).strip()
            if not more:
                defines[pending_name] = pending_value.strip()
                pending_name = None
            continue
        m = _DEFINE_RE.match(raw)
        if not m:
            continue
        name, value = m.group(1), m.group(2).rstrip()
        if value.endswith("\\"):
            pending_name = name
            pending_value = _strip_line_comment(value.rstrip("\\")).strip()
            continue
        defines[name] = _strip_line_comment(value).strip()
    if pending_name is not None:  # unterminated continuation
        defines[pending_name] = pending_value.strip()
    return defines


def _as_string(value: str | None) -> str:
    if not value:
        return ""
    m = re.search(r'"(.*)"', value)
    return m.group(1) if m else value.strip()


def _as_float(value: str | None) -> float | None:
    if not value:
        return None
    m = re.search(r"[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?", value)
    return float(m.group(0)) if m else None


def _as_int(value: str | None, default: int) -> int:
    f = _as_float(value)
    return default if f is None else int(f)


def _as_float_list(value: str | None) -> list[float]:
    if not value:
        return []
    inner = value.strip()
    if inner.startswith("{"):
        inner = inner[1:]
    if inner.endswith("}"):
        inner = inner[:-1]
    out: list[float] = []
    for part in inner.split(","):
        f = _as_float(part)
        if f is not None:
            out.append(f)
    return out


def format_value(value: float, digits: int) -> str:
    """Render a menu value the way the firmware does (``String(v, digits)``)."""
    return f"{value:.{max(0, digits)}f}"


def compact_value(value: float) -> str:
    """Render a number without pointless trailing zeros (100.0 -> ``100``)."""
    text = f"{value:.4f}".rstrip("0").rstrip(".")
    return text or "0"


@dataclass(frozen=True)
class GasProfile:
    """One selectable gas / sensor build of the M30."""

    key: str  # file stem, e.g. "EC_CO_300ppm"
    path: Path
    include_path: str  # "sensors/EC_CO_300ppm.h" — as written in SensorConfig.h
    gas: str
    unit: str
    bt_name: str
    ads_gain: str
    res_digits: int
    range_fs: float | None
    overrange: float | None
    ranges: list[float] = field(default_factory=list)
    calvalues: list[float] = field(default_factory=list)
    alarms: list[float] = field(default_factory=list)
    pels: list[float] = field(default_factory=list)
    slots: list[str] = field(default_factory=list)  # Gas("...") slots documented in the header
    title: str = ""
    notes: list[tuple[str, str]] = field(default_factory=list)
    is_template: bool = False

    @property
    def display_gas(self) -> str:
        """Gas name for the picker — composite heads list every cell."""
        if len(self.slots) > 1 and self.gas in self.slots:
            return "+".join(self.slots)
        return self.gas

    @property
    def family(self) -> str:
        head = self.key.split("_", 1)[0].upper()
        return head if head in FAMILY_ORDER else "OTHER"

    @property
    def range_label(self) -> str:
        """e.g. ``0-300 ppm``."""
        if self.range_fs is None:
            return "-"
        return f"0-{compact_value(self.range_fs)} {self.unit}".rstrip()

    @property
    def label(self) -> str:
        """Single-line label used in the picker and in log lines."""
        return f"{self.gas} {self.range_label}".strip()

    def sort_key(self) -> tuple:
        family_rank = FAMILY_ORDER.index(self.family)
        return (family_rank, self.gas.upper(), self.range_fs or 0.0, self.key)

    def details(self) -> str:
        """Human-readable summary shown in the details pane."""
        d = self.res_digits
        lines: list[str] = []
        if self.title:
            lines.append(self.title)
            lines.append("")
        lines.append(f"Profile file   : inc/{self.include_path}")
        lines.append(f"Gas name       : {self.gas}")
        if len(self.slots) > 1:
            lines.append(f"Gas slots      : {', '.join(self.slots)}  (LIBRARY menu)")
        lines.append(f"Full scale     : {self.range_label}")
        if self.overrange is not None:
            lines.append(f"Over-range     : {format_value(self.overrange, d)} {self.unit}")
        lines.append(f"Resolution     : {format_value(10 ** -d, d)} {self.unit}")
        lines.append(f"Bluetooth name : {self.bt_name}")
        gain_note = ADS_GAIN_INFO.get(self.ads_gain, "")
        lines.append(f"ADS1115 gain   : {self.ads_gain}{'  ' + gain_note if gain_note else ''}")
        lines.append("")
        lines.append("Menu ladders built into this firmware")
        for name, values in (
            ("Range", self.ranges),
            ("Cal value", self.calvalues),
            ("Alarm (high)", self.alarms),
            ("PEL (low)", self.pels),
        ):
            if not values:
                lines.append(f"  {name:<13}: (profile default)")
                continue
            rendered = ", ".join(
                "Off" if v == 0 and name in ("Alarm (high)", "PEL (low)") else format_value(v, d)
                for v in values
            )
            lines.append(f"  {name:<13}: {rendered}  {self.unit}")
        if self.notes:
            lines.append("")
            lines.append("Profile header notes")
            for key, value in self.notes:
                lines.append(f"  {key:<13}: {value}")
        return "\n".join(lines)


def parse_profile(path: Path) -> GasProfile:
    """Parse one ``inc/sensors/*.h`` sensor profile header."""
    text = path.read_text(encoding="utf-8", errors="replace")
    defines = _parse_defines(text)

    title = ""
    notes: list[tuple[str, str]] = []
    for raw in text.splitlines():
        if not title:
            m = _TITLE_RE.match(raw)
            if m:
                title = m.group(1)
                continue
        m = _NOTE_RE.match(raw)
        if m:
            key, value = m.group(1), m.group(2)
            if not value.startswith("["):  # skip unfilled template placeholders
                notes.append((key, value))

    # The headers document their gas slots as `addGas: Gas("CO", 1.0)` comments;
    # a composite head such as the 4-in-1 lists one per cell.
    slots: list[str] = []
    for name in re.findall(r'Gas\("([^"]+)"', text):
        if name not in slots and not name.startswith("["):
            slots.append(name)

    stem = path.stem
    is_template = "TEMPLATE" in stem.upper()
    gas = _as_string(defines.get("SENSOR_GAS_NAME")) or stem
    unit = _as_string(defines.get("SENSOR_UNIT_LABEL")) or "ppm"

    return GasProfile(
        key=stem,
        path=path,
        include_path=f"sensors/{path.name}",
        gas=gas,
        unit=unit,
        bt_name=_as_string(defines.get("SENSOR_BT_NAME")),
        ads_gain=_as_string(defines.get("SENSOR_ADS_GAIN")) or "GAIN_ONE",
        res_digits=_as_int(defines.get("SENSOR_RES_DIGITS"), 1),
        range_fs=_as_float(defines.get("SENSOR_RANGE_FS")),
        overrange=_as_float(defines.get("SENSOR_OVERRANGE")),
        ranges=_as_float_list(defines.get("SENSOR_RANGE_VALUES")),
        calvalues=_as_float_list(defines.get("SENSOR_CALVALUE_VALUES")),
        alarms=_as_float_list(defines.get("SENSOR_ALARM_VALUES")),
        pels=_as_float_list(defines.get("SENSOR_PEL_VALUES")),
        slots=slots,
        title=title,
        notes=notes,
        is_template=is_template,
    )


def load_profiles(sensors_dir: Path, include_templates: bool = False) -> list[GasProfile]:
    """Parse every profile header in ``sensors_dir``, sorted for display."""
    profiles = [parse_profile(p) for p in sorted(sensors_dir.glob("*.h"))]
    if not include_templates:
        profiles = [p for p in profiles if not p.is_template]
    profiles.sort(key=GasProfile.sort_key)
    return profiles
