#pragma once
// =============================================================================
// M30-EC firmware version — edit ONLY this file to bump the version.
// =============================================================================
// Scheme: MAJOR.MINOR.PATCH[-beta.N]
//   MAJOR  saved-settings layout changed (FW_EEPROM_LAYOUT bumped): units
//          reload defaults and must be recalibrated after updating.
//   MINOR  new feature or new sensor profile, settings kept.
//   PATCH  bug fix only, settings kept.
//   -beta.N  not yet verified on hardware; drop it (FW_PRERELEASE 0) for a
//            release, and tag the commit  m30-ec-vMAJOR.MINOR.PATCH.
// Every version gets a section in CHANGELOG.md.
// =============================================================================

#define FW_PRODUCT        "M30-EC"
#define FW_BOARD          "VerK"          // hardware revision
#define FW_VERSION_MAJOR  1
#define FW_VERSION_MINOR  0
#define FW_VERSION_PATCH  0
#define FW_PRERELEASE     2               // 0 = release, N = beta.N
#define FW_RELEASE_DATE   "2026-11-26"

// Saved-settings (EEPROM) layout. Bump together with MAJOR when the meaning or
// position of stored settings changes; units then reload factory defaults.
#define FW_EEPROM_LAYOUT  1

// --- Derived strings (do not edit) -------------------------------------------
#define FW_STR_(x) #x
#define FW_STR(x)  FW_STR_(x)
#define FW_VERSION_NUM  FW_STR(FW_VERSION_MAJOR) "." FW_STR(FW_VERSION_MINOR) "." FW_STR(FW_VERSION_PATCH)
#if FW_PRERELEASE
  #define FW_VERSION_TEXT   FW_VERSION_NUM "-beta." FW_STR(FW_PRERELEASE)  // 1.0.0-beta.1
  #define FW_VERSION_SHORT  FW_VERSION_NUM "-b" FW_STR(FW_PRERELEASE)      // 1.0.0-b1 (display)
#else
  #define FW_VERSION_TEXT   FW_VERSION_NUM
  #define FW_VERSION_SHORT  FW_VERSION_NUM
#endif
