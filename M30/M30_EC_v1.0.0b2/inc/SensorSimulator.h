#pragma once
// =============================================================================
// DEV-build sensor simulator.
//
// Replaces the ADS1115 detector reading with the output of a modelled
// electrochemical cell, so the whole firmware (zero/span calibration, alarms,
// logging, FAULT / CAL? / OVER) can be exercised on the bench without a sensor
// or gas. Only compiled into BUILD_DEV (see BuildConfig.h).
//
// What the operator does decides what gas the "sensor" sees, like in the field:
//   Cal Zero screen          -> clean (zero) air
//   Cal Gas / Bump Test      -> the selected cal gas concentration
//   anywhere else            -> a field profile: low background with an
//                               exposure event every few minutes (some cross
//                               the alarm setpoints, ~1 in 6 goes over range)
// The cell responds with a first-order lag (T90 ~ 35 s), so readings ramp and
// the operator has to wait for a stable value before pressing S.
//
// Serial / Bluetooth commands (end with newline):
//   SIM AUTO      back to the automatic field profile (default)
//   SIM PPM <x>   hold a fixed concentration of <x> ppm in the field
//   SIM FAULT     detector reads 0 mV (dead / disconnected cell)
//   SIM NOGAS     cal gas not applied: Cal Gas / Bump Test see clean air
//   SIM GAS       cal gas applied again (default)
//   SIM OK        clear SIM FAULT
//   SIM STATUS    print the simulator state
// =============================================================================

#include <Arduino.h>
#include <math.h>
#include "../BuildConfig.h"   // sketch root
#include "SensorConfig.h"

enum SimScene { SIM_FIELD = 0, SIM_ZERO_AIR = 1, SIM_CAL_GAS = 2 };

struct SimState {
    int   scene;              // SimScene, set from the active menu each loop
    float calGasPpm;          // selected cal-gas value (for SIM_CAL_GAS)
    float holdPpm;            // >= 0: SIM PPM override in the field
    bool  forceFault;         // SIM FAULT
    bool  noCalGas;           // SIM NOGAS
    float ppm;                // concentration currently "seen" by the cell
    unsigned long lastMs;
    // field-profile event state
    unsigned long nextEventMs;
    unsigned long eventEndMs;
    float eventPeakPpm;
    bool  inEvent;

    SimState() : scene(SIM_FIELD), calGasPpm(0), holdPpm(-1), forceFault(false), noCalGas(false),
                 ppm(SENSOR_AMBIENT_VALUE), lastMs(0), nextEventMs(45000UL), eventEndMs(0),
                 eventPeakPpm(0), inEvent(false) {}
};

// One shared instance across translation units (function-local static).
inline SimState& simState() { static SimState s; return s; }

// --- Modelled cell (scaled to the active sensor profile) ---------------------
inline float simFullScaleMv()  { return 32767.0f * adsMvPerBit(SENSOR_ADS_GAIN); }
// Uses the profile's nominal zero / sensitivity when known; otherwise a
// stand-in cell scaled to the ADC range: zero at 15% (85% for a falling
// output) and over-range at 80% (20%) of full scale.
inline int   simPolarity()     { return SENSOR_SIGNAL_POLARITY < 0 ? -1 : 1; }  // 0 -> rising
inline float simZeroMv()
{
    if (SENSOR_NOMINAL_ZERO_MV > 0) return SENSOR_NOMINAL_ZERO_MV;
    return (simPolarity() > 0 ? 0.15f : 0.85f) * simFullScaleMv();          // ~307 mV at GAIN_TWO
}
inline float simMvPerPpm()     // magnitude; direction from simPolarity()
{
    if (SENSOR_NOMINAL_MV_PER_UNIT > 0) return SENSOR_NOMINAL_MV_PER_UNIT;
    return 0.65f * simFullScaleMv() / SENSOR_OVERRANGE;
}
// Clean-air reading: the profile's ambient value (O2 20.9 %) or a trace.
inline float simBackgroundPpm()
{
    return SENSOR_AMBIENT_VALUE > 0 ? SENSOR_AMBIENT_VALUE
                                    : 0.0006f * SENSOR_RANGE_FS;            // 0.03 ppm on a 50 ppm cell
}
static const float SIM_TAU_S = 15.0f;                                 // T90 ~ 2.3 tau

inline float simRandUnit() { return random(0, 10001) / 10000.0f; }   // 0..1

inline float simFieldTargetPpm(SimState& s, unsigned long now)
{
    if (s.holdPpm >= 0) return s.holdPpm;
    if (!s.inEvent && now >= s.nextEventMs) {
        s.inEvent = true;
        s.eventEndMs = now + 40000UL;                          // hold the exposure ~40 s
        if (SENSOR_AMBIENT_VALUE > 0)                          // O2: deficiency dip to
            s.eventPeakPpm = SENSOR_AMBIENT_VALUE * (0.80f + 0.17f * simRandUnit()); // 80-97% of air
        else if (random(0, 6) == 0)
            s.eventPeakPpm = 1.2f * SENSOR_OVERRANGE;          // big leak -> OVER
        else                                                   // 0.5%..6% of range
            s.eventPeakPpm = SENSOR_RANGE_FS * (0.005f + 0.055f * simRandUnit());
    }
    if (s.inEvent && now >= s.eventEndMs) {
        s.inEvent = false;
        s.nextEventMs = now + 120000UL + (unsigned long)random(0, 120001); // 2-4 min
    }
    return s.inEvent ? s.eventPeakPpm : simBackgroundPpm();
}

// Simulated detector output in mV (what the ADS1115 would report).
inline uint16_t simDetectorMv(unsigned long now)
{
    SimState& s = simState();
    if (s.forceFault) return 0;

    float target;
    switch (s.scene) {
        case SIM_ZERO_AIR: target = 0.0f;        break;
        case SIM_CAL_GAS:  target = s.noCalGas ? simBackgroundPpm() : s.calGasPpm; break;
        default:           target = simFieldTargetPpm(s, now); break;
    }

    float dt = (s.lastMs == 0) ? 0.0f : (now - s.lastMs) / 1000.0f;
    s.lastMs = now;
    s.ppm += (target - s.ppm) * (1.0f - expf(-dt / SIM_TAU_S));   // cell response lag

    float drift = 2.0f * sinf(2.0f * (float)M_PI * now / 600000.0f); // +/-2 mV, 10 min
    float noise = (simRandUnit() - 0.5f) * 1.2f;                     // +/-0.6 mV
    float mv = simZeroMv() + drift + noise + simPolarity() * s.ppm * simMvPerPpm();
    float fs = simFullScaleMv();
    if (mv < 0) mv = 0;
    if (mv > fs) mv = fs;                                            // ADC saturates
    return (uint16_t)(mv + 0.5f);
}

inline const char* simSceneName(int scene)
{
    switch (scene) {
        case SIM_ZERO_AIR: return "ZERO_AIR";
        case SIM_CAL_GAS:  return "CAL_GAS";
        default:           return "FIELD";
    }
}

inline String simStatusLine()
{
    SimState& s = simState();
    String mode = s.forceFault ? "FAULT" : (s.holdPpm >= 0 ? "HOLD " + String(s.holdPpm, 2) : "AUTO");
    if (s.noCalGas) mode = mode + "+NOGAS";
    return "[SIM] scene=" + String(simSceneName(s.scene)) + " mode=" + mode +
           " ppm=" + String(s.ppm, 3) + " zero=" + String(simZeroMv(), 0) + "mV" +
           " sens=" + String(simPolarity() * simMvPerPpm(), 2) + "mV/" + SENSOR_UNIT_LABEL;
}

// Handles a "SIM ..." command line. Returns true if it was a SIM command.
inline bool simHandleCommand(String cmd, Print& out)
{
    cmd.trim();
    cmd.toUpperCase();
    if (!cmd.startsWith("SIM")) return false;
    SimState& s = simState();
    if (cmd == "SIM AUTO")        { s.holdPpm = -1; s.forceFault = false; s.noCalGas = false; }
    else if (cmd == "SIM FAULT")  { s.forceFault = true; }
    else if (cmd == "SIM OK")     { s.forceFault = false; }
    else if (cmd == "SIM NOGAS")  { s.noCalGas = true; }
    else if (cmd == "SIM GAS")    { s.noCalGas = false; }
    else if (cmd.startsWith("SIM PPM ")) {
        float v = cmd.substring(8).toFloat();
        if (v >= 0) s.holdPpm = v;
    }
    else if (cmd != "SIM STATUS") {
        out.println("SIM commands: AUTO | PPM <x> | FAULT | OK | NOGAS | GAS | STATUS");
        return true;
    }
    out.println(simStatusLine());
    return true;
}
