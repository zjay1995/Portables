#include "inc/TimeSync.h"
#include <Wire.h>
#include <sys/time.h>

static void applyTime(int year, int month, int day, int hour, int minute, int second) {
    struct tm t = {};
    t.tm_year  = year - 1900;
    t.tm_mon   = month - 1;
    t.tm_mday  = day;
    t.tm_hour  = hour;
    t.tm_min   = minute;
    t.tm_sec   = second;
    t.tm_isdst = -1;
    time_t ts = mktime(&t);
    struct timeval tv_now = { ts, 0 };
    settimeofday(&tv_now, NULL);
}

void TimeSync::initTimeFromRTC()
{
    bool rtcValid = false;

    if (m_rtc.begin(&Wire)) {
        DateTime now = m_rtc.now();

        // Clamp invalid date fields (no backup battery → month/day can be 0)
        int yr  = (now.year()  >= 2024) ? now.year()  : 0;
        int mo  = (now.month() >= 1)    ? now.month() : 1;
        int dy  = (now.day()   >= 1)    ? now.day()   : 1;

        if (yr >= 2024) {
            rtcValid = true;

            // Apply to ESP32 system clock
            applyTime(yr, mo, dy, now.hour(), now.minute(), now.second());
            Serial.printf("TimeSync: RTC OK  %04d/%02d/%02d %02d:%02d\n",
                          yr, mo, dy, now.hour(), now.minute());

            // Save fresh backup to EEPROM
            EEPROM.write(EEPROM_TIME_BACKUP_OFFSET,     (uint8_t)(yr - 2000));
            EEPROM.write(EEPROM_TIME_BACKUP_OFFSET + 1, (uint8_t)mo);
            EEPROM.write(EEPROM_TIME_BACKUP_OFFSET + 2, (uint8_t)dy);
            EEPROM.write(EEPROM_TIME_BACKUP_OFFSET + 3, (uint8_t)now.hour());
            EEPROM.write(EEPROM_TIME_BACKUP_OFFSET + 4, (uint8_t)now.minute());
            EEPROM.commit();
        }
    } else {
        Serial.println("TimeSync: RTC not found on I2C.");
    }

    if (!rtcValid) {
        // RTC lost power or not found — restore from EEPROM backup
        uint8_t yr = EEPROM.read(EEPROM_TIME_BACKUP_OFFSET);
        uint8_t mo = EEPROM.read(EEPROM_TIME_BACKUP_OFFSET + 1);
        uint8_t dy = EEPROM.read(EEPROM_TIME_BACKUP_OFFSET + 2);
        uint8_t hr = EEPROM.read(EEPROM_TIME_BACKUP_OFFSET + 3);
        uint8_t mn = EEPROM.read(EEPROM_TIME_BACKUP_OFFSET + 4);

        bool backupValid = (yr > 0 && yr < 100 &&
                            mo >= 1 && mo <= 12 &&
                            dy >= 1 && dy <= 31 &&
                            hr <= 23 && mn <= 59);

        if (backupValid) {
            applyTime(2000 + yr, mo, dy, hr, mn, 0);
            Serial.printf("TimeSync: RTC invalid — restored from EEPROM  %04d/%02d/%02d %02d:%02d\n",
                          2000 + yr, mo, dy, hr, mn);
        } else {
            Serial.println("TimeSync: No valid time backup in EEPROM. Clock at 00:00.");
        }
    }
}
