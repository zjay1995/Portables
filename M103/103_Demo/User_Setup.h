// ─────────────────────────────────────────────────────────────────────────────
//  TFT_eSPI User_Setup.h  —  103 Demo on Adafruit HUZZAH32
//
//  INSTRUCTIONS
//  ─────────────
//  Copy this file into your TFT_eSPI library folder, replacing the existing
//  User_Setup.h.  Arduino IDE path example:
//    Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
//
//  HUZZAH32 wiring
//  ────────────────────────────────────────────────────────────────
//  TFT pin   │ Wire to      │ HUZZAH32 label
//  ──────────┼──────────────┼──────────────────────
//  MISO      │ GPIO 19      │ 19  (MISO)
//  MOSI      │ GPIO 18      │ 18  (MOSI)
//  SCLK      │ GPIO  5      │  5  (SCK)
//  CS        │ GPIO 26      │ 26  (A0)
//  DC        │ GPIO 27      │ 27  (A10)
//  RST       │ GPIO  4      │  4  (A5)
//  VCC       │ 3.3 V        │ 3V
//  GND       │ GND          │ GND
// ─────────────────────────────────────────────────────────────────────────────

#define ILI9488_DRIVER        // 3.5" 320x480 panel (same panel as M103)

#define TFT_MISO 19
#define TFT_MOSI 18
#define TFT_SCLK  5
#define TFT_CS   26
#define TFT_DC   27
#define TFT_RST   4

#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// SPI frequency — ILI9488 max ~20 MHz writes; 8 MHz reads
#define SPI_FREQUENCY       20000000
#define SPI_READ_FREQUENCY   8000000

// Fonts built into TFT_eSPI
#define LOAD_GLCD    // Font 1 — built-in 8px
#define LOAD_FONT2   // Font 2 — small proportional
#define LOAD_FONT4   // Font 4 — medium proportional
#define LOAD_FONT6   // Font 6 — large numeric
#define LOAD_FONT7   // Font 7 — 7-segment numeric
#define LOAD_FONT8   // Font 8 — large numeric
#define LOAD_GFXFF   // FreeFonts

#define SMOOTH_FONT
