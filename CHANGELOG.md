# Firmware update — 27 October 2026

- MELODY (FR and EN) is rebuilt from the JOY STUDENT base: same apps and same 10-entry menu, controlled with the button (D4) and the dial (A1) instead of a joystick.
- New in MELODY: Temp & Hum with five pages (readings, minimum, maximum, average, automatic cycle), a Sleep mode menu entry, factory reset in Settings (8 settings), an eleventh chess time control (20+0), a boot sound that is off by default, and SHT31/SHT40 auto-detection.
- The RTC module is now required for the clock, the alarm and the automatic night mode; without it, Temp & Hum keeps working and the other pages show "Install RTC module".
- Controls: dial = scroll or change value; short press = OK; double click = page action; long press = back; sleep is started from the "Sleep mode" menu entry (no very long press). Counter: each press adds one; turn the dial for −1 or reset. Alarm: short press enables and sets, double click disables; any press or turn of the dial stops the ringing.
- Timer colours: blue when ready, orange while working, yellow when paused, green for the short break, cyan for the long break; the menu shows the same colour as the page.
- Default brightness is now 85 %. The High volume level is softer (the buzzer saturated at the old level). The alarm preview now plays when choosing the ringtone (lowest volume) and the volume (chosen level).
- The menu opens on the entry of the page just left (1/10 from Temp & Hum); turning the dial then takes over.
- TEACHER is unchanged and will be aligned later. The PDF guides and menu photos still show the earlier MELODY.

The MELODY sketches are syntax-checked with avr-gcc for the ATmega328P and tested on the device.

# Firmware update — 20 October 2026

- One language per build: the FR files use a French interface and the EN files an English interface. The Language choice is removed from Settings (seven settings remain). Other languages will be created on request.
- Frees about 6 KB of program storage on the Arduino UNO (TEACHER was at 100 %). Settings stored in EEPROM keep the same layout.
- SHT31 read: the CRC-8 of each 16-bit word is now checked. A corrupted read is ignored and the previous values are kept; a sensor fault is reported after three bad reads in a row. This follows a reader comment on the project page.
- Dice use a lighter random-number generator, saving about 250 bytes.

These changes compile for the four sketches (MELODY and TEACHER, FR and EN). A physical device check of the settings menu, the SHT31 read and the dice is still pending.

# Firmware update — 19 October 2026

- Current MELODY and TEACHER Editions, each with FR/EN-commented sketches.
- TEACHER adds EXAM: configurable countdown and expected papers, immediate collection counting, pause/resume, correction and extension controls, final active elapsed time excluding pauses.
- TEACHER starts on Favorite, menu 1/11, until the dial moves.
- Updated climate layout: clock always visible, measurements/alerts below.
- Eight alarm choices and live melody/volume previews; melody choice previews quietly.
- Counter displays the active timer/stopwatch; M identifies a running timer.
- Counter follows Timer in the menu; dial opens the menu after starting the timer.
- Embedded manual removed; TEACHER user guides provided as files.

The classroom example used the previous separate timer/counter workflow. The new EXAM app has passed compilation and source-derived logic tests and still needs a physical device check.

TEACHER build: Arduino AVR Boards 1.8.8, Grove LCD RGB Backlight 1.0.2, Arduino CLI 1.5.1. Program storage 32,256 bytes, global RAM 486 bytes. Existing Wokwi and menu photographs show earlier firmware.
