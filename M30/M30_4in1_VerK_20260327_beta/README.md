# M30 4Gas — Firmware Design Document

**FW Version:** VerG-20260514

---

## 1. System Overview

The M30 4Gas is an ESP32-based 4-gas PID analyzer measuring CO, H2S, O2, and CH4. It uses a composite menu system rendered on a 128×64 SSD1306 OLED display. User interaction is via three buttons (LEFT/RIGHT cycle menus, S-button confirms actions). Gas readings arrive via UART serial from a sensor board; calibration coefficients (slope, intercept) and settings are persisted in EEPROM.

---

## 2. Navigation Map

Horizontal menu index (cycled by Mode/RIGHT button):

| Idx | Screen Name | Renderer Class |
|-----|-------------|----------------|
| 0 | Run / Main | `SSD1306RunMenuRenderer` |
| 1 | Library (Gas Select) | `SSD1306GasMenuRenderer` |
| 2 | Zero Cal | `SSD1306ZEROMenuRenderer` |
| 3 | Cal Gas Select | `SSD1306GasMenuRenderer` |
| 4 | Span Cal | `SSD1306CalGasMenuRenderer` |
| 5 | Cal Result | `SSD1306CalResMenuRenderer` |
| 6 | Range | `SSD1306RangeMenuRenderer` |
| 7 | Alarm | `SSD1306AlarmMenuRenderer` |
| 8 | Low Alarm | `SSD1306LowalarmMenuRenderer` |
| 9 | Cal Value | `SSD1306CalvalueMenuRenderer` |
| 10 | Set Hour | `SSD1306HourMenuRenderer` |
| 11 | Set Minute | `SSD1306MinuteMenuRenderer` |
| 12 | Output Port | `SSD1306OutportMenuRenderer` |
| 13 | Cal Info (read-only) | `SSD1306CalInfoMenuRenderer` |

**Navigation rules:**
- **LEFT** — previous item within current screen
- **RIGHT / Mode** — advance to next screen (0 → 1 → … → 12 → 13 → 0); idx 1 also accessible via Library shortcut
- **S** — confirm / execute action on current screen

---

## 3. Screen Layouts

### 3.1 Screen 0 — Run (Main Display)

```
+--------------------------------+
| CO   H2S  O2   CH4             |
|  25    0  20.9   0  [unit]     |
|                                |
| Batt: 85%   [Gas Name]         |
|________________________________|
| [alarm status bar]             |
+--------------------------------+
```

Header: gas names. Row 2: live readings. Row 3: battery %, selected gas. Footer: alarm indicator.

---

### 3.2 Screen 1 — Library (Gas Select)

```
+--------------------------------+
| LIBRARY                        |
|--------------------------------|
| > CO                           |
|   H2S                          |
|   O2                           |
|   CH4                          |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

LEFT cycles gas options; S selects the active gas.

---

### 3.3 Screen 2 — Zero Cal

```
+--------------------------------+
| ZERO CAL                       |
|--------------------------------|
| Gas:  CO                       |
| Raw:  0.452 V                  |
| Conc: 24.8 ppm                 |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

S runs zero calibration for the selected gas. Banner: **ZERO SET** on pass, **CAL FAILED** if ±50% guard rejects.

---

### 3.4 Screen 3 — Cal Gas Select

```
+--------------------------------+
| CAL GAS SELECT                 |
|--------------------------------|
| > CO                           |
|   H2S                          |
|   O2                           |
|   CH4                          |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

Inserted before span cal. S selects which gas drives the span cal. Shares `GasMenuItem` vector with Library.

---

### 3.5 Screen 4 — Span Cal

```
+--------------------------------+
| CAL GAS                        |
|--------------------------------|
| Gas:  CO                       |
| Raw:  1.234 V                  |
| Target: 50 ppm                 |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

S runs span calibration for all 4 gases. Banner: **CAL COMPLETE** on pass, **CAL FAILED** if any gas is rejected by the ±50% guard.

---

### 3.6 Screen 5 — Cal Result

```
+--------------------------------+
| CAL RESULT                     |
|--------------------------------|
| CO  slope: 0.0234              |
| H2S slope: 0.0198              |
| Last cal: 2026-05-14 09:30     |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

Read-only. Shows updated slopes and the RTC timestamp of the last successful span cal.

---

### 3.7 Screen 6 — Range

```
+--------------------------------+
| RANGE                          |
|--------------------------------|
| Gas: CO                        |
| > 100 ppm                      |
|   500 ppm                      |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

LEFT cycles range options; S sets range for the selected gas.

---

### 3.8 Screen 7 — Alarm (High)

```
+--------------------------------+
| ALARM                          |
|--------------------------------|
| Gas: CO                        |
| > 25 ppm                       |
|   50 ppm                       |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

LEFT cycles alarm threshold options; S sets the high alarm for the selected gas.

---

### 3.9 Screen 8 — Low Alarm

```
+--------------------------------+
| LOW ALARM                      |
|--------------------------------|
| Gas: CO                        |
| > 10 ppm                       |
|   20 ppm                       |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

LEFT cycles low alarm options; S sets the low alarm for the selected gas.

---

### 3.10 Screen 9 — Cal Value

```
+--------------------------------+
| CALVALUE                       |
|--------------------------------|
| > 25 ppm                       |
|   50 ppm                       |
|   100 ppm                      |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

Sets the target concentration used during span calibration.

---

### 3.11 Screen 10 — Set Hour

```
+--------------------------------+
| TIME : HOUR                    |
|--------------------------------|
| > 00                           |
|   01                           |
|   ...                          |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

S writes the selected hour to the PCF8563 RTC and EEPROM (addr 220).

---

### 3.12 Screen 11 — Set Minute

```
+--------------------------------+
| TIME : MINUTE                  |
|--------------------------------|
| > 00                           |
|   01                           |
|   ...                          |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

S writes the selected minute to the PCF8563 RTC and EEPROM (addr 230). Banner: **TIME SAVED**.

---

### 3.13 Screen 12 — Output Port

```
+--------------------------------+
| OUTPUT                         |
|--------------------------------|
| > 4-20mA                       |
|   0-10V                        |
|________________________________|
| OPTION        SET        MENU  |
+--------------------------------+
```

Selects the analog output type.

---

### 3.14 Screen 13 — Cal Info (Read-Only)

```
+--------------------------------+
| CAL INFO                       |
|--------------------------------|
| CO:  s=0.0234  H2S: s=0.0198  |
| O2:  s=1.0000  CH4: s=0.0156  |
| Last: 2026-05-14 09:30         |
| FW: VerG-20260514              |
+--------------------------------+
```

Displays all 4 slopes, the cal timestamp from EEPROM addr 88, and the firmware version string. No S-button action.

---

## 4. Modified Units — Function Summary

### 4.1 `M30_4in1_VerK_20260327_beta.ino`

| Function / Block | Change |
|-----------------|--------|
| `#define FW_VERSION` | Set to `"VerG-20260514"` |
| `setup()` — splash | Logo 2000 ms, then version screen 1500 ms |
| `showSavedBanner()` | New helper: clears display, shows centred 16 pt message for 700 ms |
| S-button handler | Context-aware banners per screen index; CAL FAILED check via `wasLastCalFailed()` |
| `horizontalMenus[]` | 14 entries (idx 0–13); `calGasSelectMenu` added at idx 3 |
| RIGHT / Mode button | Wraps at idx 13 back to 0; Library accessible via dedicated shortcut |
| WiFi / MQTT | All `WiFi.h`, `WebServer`, MQTT includes and code removed |

### 4.2 `inc/GasManager.h`

| Function / Member | Change |
|------------------|--------|
| `#include <RTClib.h>` | Added for PCF8563 timestamp in `calibrate2()` |
| `EEPROM_CAL_TIMESTAMP_OFFSET` | Defined as `88` (free gap 88–119) |
| `m_lastCalFailed` | New `bool` member; tracks calibration rejection |
| `wasLastCalFailed()` | Returns `m_lastCalFailed` |
| `resetCalFlag()` | Clears `m_lastCalFailed`; called before batch span cal |
| `calibrate()` | ±50% guard added; sets flag and returns early on failure |
| `calibrate2()` | ±50% guard added; writes RTC timestamp to EEPROM addr 88 on success |

### 4.3 `inc/Menu.h`

| Class / Method | Change |
|----------------|--------|
| `CompositeMenu::moveToNext()` | Wrap boundary changed to 5 for `"Main Menu"` (was 4) |
| `CompositeMenu::action()` | Same wrap fix; `"CAL GAS SELECT"` added to stay-put list |
| `CalMenuItemCalGas::action()` | Calls `resetCalFlag()` before batch `calibrate2()` calls |
| WiFi menu item classes | Removed: `WiFiDumpMenuItem`, `WiFiRealTimeDumpMenuItem`, `NTPSyncMenuItem` |

### 4.4 `inc/MenuRenderer.h`

| Class | Change |
|-------|--------|
| `SSD1306CalInfoMenuRenderer` | New class; holds `GasManager*` and `const char* fwVersion` |
| WiFi renderer classes | Removed: `SSD1306WiFiDumpMenuRenderer`, `SSD1306WiFiRealTimeDumpMenuRenderer`, `SSD1306NTPSyncMenuRenderer` |
| `m_fwVersion` type | `const char*` (not Arduino `String`) to avoid include dependency in plain C++ header |

### 4.5 `MenuRenderer.cpp`

| Renderer | Change |
|----------|--------|
| All settings screens (9 screens) | Footer updated to **OPTION** (left) / **SET** (centre) / **MENU** (right) |
| All `drawLine` calls | Width corrected from 256 → 128 px |
| `SSD1306CalResMenuRenderer` | Now reads and displays cal timestamp from EEPROM addr 88 |
| `SSD1306CalInfoMenuRenderer` | New: draws all 4 slopes, cal timestamp, FW version |
| WiFi renderer implementations | Removed 3 renderer implementations |
| `#include "inc/GasManager.h"` | Added for `EEPROM_CAL_TIMESTAMP_OFFSET` access |

---

## 5. EEPROM Address Map

| Address | Size | Content |
|---------|------|---------|
| 4 | 4 bytes | Selected gas index |
| 8 | 8 bytes | Intercept — CO (`double`) |
| 16 | 8 bytes | Slope — CO (`double`) |
| 24 | 8 bytes | Intercept — H2S (`double`) |
| 32 | 8 bytes | Slope — H2S (`double`) |
| 40 | 8 bytes | Intercept — O2 (`double`) |
| 48 | 8 bytes | Slope — O2 (`double`) |
| 56 | 8 bytes | Intercept — CH4 (`double`) |
| 64 | 8 bytes | Slope — CH4 (`double`) |
| 72 | 16 bytes | Range settings |
| 88 | 32 bytes | Cal timestamp string `"YYYY-MM-DD HH:MM"` |
| 120 | 16 bytes | Low Alarm settings |
| 220 | 4 bytes | RTC Hour |
| 230 | 4 bytes | RTC Minute |

---

## 6. Calibration Guard — ±50% Rule

Both zero and span calibration functions reject a cal attempt if the live reading deviates more than ±50% from the target value.

**Guard formula:** `ratio = currentConc / targetValue`. Pass condition: `0.5 ≤ ratio ≤ 1.5`.

**On failure:**
- `m_lastCalFailed` is set to `true`
- No EEPROM write occurs
- S-button handler calls `wasLastCalFailed()` and shows **CAL FAILED** banner

**Span cal (batch of 4 gases):** `resetCalFlag()` is called once before the batch. Any single gas failure sets the flag; the banner fires if any gas failed.

**Zero cal special case:** Guard is skipped when `targetValue == 0` (raw zero-point cal).

---

## 7. Startup Sequence

1. Power on
2. Display logo bitmap — **2000 ms**
3. Clear; show `"M30 4Gas"` (16 pt) and `"Ver. VerG-20260514"` (10 pt) — **1500 ms**
4. Load EEPROM settings (gas index, slopes, intercepts, alarms, ranges)
5. Sync RTC hour/minute from PCF8563
6. Enter main run loop (Screen 0)
