"""Persisted user settings for the M30 Gas Flasher."""

from __future__ import annotations

import json
import os
import sys
from dataclasses import asdict, dataclass, field
from pathlib import Path

APP_NAME = "M30GasFlasher"

# ESP32 Dev Module.  "No OTA (2MB APP / 2MB SPIFFS)" matches the partition
# scheme the M-series firmware is built with — the sketch needs BluetoothSerial
# and SPIFFS, which do not fit the default 1.2MB app partition.
DEFAULT_FQBN = (
    "esp32:esp32:esp32:"
    "PartitionScheme=no_ota,"
    "CPUFreq=240,"
    "FlashFreq=80,"
    "FlashMode=qio,"
    "FlashSize=4M,"
    "UploadSpeed=921600"
)

BOARD_PRESETS = {
    "ESP32 Dev Module - No OTA (2MB APP/2MB SPIFFS)": DEFAULT_FQBN,
    "ESP32 Dev Module - Huge App (3MB no OTA/1MB SPIFFS)": DEFAULT_FQBN.replace(
        "PartitionScheme=no_ota", "PartitionScheme=huge_app"
    ),
    "ESP32 Dev Module - Minimal SPIFFS (1.9MB APP)": DEFAULT_FQBN.replace(
        "PartitionScheme=no_ota", "PartitionScheme=min_spiffs"
    ),
    "Adafruit ESP32 Feather / HUZZAH32": "esp32:esp32:featheresp32:PartitionScheme=no_ota",
}

# Core and libraries the M30 sketch needs.  Versions are pinned because the
# firmware uses the 1.x Adafruit ADS1X15 API (<Adafruit_ADS1015.h>) and the
# 2.x ESP32 core; leaving them floating silently breaks the build.
DEFAULT_CORE = "esp32:esp32@2.0.17"
DEFAULT_LIBRARIES = [
    "Adafruit ADS1X15@1.1.1",
    "Adafruit INA219",
    "Adafruit SHT4x Library",
    "ESP8266 and ESP32 OLED driver for SSD1306 displays",
    "U8g2",
    "ArduinoJson@6.21.5",
    "MQTT",
    "RTClib",
]


def app_data_dir() -> Path:
    """Per-user directory for settings, staging, build cache and logs."""
    if sys.platform.startswith("win"):
        base = os.environ.get("LOCALAPPDATA") or os.environ.get("APPDATA")
        root = Path(base) if base else Path.home() / "AppData" / "Local"
    elif sys.platform == "darwin":
        root = Path.home() / "Library" / "Application Support"
    else:
        root = Path(os.environ.get("XDG_DATA_HOME", Path.home() / ".local" / "share"))
    return root / APP_NAME


@dataclass
class Settings:
    """Everything the user can configure, persisted as JSON."""

    sketch_dir: str = ""
    fqbn: str = DEFAULT_FQBN
    port: str = ""
    arduino_cli: str = ""
    core: str = DEFAULT_CORE
    libraries: list[str] = field(default_factory=lambda: list(DEFAULT_LIBRARIES))
    last_profile: str = ""
    unit_id: str = ""
    work_dir: str = ""
    show_templates: bool = False

    @property
    def work_path(self) -> Path:
        return Path(self.work_dir) if self.work_dir else app_data_dir()

    @classmethod
    def path(cls) -> Path:
        return app_data_dir() / "settings.json"

    @classmethod
    def load(cls) -> "Settings":
        try:
            raw = json.loads(cls.path().read_text(encoding="utf-8"))
        except (OSError, ValueError):
            return cls()
        known = {f for f in cls().__dict__}
        return cls(**{k: v for k, v in raw.items() if k in known})

    def save(self) -> None:
        path = self.path()
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps(asdict(self), indent=2), encoding="utf-8")
        except OSError:
            pass  # a read-only profile directory must not break flashing
