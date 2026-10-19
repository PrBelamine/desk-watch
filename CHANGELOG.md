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
