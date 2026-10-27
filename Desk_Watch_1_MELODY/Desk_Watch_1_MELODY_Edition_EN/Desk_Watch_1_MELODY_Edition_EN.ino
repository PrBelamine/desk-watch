// Desk Watch: MELODY EDITION — temperature and humidity, Pomodoro timer, counter, stopwatch, alarm, dice, chess clock and melodies, controlled with a button and a dial.
//
// Hardware: Arduino UNO + Grove Base Shield; Grove LCD RGB V4.0 (I2C); SHT31 or SHT40 sensor (I2C 0x44, auto-detected);
//           buzzer on D3; Grove button on D4; Grove rotary potentiometer (dial) on A1; RTC module DS1307 or DS3231 REQUIRED (I2C 0x68).
// The RTC provides the time, the alarm and the automatic night mode. Without an RTC the device starts and Temp & Hum still works (time shown as "--:--:--"),
// but the clock, the time setting and the alarm show "Install RTC module", and the automatic night mode is inactive.
// Required library: "Grove - LCD RGB Backlight" (rgb_lcd.h). The SHT and the RTC are read directly over I2C: no other library needed.
// MELODY has the same functions as JOY STUDENT; only the control differs: a button and a dial replace the joystick.
//
// ---- Controls ----
//   DIAL                : scroll the menu, change a value; its position selects the entry or the value (turn right = next or more).
//   SHORT PRESS         : OK (confirmed 0.3 s after release, to leave room for a double click).
//   DOUBLE CLICK        : two presses within 0.3 s; page-specific action (see below).
//   LONG PRESS          : (0.8 s or more) back: menu, cancel, previous step.
//   Sleep is started from the "Sleep mode" menu entry; a press or a turn of the dial wakes the device.
//   Wiring settings     : BTN_ACTIVE (button) and POT_MAX (1023 at 5 V, 675 if the shield selector is on 3.3 V).
//
// ---- Menu (10 entries) ----
//   Temp & Hum, Timer, Counter, Stopwatch, Alarm, Dice, Chess clock, Melody, Settings, Sleep mode.
//   The favorite is not a menu entry: it is chosen in Settings (Menu by default, Temp and Hum, Timer, Stopwatch, Counter, Dice, Chess clock) and sets the page shown at start-up.
//   Settings (8): brightness, volume, startup sound, auto sleep, night mode, set time, favorite, factory reset.
//   The startup sound is off by default (change it in Settings). Settings are saved in EEPROM.
//
// ---- Pages ----
//   Temp & Hum  : line 1 = RTC time and background-function indicators; line 2 = temperature, humidity and OK.
//                 On alert, line 2 alternates every 2 s: readings / temperature alert / readings / humidity alert.
//                 The backlight colours the screen: too high = red, too low = blue, OK = turquoise.
//                 DOUBLE CLICK = set the four thresholds (Tmin, Tmax, Hmin, Hmax): dial = value, short press = next threshold, then short press to save; long press = previous threshold.
//                 Thresholds are saved and reused at start-up. Short press = menu. The menu opens on the entry of the page just left; turning the dial takes over.
//                 The dial changes page (<> arrows left of the clock): 1 readings, 2 minimum, 3 maximum, 4 average, 5 "AUT" (alternating with the arrows) which cycles through the four pages every 3 s.
//   Timer (Pomodoro) : short press = start or pause; double click = reset (then back to the menu if already at zero).
//                 When stopped, the dial sets the work time (1 to 60 min, then 65 min to 4 h in steps of 5). 5 min breaks, 15 min long break every 4th session.
//                 Once started, turning the dial opens the menu; the countdown keeps running in the background.
//   Counter     : each short press adds 1 immediately. Turning the dial opens the action screen: short press = -1, double click = reset, 5 s without answer or long press = cancel.
//   Stopwatch   : short press = start or stop; double click = reset; if already at zero, double click = menu; turning the dial = menu (the stopwatch keeps running).
//   Alarm       : short press = enable and set (hours, minutes, ringtone, volume; short press = next step, long press = previous step), double click = disable. Eight ringtones, with preview.
//                 Turning the dial on the alarm home screen opens the menu.
//                 While ringing: any press or turn of the dial stops it for the day and opens the menu. No reaction: stops after 120 s, rings again 5 min later, until touched.
//   Dice        : short press = roll (rattle for 1 s, then result); dial = number of sides (2, 4, 6, 8, 10, 12, 20, 30, 100); double click = menu.
//   Chess clock : dial = time control when stopped (1+0, 3+0, 3+2, 5+0, 5+3, 10+0, 10+5, 15+10, 20+0, 30+0, 60+0); short press = start. While running, short press = hand over to the other player;
//                 double click = pause; then short press = resume, double click = reset, long press = menu.
//   Melody      : dial = choose; short press = play, pause or resume; double click = menu.
//   Night mode  : No / Auto (10 pm to 6 am, with RTC) / Yes: brightness reduced to 8 %, alert beeps and startup melody muted; the alarm still rings.
//   Auto sleep  : never, 1, 2, 5, 10, 15, 30 min or 1 h without activity (press or dial movement); disabled while a chess game is running.
//   Sleep mode  : screen off, alarm still active. The UNO LED (pin 13) lights up in sleep.
//   Top-right indicators: ! = temperature / humidity alert; R = alarm enabled; C = stopwatch running; M = timer running (column hidden for 1 s every 10 s).
//   Start-up    : DESK WATCH, short melody (if enabled), PR BELAMINE MAKERLAB, MELODY loading bar, module check.
//
// The internal names "molette" / "dial" (potLive, potReset, potSelect...) refer to the rotary potentiometer.

#include <Wire.h>
#include <EEPROM.h>
#include <rgb_lcd.h>

// Repeated flash strings are shared without changing any translation.
static const char TXT_EMPTY_0[] PROGMEM = "";
static const char TXT_12s_OK_1[] PROGMEM = "%-12s OK";
static const char TXT_s_laeuft_2[] PROGMEM = "%s laeuft";
static const char TXT_s_running_3[] PROGMEM = "%s running";
static const char TXT_u_min_4[] PROGMEM = "%u min";
static const char TXT_12s_5[] PROGMEM = "<> %-12s";
static const char TXT_Auto_22_6h_6[] PROGMEM = "Auto 22-6h";
static const char TXT_Brightness_7[] PROGMEM = "Brightness";
static const char TXT_Brillo_8[] PROGMEM = "Brillo";
static const char TXT_Capteur_absent_9[] PROGMEM = "Capteur absent";
static const char TXT_DESK_WATCH_11[] PROGMEM = "DESK WATCH";
static const char TXT_FIN_12[] PROGMEM = "FIN";
static const char TXT_Helligkeit_13[] PROGMEM = "Helligkeit";
static const char TXT_JA_14[] PROGMEM = "JA";
static const char TXT_Luminosite_15[] PROGMEM = "Luminosite";
static const char TXT_Menu_principal_16[] PROGMEM = "Menu principal";
static const char TXT_NEIN_17[] PROGMEM = "NEIN";
static const char TXT_NO_18[] PROGMEM = "NO";
static const char TXT_NON_19[] PROGMEM = "NON";
static const char TXT_OK_snooze_5_min_20[] PROGMEM = "OK=snooze 5 min";
static const char TXT_OK_start_21[] PROGMEM = "OK=start";
static const char TXT_OUI_22[] PROGMEM = "OUI";
static const char TXT_PAUSE_23[] PROGMEM = "PAUSE";
static const char TXT_Pause_24[] PROGMEM = "Pause";
static const char TXT_SI_25[] PROGMEM = "SI";
static const char TXT_Sensor_OK_26[] PROGMEM = "Sensor OK";
static const char TXT_Sensor_ausente_27[] PROGMEM = "Sensor ausente";
static const char TXT_Sensor_fehlt_28[] PROGMEM = "Sensor fehlt";
static const char TXT_Sensor_missing_29[] PROGMEM = "Sensor missing";

static const char TXT_Volume_31[] PROGMEM = "Volume";
#define TR_PTR(fr, en, es, de) (en)

// ---------- Settings ----------
const uint8_t PIN_BUZZER = 3;
const uint8_t PIN_BUTTON = 4;
const uint8_t PIN_LED    = 13;         // UNO L LED ; for a real red LED, plug in a Grove LED module and change this number
const uint8_t PIN_POT    = A1;
const uint8_t BTN_ACTIVE = HIGH;          // Grove module: HIGH = pressed
const int  POT_MAX   = 1023;              // 675 if the shield switch is set to 3.3 V

// Alert thresholds (as in the V6 station): value >= high threshold = TOO HIGH, <= low threshold = TOO LOW.
// Default values ; adjustable by double click on the Temp & Hum page and saved.
const uint8_t T_LO_DEFAULT2 = 40, T_HI_DEFAULT2 = 56;   // temperature in half-degrees: 20.0 and 28.0 C
const uint8_t H_LO_DEFAULT  = 40, H_HI_DEFAULT  = 60;   // humidity in %
const uint8_t T_MIN2 = 20, T_MAX2 = 80, T_GAP2 = 2;     // range 10 to 40 C in 0.5 C steps, minimum gap 1 C
const uint8_t H_MIN  = 10, H_MAX  = 90, H_GAP  = 5;     // range 10 to 90 %, minimum gap 5 points
const int T_MARGE10 = 3, H_MARGE10 = 20;                 // anti-flicker margin around a threshold (in tenths: 0.3 C and 2 %)
const int TEMP_OFFSET10 = 0, HUM_OFFSET10 = 0;           // correction in tenths after comparison with a reference

const unsigned long SNOOZE_MS  = 5UL * 60000UL;
const unsigned long RING_MAX_S = 120;     // maximum ringing time before automatic snooze
const uint8_t NIGHT_START = 22, NIGHT_END = 6;   // dimmed screen

const uint8_t SHORT_BREAK_MIN = 5, LONG_BREAK_MIN = 15, LONG_EVERY = 4;
// work durations: 1 to 60 min in 1 min steps, then 65 min to 4 h in 5 min steps
const uint8_t N_WORK = 96;
uint8_t workValue(uint8_t i) { return i < 60 ? i + 1 : 60 + (i - 59) * 5; }

const uint8_t SHT_ADDR = 0x44, RTC_ADDR = 0x68;

const uint8_t BRIGHT_LEVELS[8] = { 5, 10, 20, 30, 50, 70, 85, 100 };   // % of the backlight

// ---------- State ----------
rgb_lcd lcd;

enum { M_TH, M_POMO, M_CHRONO, M_TALLY, M_DIE, M_CHESS, M_MUSIC, M_PARAMS, M_HOME, M_ALARM, M_SETCLOCK, M_BRIGHT, M_MENU, M_CLOCK, M_SLEEP };
const uint8_t N_MENU = 10;
const uint8_t N_PAR = 8;   // visible settings (the language choice was removed: one language per edition)
uint8_t mode = M_HOME, prevMode = M_HOME;   // M_* values
uint8_t menuIdx = 0;
uint8_t brightBackup = 100, brightIdx = 7;

enum { EV_NONE, EV_SHORT, EV_LONG, EV_VLONG, EV_DOUBLE, EV_PRESS };

struct Settings { uint8_t magic, aH, aM, armed, workMin, bright, tLo2, tHi2, hLo, hHi, sound, lang, volume, home, night, sleep; } cfg;   // lang: 0 French, 1 English, 2 Spanish, 3 German ; home: startup page (0 menu, 1 Temp & Hum, 2 timer, 3 stopwatch, 4 counter, 5 dice, 6 chess clock) ; night: night mode (0 no, 1 auto by time of day, 2 yes) ; sleep: auto sleep delay (index into SLEEP_MIN, 0 = never)

// text of this edition (one language per build: the file name gives it), in flash memory
#define TR(fr, en, es, de) PSTR(en)

// clock
bool rtcPresent = false;
unsigned long swSecs = 0, swRefMs = 0;
uint8_t nowH = 0, nowM = 0, nowS = 0;

// sensor
int temp10 = 0, hum10 = 0;      // temperature (tenths of C) and humidity (tenths of %): no floating-point math, to save memory
bool shtOk = false;
uint8_t tState = 0, hState = 0;   // 0 OK, 1 TOO HIGH, 2 TOO LOW

// alarm
uint8_t clockSub = 0;       // Set time page: 1 hours, 2 minutes
uint8_t alarmSub = 0;       // 0 activation, 1 hours, 2 minutes, 3 melody, 4 volume, 5 confirmation.
bool alarmExitPending = false;
unsigned long alarmExitAt = 0;
bool alarmSavedPending = false;
unsigned long alarmSavedReturnAt = 0;
// Separate EEPROM extension preserves the existing settings and their addresses.
struct AlarmSettings { uint8_t magic, melody, volume; } alarmCfg;
uint8_t alarmPos = 0;
unsigned long alarmNoteEnd = 0;
bool alarmPreview = false;
uint8_t previewMelody = 0, previewVolume = 0;
uint8_t parSub = 0, parBackup = 0;   // setting open? value before editing
uint8_t parIdx = 1;          // Settings page: 1 brightness, 2 volume, 3 sound, 4 sleep, 5 night mode, 6 time, 7 favorite, 8 factory reset
unsigned int tallyCount = 0;  // hand tally counter
uint8_t tallySub = 0;         // 0 counting, 1 reset confirmation
unsigned long tallyT0 = 0;
unsigned long tapMs = 0;      // time of the last short press release
bool tapFlag = false;         // a short press has just been released (counts immediately, without waiting for the double-click delay)
// dice: possible sides (2 = coin flip, 30 = pick a student from the class)
const uint8_t N_DIE = 9;
static const uint8_t DIE_FACES[N_DIE] PROGMEM = { 2, 4, 6, 8, 10, 12, 20, 30, 100 };
uint8_t dieIdx = 2, dieVal = 0, dieShow = 0;   // chosen sides (D6 at start), last result, value displayed during the roll
bool dieRolling = false;
unsigned long dieEnd = 0;
// chess clock: time controls (minutes, increment in seconds)
const uint8_t N_CHESS = 11;
static const uint8_t CHESS_MIN[N_CHESS] PROGMEM = { 1, 3, 3, 5, 5, 10, 10, 15, 20, 30, 60 };
static const uint8_t CHESS_INC[N_CHESS] PROGMEM = { 0, 0, 2, 0, 3, 0, 5, 10, 0, 0, 0 };
uint8_t chessIdx = 4;                 // 5+3 at start
uint8_t chessState = 0;               // 0 waiting, 1 running, 2 paused, 3 time out
uint8_t chessSide = 0;                // player whose clock is running (0 or 1)
long chessBase[2];                    // time left for each player at the start of their turn (ms)
unsigned long chessTurn = 0;          // time the turn started
long chessLeft(uint8_t sd) {
  long v = chessBase[sd];
  if (chessState == 1 && sd == chessSide) v -= (long)(millis() - chessTurn);
  return v < 0 ? 0 : v;
}
// music notes (Hz) and lengths (x 10 ms), kept in flash
const uint8_t N_BDAY = 25;
static const uint16_t BD_NOTE[N_BDAY] PROGMEM = {
  349, 349, 392, 349, 466, 440,
  349, 349, 392, 349, 523, 466,
  349, 349, 698, 587, 466, 440, 392,
  622, 622, 587, 466, 523, 466 };
static const uint8_t BD_LEN[N_BDAY] PROGMEM = {
  35, 20, 60, 60, 60, 120,
  35, 20, 60, 60, 60, 120,
  35, 20, 60, 60, 60, 75, 100,
  35, 20, 60, 60, 60, 120 };
const uint8_t N_ELISE = 34;
static const uint16_t ELISE_NOTE[N_ELISE] PROGMEM = {
  659, 622, 659, 622, 659, 494, 587, 523, 440,
  262, 330, 440, 494, 330, 415, 494, 523,
  659, 622, 659, 622, 659, 494, 587, 523, 440,
  262, 330, 440, 494, 330, 523, 494, 440 };
static const uint8_t ELISE_LEN[N_ELISE] PROGMEM = {
  18, 18, 18, 18, 18, 18, 18, 18, 45,
  18, 18, 18, 45, 18, 18, 18, 45,
  18, 18, 18, 18, 18, 18, 18, 18, 45,
  18, 18, 18, 45, 18, 18, 18, 60 };
const uint8_t N_MARIO = 45;
static const uint16_t MARIO_NOTE[N_MARIO] PROGMEM = { 659, 659, 0, 659, 0, 523, 659, 784, 0, 392, 0, 523, 392, 0, 330, 0, 392, 0, 523, 0, 440, 494, 466, 440, 392, 659, 784, 880, 698, 784, 0, 659, 523, 587, 494, 0, 523, 0, 392, 0, 440, 494, 466, 440, 392 };
static const uint8_t MARIO_LEN[N_MARIO] PROGMEM = { 15, 15, 15, 15, 15, 15, 15, 30, 30, 15, 30, 45, 15, 30, 45, 30, 30, 15, 15, 15, 15, 15, 30, 30, 30, 30, 30, 45, 15, 30, 15, 15, 15, 15, 30, 15, 15, 15, 30, 15, 15, 15, 15, 15, 45 };
const uint8_t N_TETRIS = 36;
static const uint16_t TETRIS_NOTE[N_TETRIS] PROGMEM = { 659, 494, 523, 587, 523, 494, 440, 440, 523, 659, 587, 523, 494, 523, 587, 659, 523, 440, 440, 0, 0, 587, 698, 880, 784, 698, 659, 523, 659, 587, 523, 494, 494, 523, 587, 659 };
static const uint8_t TETRIS_LEN[N_TETRIS] PROGMEM = { 42, 21, 21, 42, 21, 21, 42, 21, 21, 42, 21, 21, 63, 21, 42, 42, 42, 42, 42, 42, 21, 42, 21, 42, 42, 21, 21, 42, 21, 21, 42, 42, 21, 21, 42, 42 };
const uint8_t N_MUSIC = 4;
static const uint16_t* const MUSIC_NOTES[N_MUSIC] PROGMEM = {
  BD_NOTE, ELISE_NOTE, MARIO_NOTE, TETRIS_NOTE };
static const uint8_t* const MUSIC_LENGTHS[N_MUSIC] PROGMEM = {
  BD_LEN, ELISE_LEN, MARIO_LEN, TETRIS_LEN };
static const uint8_t MUSIC_NOTE_COUNTS[N_MUSIC] PROGMEM = {
  N_BDAY, N_ELISE, N_MARIO, N_TETRIS };
static const char MUSIC_NAMES[] PROGMEM =
  "Birthday\0"
  "Fur Elise\0"
  "Mario Bros\0"
  "Tetris\0";
// Original alarm-only themes (Hz / duration in 10 ms units). Zero is a rest.
// Aurora: flowing major arpeggios, then a bright upper-register resolution.
static const uint16_t AURORA_NOTE[] PROGMEM = {
  523, 659, 784, 1047, 988, 784, 659, 0,
  587, 740, 880, 1175, 1047, 880, 740, 0,
  659, 784, 1047, 1319, 1175, 988, 1047, 0 };
static const uint8_t AURORA_LEN[] PROGMEM = {
  18, 18, 18, 36, 18, 18, 36, 12,
  18, 18, 18, 36, 18, 18, 36, 12,
  18, 18, 24, 36, 18, 18, 72, 24 };
// Cosmic Pulse: syncopated minor pulses and a cascading answer.
static const uint16_t COSMIC_NOTE[] PROGMEM = {
  440, 0, 880, 659, 0, 523, 659, 0,
  440, 0, 988, 784, 0, 659, 523, 0,
  587, 0, 1175, 880, 0, 698, 880, 0,
  1047, 988, 880, 784, 659, 523, 440, 0 };
static const uint8_t COSMIC_LEN[] PROGMEM = {
  12, 6, 24, 12, 6, 12, 24, 12,
  12, 6, 24, 12, 6, 12, 24, 12,
  12, 6, 24, 12, 6, 12, 24, 12,
  12, 12, 12, 12, 18, 18, 48, 24 };
// Arcade Hero: short upbeat calls, octave jumps and a victory flourish.
static const uint16_t ARCADE_NOTE[] PROGMEM = {
  784, 0, 784, 988, 1175, 0, 1568, 1175,
  1319, 0, 1175, 988, 1047, 0, 784, 0,
  880, 1047, 1319, 0, 1175, 988, 1568, 0,
  1319, 1175, 988, 1175, 1568, 0, 1568, 0 };
static const uint8_t ARCADE_LEN[] PROGMEM = {
  12, 6, 12, 12, 24, 6, 18, 12,
  24, 6, 12, 12, 24, 6, 24, 12,
  12, 12, 24, 6, 12, 12, 24, 12,
  12, 12, 12, 12, 36, 12, 48, 24 };
// Epic Rise: dotted fanfare, a rising sequence and a sustained final high note.
static const uint16_t EPIC_NOTE[] PROGMEM = {
  392, 0, 392, 523, 659, 0, 784, 659,
  587, 0, 587, 740, 880, 0, 1175, 880,
  659, 784, 988, 1175, 784, 988, 1175, 1397,
  1047, 0, 1319, 0, 1568, 1319, 1568, 0 };
static const uint8_t EPIC_LEN[] PROGMEM = {
  27, 9, 12, 24, 36, 12, 36, 18,
  27, 9, 12, 24, 36, 12, 36, 18,
  12, 12, 12, 24, 12, 12, 12, 24,
  36, 12, 36, 12, 36, 18, 72, 24 };
static_assert(sizeof(AURORA_NOTE) / sizeof(uint16_t) == sizeof(AURORA_LEN), "Aurora note/length mismatch");
static_assert(sizeof(COSMIC_NOTE) / sizeof(uint16_t) == sizeof(COSMIC_LEN), "Cosmic note/length mismatch");
static_assert(sizeof(ARCADE_NOTE) / sizeof(uint16_t) == sizeof(ARCADE_LEN), "Arcade note/length mismatch");
static_assert(sizeof(EPIC_NOTE) / sizeof(uint16_t) == sizeof(EPIC_LEN), "Epic note/length mismatch");
const uint8_t N_ALARM_TUNES = N_MUSIC + 4;
static const uint16_t* const ALARM_NOTES[N_ALARM_TUNES] PROGMEM = {
  BD_NOTE, ELISE_NOTE, MARIO_NOTE, TETRIS_NOTE, AURORA_NOTE, COSMIC_NOTE, ARCADE_NOTE, EPIC_NOTE };
static const uint8_t* const ALARM_LENGTHS[N_ALARM_TUNES] PROGMEM = {
  BD_LEN, ELISE_LEN, MARIO_LEN, TETRIS_LEN, AURORA_LEN, COSMIC_LEN, ARCADE_LEN, EPIC_LEN };
static const uint8_t ALARM_NOTE_COUNTS[N_ALARM_TUNES] PROGMEM = {
  N_BDAY, N_ELISE, N_MARIO, N_TETRIS,
  sizeof(AURORA_LEN), sizeof(COSMIC_LEN), sizeof(ARCADE_LEN), sizeof(EPIC_LEN) };
static const char ALARM_EXTRA_NAMES[4][13] PROGMEM = {
  "Aurora", "Cosmic Pulse", "Arcade Hero", "Epic Rise" };

PGM_P packedText(PGM_P text, uint8_t index);
PGM_P alarmTuneName() {
  return alarmCfg.melody < N_MUSIC ? packedText(MUSIC_NAMES, alarmCfg.melody)
                                 : ALARM_EXTRA_NAMES[alarmCfg.melody - N_MUSIC];
}

uint8_t musicTrack = 0;
uint8_t musicState = 0, musicPos = 0;  // 0 ready, 1 playing, 2 paused ; next note to play
int musicRemain = 0;                  // time left on the current note (ms)
unsigned int musicFreq = 0;
unsigned long musicNoteEnd = 0;
uint8_t thSub = 0;          // 0 = display, 1-4 = thresholds, 5 = confirmation.
bool thSavedPending = false;
unsigned long thSavedReturnAt = 0;
uint8_t thView = 0, thSampleCount = 0, thSamplePos = 0;
int16_t thTempSamples[30] = {0}, thHumSamples[30] = {0};
int32_t thTempSum = 0, thHumSum = 0;
uint8_t edH = 0, edM = 0;
bool ringing = false, snoozing = false;
bool standby = false;       // sleep: screen off, alarm still active
static const uint8_t SLEEP_MIN[8] PROGMEM = { 0, 1, 2, 5, 10, 15, 30, 60 };   // auto sleep delays (minutes, 0 = never)
unsigned long lastActivity = 0;   // last press or dial movement
int lastPotSample = 0;
unsigned long lastPotCheck = 0;
unsigned long btnHeld = 0;        // duration of the current press (ms), 0 if the button is released
unsigned long ringStart = 0, snoozeUntil = 0;
int lastFiredMin = -1;

// pomodoro
uint8_t pomoPhase = 0;      // 0 work, 1 short break, 2 long break
uint8_t pomoCount = 0;
bool pomoRun = false;
unsigned long pomoRemMs = 0, pomoLast = 0;

// stopwatch
bool chronoRun = false;
unsigned long chronoStart = 0, chronoAccum = 0;
unsigned long chronoMs() { return chronoRun ? millis() - chronoStart : chronoAccum; }

// potentiometer / display
bool potActive = false; int potAnchor = 0; bool dialAct = false;   // Dial: anchor, detected movement, recent activity.
uint8_t lastR = 255, lastG = 255, lastB = 255;
unsigned long lastDraw = 0, lastClockRead = 0, lastSht = 0;

// ---------- Utilities ----------
uint8_t bcd2dec(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
uint8_t dec2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }


// Smoothed reading: average of eight samples; the first one, disturbed by the multiplexer, is ignored.
int potRead() {
  analogRead(PIN_POT);
  long sum = 0;
  for (uint8_t i = 0; i < 8; i++) sum += analogRead(PIN_POT);
  int v = (int)(sum / 8);
  if (v > POT_MAX) v = POT_MAX;
  return POT_MAX - v;                              // Reversed: turning right increases the value.
}
// Choice among n values with the dial, with hysteresis at the range limits.
uint8_t potSelect(uint8_t n, uint8_t cur, int margin) {
  const int zone = (POT_MAX + 1) / n;
  int raw = potRead();
  int target = raw / zone;
  if (target > n - 1) target = n - 1;
  if (target > cur && raw >= target * zone + margin) return target;
  if (target < cur && raw <= (int)cur * zone - margin) return target;
  return cur;
}
uint8_t workIndex(uint8_t m) {
  if (m < 1) return 0;
  if (m <= 60) return m - 1;
  uint8_t i = 59 + (m - 60 + 2) / 5;
  return i > N_WORK - 1 ? N_WORK - 1 : i;
}
// "25 min" up to 60 minutes, then "1h05"
void fmtDuration(char* b, size_t n, unsigned m) {
  if (m <= 60) snprintf_P(b, n, TXT_u_min_4, m);
  else         snprintf_P(b, n, PSTR("%uh%02u"), m / 60, m % 60);
}
void potReset() { potAnchor = potRead(); potActive = false; }
bool potLive() {
  int d = potRead() - potAnchor;
  if (d < 0) d = -d;
  if (!potActive && d > 30) potActive = true;
  return potActive;
}

// night mode: active if selected (Yes) or, in automatic mode, between NIGHT_START and NIGHT_END (needs the RTC module)
bool nightActive() {
  if (cfg.night == 2) return true;
  return cfg.night == 1 && rtcPresent && (nowH >= NIGHT_START || nowH < NIGHT_END);
}
bool nightPrev = false;
bool validationFlashActive = false;
unsigned long validationFlashDuration = 1000UL;
unsigned long validationFlashStart = 0;

// applies the requested colour, scaled by the chosen brightness (and dimmed at night, except while ringing)
void applyColor() {
  unsigned long elapsed = millis() - validationFlashStart;
  bool flashOn = validationFlashActive && elapsed < validationFlashDuration;
  uint8_t k = (nightActive() && !ringing) ? 8 : 100;
  nightPrev = nightActive();
  uint8_t r = flashOn ? 0 : lastR;
  uint8_t g = flashOn ? 200 : lastG;
  uint8_t b = flashOn ? 45 : lastB;
  lcd.setRGB((uint32_t)r * cfg.bright * k / 10000,
             (uint32_t)g * cfg.bright * k / 10000,
             (uint32_t)b * cfg.bright * k / 10000);
}

void color(uint8_t r, uint8_t g, uint8_t b) {
  bool statusColor =
      (r == 255 && b == 0 && (g == 0 || g == 90 || g == 110)) ||
      (r == 220 && g == 30 && b == 0) ||
      (r == 200 && g == 110 && b == 0) ||
      (r == 255 && g == 110 && b == 0) ||
      (r == 0 && g == 80 && (b == 255 || b == 220)) ||
      (r == 0 && g == 160 && b == 40) ||
      (r == 0 && g == 170 && b == 50) ||
      (r == 0 && g == 200 && b == 45) ||
      (r == 0 && g == 140 && b == 170) ||
      (r == 60 && g == 60 && (b == 60 || b == 120));
  if (mode == M_TH && r == 0 && g == 160 && b == 40) statusColor = false;
  if (mode == M_POMO && (r | g | b)) statusColor = true;   // Timer: own colour for each state, see drawPomo.
  if (!statusColor) {
    switch (mode) {
    case M_TH: r=0; g=145; b=145; break;
    case M_POMO: r=205; g=80; b=0; break;
    case M_CHRONO: r=0; g=105; b=205; break;
    case M_TALLY: r=0; g=155; b=45; break;
    case M_DIE: r=125; g=40; b=170; break;
    case M_CHESS: r=170; g=40; b=125; break;
    case M_MUSIC: r=95; g=45; b=180; break;
    case M_PARAMS: r=40; g=80; b=160; break;
    case M_HOME: r=0; g=130; b=120; break;
    case M_ALARM: r=205; g=30; b=0; break;
    case M_SETCLOCK: r=165; g=95; b=0; break;
    case M_BRIGHT: r=30; g=100; b=180; break;
    case M_MENU: r=0; g=105; b=155; break;
    case M_CLOCK: r=35; g=85; b=155; break;
    case M_SLEEP: r=255; g=0; b=0; break;
    default: break;
    }
  }
  if (r == lastR && g == lastG && b == lastB) return;
  lastR = r; lastG = g; lastB = b;
  applyColor();
}

// Write sixteen characters from RAM or flash, padding with spaces.
__attribute__((noinline, noclone)) void writeLcdLine(uint8_t row, const char* s, bool flash) {
  lcd.setCursor(0, row);
  for (uint8_t i = 0; i < 16; i++) {
    uint8_t c = flash ? pgm_read_byte(s) : *s;
    if (c) s++; else c = ' ';
    lcd.write(c);
  }
}
void startValidationFlash(unsigned long duration = 1000UL) {
  validationFlashDuration = duration;
  validationFlashActive = true;
  validationFlashStart = millis();
  applyColor();
}

void updateValidationFlash() {
  if (!validationFlashActive) return;
  if (millis() - validationFlashStart >= validationFlashDuration) {
    validationFlashActive = false;
    applyColor();
  }
}
void line(uint8_t row, const char* s) { writeLcdLine(row, s, false); }
void lineP(uint8_t row, PGM_P s) { writeLcdLine(row, s, true); }

// Zero-separated flash strings avoid the padding of fixed-width text tables.
PGM_P packedText(PGM_P text, uint8_t index) {
  while (index--) text += strlen_P(text) + 1;
  return text;
}

PGM_P volumeName(uint8_t volume) {
  return volume == 0 ? TR("Faible", "Low", "Bajo", "Leise") : volume == 1 ? TR("Moyen", "Medium", "Medio", "Mittel") : TR("Fort", "High", "Alto", "Laut");
}

void musicToneStop() {
  TCCR2A = 0;
  TCCR2B = 0;
  PORTD &= ~_BV(PD3);
}

void musicToneStartAtVolume(unsigned int freq, uint8_t volume) {
  static const uint8_t DUTY[3] = { 5, 15, 25 };
  unsigned long count = F_CPU / (256UL * freq);
  if (count < 1) count = 1;
  if (count > 256) count = 256;
  musicToneStop();
  OCR2A = count - 1;
  OCR2B = count * DUTY[volume] / 100;
  TCNT2 = 0;
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS22) | _BV(CS21);
}

void musicToneStart(unsigned int freq) { musicToneStartAtVolume(freq, cfg.volume); }

__attribute__((noinline)) unsigned int playTrackNote(uint8_t track, uint8_t pos, uint8_t volume, unsigned int &freq) {
  const uint16_t* notes = (const uint16_t*)pgm_read_word(&MUSIC_NOTES[track]);
  const uint8_t* lengths = (const uint8_t*)pgm_read_word(&MUSIC_LENGTHS[track]);
  freq = pgm_read_word(&notes[pos]);
  if (freq) musicToneStartAtVolume(freq, volume); else musicToneStop();
  return pgm_read_byte(&lengths[pos]) * 10;
}

void stopAlarmPreview() {
  if (!alarmPreview) return;
  musicToneStop();
  alarmPreview = false;
}

// Shared by the real alarm and its preview; no delay blocks the controls.
void alarmMelodyTick(unsigned long now, uint8_t volume) {
  if ((long)(now - alarmNoteEnd) < 0) return;
  musicToneStop();
  uint8_t count = pgm_read_byte(&ALARM_NOTE_COUNTS[alarmCfg.melody]);
  if (alarmPos >= count) { alarmPos = 0; alarmNoteEnd = now + 700; }
  else {
    const uint16_t* notes = (const uint16_t*)pgm_read_word(&ALARM_NOTES[alarmCfg.melody]);
    const uint8_t* lengths = (const uint8_t*)pgm_read_word(&ALARM_LENGTHS[alarmCfg.melody]);
    unsigned int freq = pgm_read_word(&notes[alarmPos]);
    alarmNoteEnd = now + pgm_read_byte(&lengths[alarmPos]) * 10UL;
    if (freq) musicToneStartAtVolume(freq, volume);
    alarmPos++;
  }
}

void updateAlarmPreview() {
  if (alarmSub != 3 && alarmSub != 4) { stopAlarmPreview(); return; }
  uint8_t volume = alarmSub == 3 ? 0 : alarmCfg.volume;
  if (!alarmPreview || previewMelody != alarmCfg.melody || previewVolume != volume) {
    musicToneStop();
    alarmPos = 0; alarmNoteEnd = millis();
    previewMelody = alarmCfg.melody; previewVolume = volume;
    alarmPreview = true;
  }
  alarmMelodyTick(millis(), volume);
}

// Buzzer sound: square wave at the requested frequency ; the volume sets the duty cycle
// (5 %, 20 % or 50 %: the shorter it is, the quieter the piezo buzzer). Blocking for 'ms'.
void buzz(unsigned int freq, unsigned int ms) {
  if (alarmPreview) musicToneStop();   // let notification beeps temporarily take over the pin
  static const uint8_t DUTY[3] = { 5, 15, 25 };
  unsigned long period = 1000000UL / freq;
  unsigned long high = period * DUTY[cfg.volume] / 100;
  unsigned long end = millis() + ms;
  while ((long)(millis() - end) < 0) {
    digitalWrite(PIN_BUZZER, HIGH); delayMicroseconds(high);
    digitalWrite(PIN_BUZZER, LOW);  delayMicroseconds(period - high);
  }
}

void beep(uint8_t n, unsigned int ms) {
  for (uint8_t i = 0; i < n; i++) {
    buzz(2300, ms);
    delay(ms);
  }
}

const unsigned long DOUBLE_MS = 300;       // maximum delay between two presses for a double click

// ---------- Button and dial ----------
// readButton() reads button D4: EV_SHORT (short press, confirmed after the double-click delay), EV_DOUBLE, EV_LONG (0.8 s or more)
// and EV_PRESS (music, immediate). It also detects dial movement (dialAct).
uint8_t readButton() {
  static bool down = false, pending = false, second = false, musicPressHandled = false;
  static unsigned long t0 = 0, tRel = 0;
  bool p = (digitalRead(PIN_BUTTON) == BTN_ACTIVE);
  unsigned long now = millis();
  btnHeld = (p && down) ? now - t0 : 0;

  if (now - lastPotCheck >= 60) {                  // Dial movement: wake-up, ring stop, auto sleep.
    lastPotCheck = now;
    int pv = potRead();
    if (abs(pv - lastPotSample) > 25) { lastPotSample = pv; dialAct = true; }
  }

  // An isolated short press is confirmed once the double-click delay has elapsed.
  if (pending && !down && !p && now - tRel > DOUBLE_MS) {
    pending = false;
    if (musicPressHandled) { musicPressHandled = false; if (!ringing) return EV_NONE; }
    return EV_SHORT;
  }

  if (p && !down) {
    down = true; t0 = now;
    second = pending && (now - tRel <= DOUBLE_MS);
    if (mode == M_MUSIC && !ringing && !second) { musicPressHandled = true; return EV_PRESS; }
  } else if (!p && down) {
    down = false;
    unsigned long d = now - t0;
    if (d < 30) { second = false; musicPressHandled = false; return EV_NONE; }
    if (d < 800) { tapFlag = true; tapMs = now; }
    if (d < 800) {
      if (second) { pending = false; second = false; musicPressHandled = false; return EV_DOUBLE; }
      pending = true; tRel = now;
      return EV_NONE;
    }
    pending = false; second = false; musicPressHandled = false;
    return EV_LONG;
  }
  return EV_NONE;
}

// ---------- EEPROM configuration ----------
void loadCfg() {
  EEPROM.get(0, cfg);
  if (cfg.magic == 0xAB) {
    if (cfg.home == 2) cfg.home = 0; else if (cfg.home >= 3 && cfg.home <= 5) cfg.home--;
    cfg.magic = 0xAC; EEPROM.put(0, cfg);
  }
  if (cfg.magic != 0xAC || cfg.aH > 23 || cfg.aM > 59 || cfg.armed > 1 ||
      cfg.workMin < 1 || cfg.workMin > 240) {
    cfg.magic = 0xAC; cfg.aH = 7; cfg.aM = 0; cfg.armed = 0; cfg.workMin = 25;
    cfg.bright = 85; cfg.sound = 0; cfg.volume = 1; cfg.home = 0; cfg.night = 1; cfg.sleep = 0; cfg.tLo2 = 0xFF;       // thresholds are re-checked below
  }
  if (cfg.sleep > 7) cfg.sleep = 0;                     // auto sleep: off by default
  if (cfg.night > 2) cfg.night = 1;                     // night mode: automatic by default
  if (cfg.home > 6) cfg.home = 0;                       // Favorite: Menu by default
  if (cfg.volume > 2) cfg.volume = 1;                   // medium volume by default
  if (cfg.sound > 1) cfg.sound = 0;                    // startup sound: off by default
  // thresholds: default values if missing or inconsistent
  if (cfg.tLo2 < T_MIN2 || cfg.tHi2 > T_MAX2 || cfg.tHi2 < cfg.tLo2 + T_GAP2 ||
      cfg.hLo < H_MIN || cfg.hHi > H_MAX || cfg.hHi < cfg.hLo + H_GAP) {
    cfg.tLo2 = T_LO_DEFAULT2; cfg.tHi2 = T_HI_DEFAULT2;
    cfg.hLo = H_LO_DEFAULT;   cfg.hHi = H_HI_DEFAULT;
  }
  if (cfg.bright < 1 || cfg.bright > 100) cfg.bright = 85;   // old save or invalid value
  EEPROM.get(sizeof(cfg), alarmCfg);
  if (alarmCfg.magic != 0xB1 || alarmCfg.melody >= N_ALARM_TUNES || alarmCfg.volume > 2) {
    alarmCfg.magic = 0xB1; alarmCfg.melody = 0; alarmCfg.volume = cfg.volume;
  }
}
void saveCfg(unsigned long validationDuration = 1000UL) { EEPROM.put(0, cfg); EEPROM.put(sizeof(cfg), alarmCfg); startValidationFlash(validationDuration); }

// ---------- Clock ----------
void readClock() {
  if (rtcPresent) {
    Wire.beginTransmission(RTC_ADDR);
    Wire.write((uint8_t)0);
    if (Wire.endTransmission() == 0 && Wire.requestFrom(RTC_ADDR, (uint8_t)3) == 3) {
      uint8_t s = Wire.read();
      uint8_t m = Wire.read();
      uint8_t h = Wire.read();
      nowS = bcd2dec(s & 0x7F);
      nowM = bcd2dec(m & 0x7F);
      nowH = bcd2dec(h & 0x3F);
    }
  } else {
    unsigned long t = (swSecs + (millis() - swRefMs) / 1000UL) % 86400UL;
    nowH = t / 3600; nowM = (t / 60) % 60; nowS = t % 60;
  }
}

void setClock(uint8_t h, uint8_t m) {
  if (rtcPresent) {
    Wire.beginTransmission(RTC_ADDR);
    Wire.write((uint8_t)0);
    Wire.write((uint8_t)0);          // seconds = 0 (also clears the CH bit of the DS1307)
    Wire.write(dec2bcd(m));
    Wire.write(dec2bcd(h));          // 24 h mode
    Wire.endTransmission();
  } else {
    swSecs = (unsigned long)h * 3600UL + (unsigned long)m * 60UL;
    swRefMs = millis();
  }
  readClock();
  startValidationFlash();
}

// ---------- SHT31 ----------
// Small 16-bit xorshift generator: replaces random()/randomSeed() and saves about 250 bytes of flash
static uint16_t rngS = 1;
static void seedRng(uint32_t s) { rngS = (uint16_t)(s ^ (s >> 16)); if (!rngS) rngS = 1; }
static uint16_t rollDie(uint8_t faces) {
  rngS ^= rngS << 7; rngS ^= rngS >> 9; rngS ^= rngS << 8;
  return 1 + rngS % faces;
}

// CRC-8 (polynomial 0x31, init 0xFF) used by the SHT31 to protect each 16-bit word
uint8_t crc8(const uint8_t *d, uint8_t n) {
  uint8_t c = 0xFF;
  while (n--) {
    c ^= *d++;
    for (uint8_t i = 0; i < 8; i++) c = (c & 0x80) ? (c << 1) ^ 0x31 : (c << 1);
  }
  return c;
}

// Humidity sensor: SHT31 or SHT40, detected automatically (same I2C address 0x44, different commands, same CRC).
uint8_t shtType = 0;   // 0 = not detected yet, 1 = SHT31, 2 = SHT40
bool shtTry(uint8_t type, uint8_t *b) {
  Wire.beginTransmission(SHT_ADDR);
  if (type == 1) { Wire.write((uint8_t)0x24); Wire.write((uint8_t)0x00); }   // SHT31: measurement with high repeatability.
  else Wire.write((uint8_t)0xFD);                                            // SHT40: high-precision measurement.
  if (Wire.endTransmission() != 0) return false;
  delay(type == 1 ? 20 : 10);
  if (Wire.requestFrom(SHT_ADDR, (uint8_t)6) != 6) return false;
  for (uint8_t i = 0; i < 6; i++) b[i] = Wire.read();
  return true;
}
bool readSht() {
  uint8_t b[6];
  static uint8_t crcBad = 0;
  bool got = false, crcOk = false;
  for (uint8_t t = shtType ? shtType : 1; t <= (shtType ? shtType : 2); t++) {   // known type: one attempt; otherwise SHT31 then SHT40
    if (!shtTry(t, b)) continue;
    got = true;
    if (crc8(b, 2) == b[2] && crc8(b + 3, 2) == b[5]) { shtType = t; crcOk = true; break; }
  }
  if (!crcOk) {
    if (got && shtType && ++crcBad < 3) return shtOk;   // corrupted read: keep the previous values; fault reported after 3 bad reads in a row
    shtType = 0;                                        // sensor missing or changed: detect again on the next read
    return false;
  }
  crcBad = 0;
  uint16_t rt = ((uint16_t)b[0] << 8) | b[1];
  uint16_t rh = ((uint16_t)b[3] << 8) | b[4];
  temp10 = (int)((1750L * rt) / 65535L) - 450 + TEMP_OFFSET10;                    // -45 + 175 * S / 65535, same for both sensors
  hum10  = (shtType == 1 ? (int)((1000L * rh) / 65535L) : (int)((1250L * rh) / 65535L) - 60) + HUM_OFFSET10;   // SHT31: 100 * S / 65535; SHT40: -6 + 125 * S / 65535
  if (hum10 < 0) hum10 = 0;
  if (hum10 > 1000) hum10 = 1000;
  if (thSampleCount == 30) { thTempSum -= thTempSamples[thSamplePos]; thHumSum -= thHumSamples[thSamplePos]; }
  else thSampleCount++;
  thTempSamples[thSamplePos] = temp10; thHumSamples[thSamplePos] = hum10;
  thTempSum += temp10; thHumSum += hum10;
  thSamplePos = (thSamplePos + 1) % 30;
  return true;
}

// updates a state with a margin ; returns true if it has just become TOO HIGH or TOO LOW
bool updateState(uint8_t &st, int x, int lo, int hi, int marge) {
  uint8_t old = st;
  if (x >= hi)                                st = 1;
  else if (x <= lo)                           st = 2;
  else if (st == 1 && x >= hi - marge)        { }
  else if (st == 2 && x <= lo + marge)        { }
  else                                        st = 0;
  return st != old && st != 0;
}
void alertBeep(uint8_t st) {
  if (ringing || standby || nightActive()) return;   // silence while asleep and at night
  if (st == 1) beep(2, 120); else beep(1, 700);   // high: two short beeps, low: one long beep
}
void updateAlerts() {
  if (updateState(tState, temp10, cfg.tLo2 * 5, cfg.tHi2 * 5, T_MARGE10))   alertBeep(tState);
  if (updateState(hState, hum10, cfg.hLo * 10, cfg.hHi * 10, H_MARGE10))   alertBeep(hState);
}
bool anyAlert() { return tState != 0 || hState != 0; }

// ---------- Alarm ----------
void startRing() {
  if (alarmPreview) { stopAlarmPreview(); alarmSub = 0; }   // real alarm takes priority
  if (musicState == 1) {
    long remaining = (long)(musicNoteEnd - millis());
    musicRemain = remaining > 0 ? remaining : 0;
    musicState = 2;                     // resume manually after the alarm
  }
  musicToneStop();
  alarmPos = 0; alarmNoteEnd = millis();
  if (standby) { standby = false; lcd.clear(); }   // the alarm turns the screen back on
  ringing = true; snoozing = false;
  ringStart = millis();
  lastR = 255;                          // forces the colour update
}
void stopRing() {
  ringing = false; snoozing = false;
  lastR = 255;
  musicToneStop();
}

void checkAlarm() {
  if (ringing || !rtcPresent) return;   // no alarm without a reliable clock
  int minuteOfDay = nowH * 60 + nowM;
  if (!cfg.armed) snoozing = false;
  if (snoozing) {
    if ((long)(millis() - snoozeUntil) >= 0) startRing();
    return;
  }
  if (cfg.armed && nowH == cfg.aH && nowM == cfg.aM && lastFiredMin != minuteOfDay) {
    lastFiredMin = minuteOfDay;
    startRing();
  }
}

void handleRing(uint8_t e, bool draw) {
  unsigned long now = millis();
  unsigned long el = (now - ringStart) / 1000UL;

  if (e == EV_SHORT || e == EV_DOUBLE || el >= RING_MAX_S) {
    musicToneStop();
    ringing = false; snoozing = true; snoozeUntil = now + SNOOZE_MS;
    lastR = 255;
    return;
  }
  if (e == EV_LONG || e == EV_VLONG) { stopRing(); return; }

  alarmMelodyTick(now, alarmCfg.volume);

  if (draw) {
    color(255, el < 30 ? 90 : 0, 0);
    char b[17];
    snprintf_P(b, sizeof(b), TR("REVEIL  %02d:%02d", "ALARM   %02d:%02d", "ALARMA  %02d:%02d", "WECKER  %02d:%02d"), nowH, nowM);
    line(0, b);
    lineP(1, TR("Toucher = arret", "Touch = stop", "Tocar = parar", "Beruehren=Stopp"));
  }
}

// ---------- Pomodoro ----------
unsigned long phaseMs(uint8_t p) {
  if (p == 0) return (unsigned long)cfg.workMin * 60000UL;
  if (p == 1) return (unsigned long)SHORT_BREAK_MIN * 60000UL;
  return (unsigned long)LONG_BREAK_MIN * 60000UL;
}
bool pomoIdle() {
  return !pomoRun && pomoPhase == 0 && pomoCount == 0 && pomoRemMs == phaseMs(0);
}
void pomoReset() {
  pomoPhase = 0; pomoCount = 0; pomoRun = false; pomoRemMs = phaseMs(0);
}
void pomoNextPhase() {
  if (pomoPhase == 0) {
    pomoCount++;
    pomoPhase = (pomoCount % LONG_EVERY == 0) ? 2 : 1;
    beep(3, 150);
  } else {
    pomoPhase = 0;
    beep(2, 300);
  }
  pomoRemMs = phaseMs(pomoPhase);
}
void pomoTick() {
  unsigned long now = millis();
  if (pomoRun) {
    unsigned long dt = now - pomoLast;
    if (dt >= pomoRemMs) pomoNextPhase();
    else pomoRemMs -= dt;
  }
  pomoLast = millis();
}

// ---------- Screens ----------
// ---------- Navigation ----------
// values of the Settings page items (2 volume, 3 sound, 4 auto sleep, 5 night mode, 7 favorite)
uint8_t parValue(uint8_t i) { return i == 2 ? cfg.volume : i == 3 ? cfg.sound : i == 4 ? cfg.sleep : i == 5 ? cfg.night : i == 7 ? cfg.home : 0; }
void setParValue(uint8_t i, uint8_t v) { if (i == 2) cfg.volume = v; else if (i == 3) cfg.sound = v; else if (i == 4) cfg.sleep = v; else if (i == 5) cfg.night = v; else if (i == 7) cfg.home = v; }

void chessPause() {                    // leaving the page pauses the clock
  if (chessState == 1) { chessBase[chessSide] -= (long)(millis() - chessTurn); chessState = 2; }
}



void enterMode(uint8_t m) {
  stopAlarmPreview();
  if (mode == M_CHESS && m != M_CHESS) chessPause();
  if (mode == M_MUSIC && m != M_MUSIC) musicToneStop();
  mode = m;
  clockSub = 0; alarmSub = 0; alarmExitPending = false; alarmSavedPending = false; thSub = 0; thSavedPending = false; thView = 0; parIdx = 1; parSub = 0; tallySub = 0;
  if (m == M_DIE) dieRolling = false;
  if (m == M_MUSIC) { musicState = 0; musicPos = 0; musicRemain = 0; }
  potReset();
  if (m == M_SETCLOCK) { clockSub = 1; edH = nowH; edM = nowM; }
  if (m == M_BRIGHT) {
    brightBackup = cfg.bright;
    brightIdx = 0;
    for (uint8_t i = 1; i < 8; i++)
      if (abs((int)BRIGHT_LEVELS[i] - cfg.bright) < abs((int)BRIGHT_LEVELS[brightIdx] - cfg.bright)) brightIdx = i;
  }
  potReset();
  lastDraw = 0;
}

// Startup/wake-up: opens the page chosen in Settings (0 = menu)
void goHome() {
  uint8_t h = cfg.home;
  enterMode(h == 0 ? M_MENU : h == 1 ? M_TH : h == 2 ? M_POMO : h == 3 ? M_CHRONO : h == 4 ? M_TALLY : h == 5 ? M_DIE : M_CHESS);
}

// back to the Settings page, on setting number i
void backToParams(uint8_t i) { enterMode(M_PARAMS); parIdx = i; }

void openMenu() {
  stopAlarmPreview();
  if (mode == M_MUSIC) musicToneStop();
  if (mode == M_CHESS) chessPause();
  prevMode = mode;
  menuIdx = 0;                                         // The menu opens on the entry of the page just left (1/10 for Temp & Hum); the dial takes over as soon as it moves.
  { uint8_t menuMode(uint8_t); for (uint8_t i = 0; i < N_MENU - 1; i++) if (menuMode(i) == mode) { menuIdx = i; break; } }
  mode = M_MENU;
  clockSub = 0; alarmSub = 0; thSub = 0; parSub = 0; tallySub = 0;
  potReset();
  lastDraw = 0;
}

void backToMenu() {
  if (mode == M_MENU) { enterMode(prevMode); return; }          // in the menu: back to the page
  if (mode == M_BRIGHT) { cfg.bright = brightBackup; applyColor(); backToParams(1); return; }   // cancels the brightness change
  if (mode == M_ALARM && alarmSub != 0) saveCfg();
  if (mode == M_TH && thSub != 0) saveCfg();
  if (mode == M_PARAMS && parSub != 0) { setParValue(parIdx, parBackup); parSub = 0; potReset(); lastDraw = 0; return; }
  openMenu();
}

// Menu entry: dial position, spread over the ten entries.
uint8_t menuFromPot() {
  int z = potRead() / ((POT_MAX + 1) / N_MENU);
  return (uint8_t)(z > N_MENU - 1 ? N_MENU - 1 : z);
}

// letters of the active background functions, except the one of the displayed page (columns 13 to 15)
void badges(uint8_t self) {
  char all[5];                                       // order: ! A C M (M always stays the rightmost)
  uint8_t n = 0;
  if (self != M_TH     && anyAlert())                all[n++] = '!';
  if (self != M_ALARM  && cfg.armed && rtcPresent)   all[n++] = 'A';
  if (self != M_CHRONO && chronoRun)                 all[n++] = 'C';
  if (self != M_POMO   && pomoRun)                   all[n++] = ((millis() / 1000UL) % 10 == 9) ? ' ' : 'M';   // M for the timer on every page; absent for 1 s every 10 s
  char s[4] = { ' ', ' ', ' ', 0 };                  // columns 13 to 15, right-aligned
  uint8_t keep = n > 3 ? 3 : n;
  for (uint8_t i = 0; i < keep; i++) s[3 - keep + i] = all[n - keep + i];
  lcd.setCursor(13, 0);
  lcd.print(s);
}

// ---------- Pages ----------
uint8_t menuMode(uint8_t i);
void sleepNow();
// Timer colours: ready = blue, work = orange, timer paused = yellow, short break = green, long break = cyan.
void pomoColor() {
  if (pomoIdle()) color(40, 90, 160);
  else if (!pomoRun) color(200, 150, 0);
  else if (pomoPhase == 0) color(255, 70, 0);
  else if (pomoPhase == 1) color(0, 170, 50);
  else color(0, 170, 200);
}

void drawMenu(uint8_t e, bool draw) {
  static const char names[] PROGMEM =
  "Time Temp Hum\0"
  "Timer\0"
  "Counter\0"
  "Stopwatch\0"
  "Alarm\0"
  "Dice\0"
  "Chess Clock\0"
  "Melody\0"
  "Settings\0"
  "Sleep mode\0"
;
  if (potLive()) menuIdx = menuFromPot();
  if (e == EV_SHORT) { if (menuIdx == N_MENU - 1) sleepNow(); else enterMode(menuMode(menuIdx)); return; }
  if (e == EV_LONG || e == EV_VLONG) { enterMode(prevMode); return; }
  if (!draw) return;
  char b[17];
  uint8_t displayMode = mode;
  mode = menuIdx == N_MENU - 1 ? M_SLEEP : menuMode(menuIdx);
  if (mode == M_POMO) pomoColor(); else color(0, 0, 0);   // Same colour in the menu and in the page.
  mode = displayMode;
  snprintf_P(b, sizeof(b), PSTR("MENU       %2d/%d"), menuIdx + 1, N_MENU);
  line(0, b);
  char nm[20]; strcpy_P(nm, packedText(names, menuIdx));
  snprintf_P(b, sizeof(b), PSTR("> %s"), nm);
  line(1, b);
}

uint8_t menuMode(uint8_t i) {
  static const uint8_t modes[N_MENU] PROGMEM = { M_TH, M_POMO, M_TALLY, M_CHRONO, M_ALARM, M_DIE, M_CHESS, M_MUSIC, M_PARAMS, M_HOME };
  return pgm_read_byte(&modes[i]);
}

// text "T23.4C  H 45%"
void thText(char* b, size_t n) {
  if (shtOk) {
    int t10 = temp10;
    snprintf_P(b, n, PSTR("T%s%d.%dC  H%3d%%"), t10 < 0 ? "-" : "",
             abs(t10) / 10, abs(t10) % 10, (hum10 + 5) / 10);
  } else {
    snprintf_P(b, n, PSTR("T --.-C  H --%%"));
  }
}

void noRtcScreen() {
  color(60, 60, 120);
  lineP(0, TR("Installer module", "Install RTC", "Instalar modulo", "RTC-Modul noetig"));
  lineP(1, TR("RTC pour l'heure", "module for clock", "RTC para la hora", "fuer die Uhrzeit"));
}

void drawClock(bool draw) {
  if (!draw) return;
  if (!rtcPresent) { noRtcScreen(); return; }
  char b[17];
  color(40, 90, 160);                                 // dimmed by night mode
  snprintf_P(b, sizeof(b), PSTR("    %02d:%02d:%02d"), nowH, nowM, nowS);
  line(0, b);
  badges(M_CLOCK);
  thText(b, sizeof(b));
  line(1, b);
}

void introSound();
// possible startup pages: 0 menu, 1 Temp & Hum, 2 timer, 3 stopwatch, 4 counter, 5 dice, 6 chess clock
static const char homeName[] PROGMEM =
  "Menu\0"
  "Time Temp Hum\0"
  "Timer\0"
  "Stopwatch\0"
  "Counter\0"
  "Dice\0"
  "Chess clock\0"
;
// Settings: brightness, volume, startup sound, auto sleep, night mode, set time, favorite.

void drawParams(uint8_t e, bool draw) {
  if (e == EV_DOUBLE) { backToMenu(); return; }          // double click: back to the menu (cancels an open setting)
  if (parSub == 0) {
    if (potLive()) parIdx = 1 + potSelect(N_PAR, parIdx - 1, 30);
    if (e == EV_SHORT) {
      if (parIdx == 1)      { enterMode(M_BRIGHT); return; }
      else if (parIdx == 6) { enterMode(M_SETCLOCK); return; }
      else if (parIdx == 8) { parSub = 1; potReset(); return; }
      parSub = 1; parBackup = parValue(parIdx); potReset();
    }
  } else {
    if (potLive()) {
      uint8_t cur = parValue(parIdx);
      uint8_t count = parIdx == 2 ? 3 : parIdx == 3 ? 2 : parIdx == 4 ? 8 : parIdx == 5 ? 3 : parIdx == 7 ? 7 : 0;
      uint8_t v = potSelect(count, cur, parIdx == 2 ? 40 : 30);
      if (parIdx != 8 && v != cur) { setParValue(parIdx, v); if (parIdx == 2) beep(1, 100); }
    }
    if (e == EV_SHORT) {
      if (parIdx == 8) {
        cfg.magic = 0xAC; cfg.aH = 7; cfg.aM = 0; cfg.armed = 0; cfg.workMin = 25;
        cfg.bright = 85; cfg.sound = 0; cfg.volume = 1; cfg.home = 0; cfg.night = 1; cfg.sleep = 0;
        cfg.tLo2 = T_LO_DEFAULT2; cfg.tHi2 = T_HI_DEFAULT2; cfg.hLo = H_LO_DEFAULT; cfg.hHi = H_HI_DEFAULT;
        alarmCfg.magic = 0xB1; alarmCfg.melody = 0; alarmCfg.volume = 1;
        EEPROM.put(0, cfg); EEPROM.put(sizeof(cfg), alarmCfg);
        color(0, 200, 45); lineP(0, PSTR("Confirmation Ok")); lineP(1, PSTR(""));
        delay(1500); parSub = 0; backToParams(1); potReset(); applyColor(); return;
      } else {
        saveCfg(); parSub = 0; potReset();
        if (parIdx == 7) {
          color(0, 200, 45); lineP(0, PSTR("Confirmation Ok")); lineP(1, PSTR(""));
          delay(1500); backToParams(1); potReset(); applyColor(); return;
        }
      }
      if (parIdx == 3 && cfg.sound) introSound();
    }
  }
  if (!draw) return;
  char b[17], nm[14], val[14];
  color(40, 90, 160);
  strcpy_P(nm, parIdx == 1 ? TR_PTR(TXT_Luminosite_15, TXT_Brightness_7, TXT_Brillo_8, TXT_Helligkeit_13)
             : parIdx == 2 ? TR_PTR(TXT_Volume_31, TXT_Volume_31, PSTR("Volumen"), PSTR("Lautstaerke"))
             : parIdx == 3 ? TR("Son demarr.", "Boot sound", "Sonido inic.", "Startton")
             : parIdx == 4 ? TR("Veille auto", "Auto sleep", "Reposo auto", "Auto-Standby")
             : parIdx == 5 ? TR("Mode nuit", "Night mode", "Modo noche", "Nachtmodus")
             : parIdx == 6 ? TR("Regler heure", "Set time", "Ajustar hora", "Uhr stellen")
             : parIdx == 7 ? TR("Favori", "Favorite", "Favorito", "Favorit")
                           : TR("Regl. usine", "Factory", "Fabrica", "Werk"));
  snprintf_P(b, sizeof(b), PSTR("%-12s %d/%d"), nm, parIdx, N_PAR);
  line(0, b);
  if (parIdx == 6) {
    if (rtcPresent) snprintf_P(b, sizeof(b), PSTR("%02d:%02d        OK"), nowH, nowM);
    else            snprintf_P(b, sizeof(b), PSTR("--:--        OK"));
    line(1, b);
    return;
  }
  if (parIdx == 1) {
    snprintf_P(b, sizeof(b), PSTR("%3d%%         OK"), cfg.bright);
    line(1, b);
    return;
  }
  if (parIdx == 7) {
    strcpy_P(val, packedText(homeName, cfg.home));
    snprintf_P(b, sizeof(b), parSub ? TXT_12s_5 : TXT_12s_OK_1, val);
    line(1, b);
    return;
  }
  if (parIdx == 8) {
    lineP(1, parSub ? PSTR("OK confirm") : PSTR("OK=open"));
    return;
  }
  if (parIdx == 4) {
    uint8_t m = pgm_read_byte(&SLEEP_MIN[cfg.sleep]);
    if (m == 0)       strcpy_P(val, TR("Jamais", "Never", "Nunca", "Nie"));
    else if (m == 60) strcpy_P(val, PSTR("1 h"));
    else              snprintf_P(val, sizeof(val), TXT_u_min_4, m);
    snprintf_P(b, sizeof(b), parSub ? TXT_12s_5 : TXT_12s_OK_1, val);
    line(1, b);
    return;
  }
  if (parIdx == 5) {
    strcpy_P(val, cfg.night == 0 ? TR("Non", "Off", "No", "Aus") : cfg.night == 1 ? TR_PTR(TXT_Auto_22_6h_6, TXT_Auto_22_6h_6, TXT_Auto_22_6h_6, TXT_Auto_22_6h_6) : TR("Oui", "On", "Si", "An"));
    snprintf_P(b, sizeof(b), parSub ? TXT_12s_5 : TXT_12s_OK_1, val);
    line(1, b);
    return;
  }
  if (parIdx == 3) strcpy_P(val, cfg.sound ? TR_PTR(TXT_OUI_22, PSTR("ON"), TXT_SI_25, TXT_JA_14) : TR_PTR(TXT_NON_19, PSTR("OFF"), TXT_NO_18, TXT_NEIN_17));
  else {
    char bar[4] = "   ";
    for (uint8_t i = 0; i <= cfg.volume; i++) bar[i] = '#';                            // ### = loud
    strcpy_P(val, volumeName(cfg.volume));
    snprintf_P(b, sizeof(b), parSub ? PSTR("<> %-6s %-3s OK") : PSTR("%-6s %-3s  OK"), val, bar);
    line(1, b);
    return;
  }
  snprintf_P(b, sizeof(b), parSub ? PSTR("<> %-9s  OK") : PSTR("%-9s OK=chg"), val);
  line(1, b);
}

void drawTempHum(uint8_t e, bool draw);
// Home uses the combined measurements / clock page, without automatic page changes.
void drawHome(uint8_t e, bool draw) {
  if (e == EV_DOUBLE) { enterMode(M_TH); thSub = 1; potReset(); return; }   // set the thresholds
  if (e != EV_NONE || potLive()) { openMenu(); return; }   // button or dial: open the menu
  drawTempHum(EV_NONE, draw);
}

// Clock page alone (chosen in Settings > Home): button or dial = menu
void drawClockPage(uint8_t e, bool draw) {
  if (e != EV_NONE || potLive()) { openMenu(); return; }
  drawClock(draw);
}

uint8_t* thPtr(uint8_t sub) { return sub == 1 ? &cfg.tLo2 : sub == 2 ? &cfg.tHi2 : sub == 3 ? &cfg.hLo : &cfg.hHi; }

// sets threshold number 'sub' with the dial, keeping the minimum gap between low and high
void thEdit(uint8_t sub) {
  bool isT = (sub <= 2);
  uint8_t mn = isT ? T_MIN2 : H_MIN, mx = isT ? T_MAX2 : H_MAX, gap = isT ? T_GAP2 : H_GAP;
  uint8_t* p = thPtr(sub);
  uint8_t v = mn + potSelect(mx - mn + 1, *p - mn, 4);
  if (sub == 1 && v > cfg.tHi2 - gap) v = cfg.tHi2 - gap;
  if (sub == 2 && v < cfg.tLo2 + gap) v = cfg.tLo2 + gap;
  if (sub == 3 && v > cfg.hHi - gap)  v = cfg.hHi - gap;
  if (sub == 4 && v < cfg.hLo + gap)  v = cfg.hLo + gap;
  *p = v;
}

void drawTempHum(uint8_t e, bool draw) {
  if (thSavedPending) {
    if ((long)(millis() - thSavedReturnAt) >= 0) {
      thSavedPending = false;
      thSub = 0;
      potReset();
      e = EV_NONE;
      draw = true;
    } else {
      if (!draw) return;
      color(150, 100, 0);
      lineP(0, TR("     Choix      ", "     Choose     ", "     Elegir     ", "    Auswahl     "));
      lineP(1, PSTR("       OK       "));
      lcd.setCursor(15, 1); lcd.write((uint8_t)3);
      return;
    }
  }
  if (e == EV_LONG && thSub != 0) { thSub--; potReset(); e = EV_NONE; }
  if (thSub == 0) {
    if (e == EV_DOUBLE) { thSub = 1; potReset(); }
    else if (e != EV_NONE) { openMenu(); return; }
  } else if (thSub == 5) {
    if (e == EV_SHORT) {
      saveCfg(1500UL);
      thSavedPending = true;
      thSavedReturnAt = millis() + 1500UL;
      potReset();
    }
  } else if (e == EV_SHORT) {
    if (thSub < 4) { thSub++; potReset(); }
    else { thSub = 5; potReset(); }
  }
  if (thSub >= 1 && thSub <= 4 && potLive()) thEdit(thSub);
  if (thSub == 0 && potLive()) thView = potSelect(5, thView, 40);
  if (!draw) return;
  char b[17];
  const char* pre = (thView == 4 && (millis() / 1500UL) % 2 == 0) ? "AUT " : "<>  ";   // pages 1 to 4: <> arrows left of the clock; page 5: "AUT" alternating with the arrows (1.5 s); the clock occupies columns 4 to 11
  if (rtcPresent) snprintf_P(b, sizeof(b), PSTR("%s%02d:%02d:%02d"), pre, nowH, nowM, nowS);
  else snprintf_P(b, sizeof(b), PSTR("%s--:--:--"), pre);
  line(0, b); badges(M_TH);
  if (thSub == 5) {
    color(150, 100, 0);
    lineP(0, TR("     Choix      ", "     Choose     ", "     Elegir     ", "    Auswahl     "));
    lineP(1, PSTR("       OK       "));
    lcd.setCursor(15, 1); lcd.write((uint8_t)3);
    return;
  }
  if (thSub != 0) {
    color(150, 100, 0);
    uint8_t v = *thPtr(thSub);
    if (thSub <= 2) snprintf_P(b, sizeof(b), PSTR("<> %s %d.%d%c"), thSub == 1 ? "Tmin" : "Tmax", v / 2, (v % 2) * 5, 0xDF);
    else snprintf_P(b, sizeof(b), PSTR("<> %s %d%%"), thSub == 3 ? "Hmin" : "Hmax", v);
    line(1, b);
    lcd.setCursor(15, 1); lcd.write((uint8_t)3);
    return;
  }
  if (!shtOk) {
    color(60, 60, 60);
    lineP(1, TR_PTR(TXT_Capteur_absent_9, TXT_Sensor_missing_29, TXT_Sensor_ausente_27, TXT_Sensor_fehlt_28));
    return;
  }

  // Second row: measurements / full alert, with measurements before each alert.
  uint8_t view = thView;                                           // 0 readings, 1 minimum, 2 maximum, 3 average, 4 AUT (cycles through the four pages every 3 s)
  if (view == 4) view = (millis() / 3000UL) % 4;
  if (view == 1 || view == 2) {
    int16_t tLo = thTempSamples[0], tHi = thTempSamples[0];
    int16_t hLo = thHumSamples[0], hHi = thHumSamples[0];
    for (uint8_t i = 1; i < thSampleCount; i++) {
      if (thTempSamples[i] < tLo) tLo = thTempSamples[i];
      if (thTempSamples[i] > tHi) tHi = thTempSamples[i];
      if (thHumSamples[i] < hLo) hLo = thHumSamples[i];
      if (thHumSamples[i] > hHi) hHi = thHumSamples[i];
    }
    bool showMin = (view == 1);
    int16_t tExtreme = showMin ? tLo : tHi;
    int16_t hExtreme = showMin ? hLo : hHi;
    int textLen = snprintf_P(b, sizeof(b), PSTR("%s T%s%d.%d%c H%d%%"), showMin ? "MIN" : "MAX",
                             tExtreme < 0 ? "-" : "", abs(tExtreme)/10, abs(tExtreme)%10, 0xDF, (hExtreme + 5)/10);
    if (textLen > 16)
      snprintf_P(b, sizeof(b), PSTR("%sT%s%d.%d%cH%d%%"), showMin ? "MIN" : "MAX",
                 tExtreme < 0 ? "-" : "", abs(tExtreme)/10, abs(tExtreme)%10, 0xDF, (hExtreme + 5)/10);
    line(1, b);
    return;
  }
  if (view == 3) {
    int16_t tAvg = thSampleCount ? thTempSum / thSampleCount : temp10;
    int16_t hAvg = thSampleCount ? thHumSum / thSampleCount : hum10;
    int textLen = snprintf_P(b, sizeof(b), TR("MOY T%s%d.%d%c H%d%%", "AVG T%s%d.%d%c H%d%%", "PROM T%s%d.%d%c H%d%%", "DUR T%s%d.%d%c H%d%%"), tAvg < 0 ? "-" : "", abs(tAvg)/10, abs(tAvg)%10, 0xDF, (hAvg + 5)/10);
    if (textLen > 16)
      snprintf_P(b, sizeof(b), TR("MOYT%s%d.%d%cH%d%%", "AVGT%s%d.%d%cH%d%%", "PROMT%s%d.%d%cH%d%%", "DURT%s%d.%d%cH%d%%"), tAvg < 0 ? "-" : "", abs(tAvg)/10, abs(tAvg)%10, 0xDF, (hAvg + 5)/10);
    line(1, b);
    return;
  }
  PGM_P msg[2]; uint8_t states[2], n = 0;
  if (tState) {
    states[n] = tState;
    msg[n++] = tState == 1 ? TR("TEMP TROP HAUTE", "TEMP TOO HIGH", "TEMP MUY ALTA", "TEMP ZU HOCH") : TR("TEMP TROP BASSE", "TEMP TOO LOW", "TEMP MUY BAJA", "TEMP ZU TIEF");
  }
  if (hState) {
    states[n] = hState;
    msg[n++] = hState == 1 ? TR("HUM TROP HAUTE", "HUM TOO HIGH", "HUM MUY ALTA", "FEUCHTE ZU HOCH") : TR("HUM TROP BASSE", "HUM TOO LOW", "HUM MUY BAJA", "FEUCHTE ZU TIEF");
  }
  uint8_t slot = n ? (millis() / 2000UL) % (2 * n) : 0;
  uint8_t state = n ? states[slot / 2] : 0;
  if (state == 1) color(255, 0, 0);
  else if (state == 2) color(0, 80, 255);
  else color(0, 160, 40);
  if (slot & 1) { lineP(1, msg[slot / 2]); return; }
  // Degrees use LCD character 0xDF; measurements occupy at most 13 columns.
  snprintf_P(b, sizeof(b), PSTR("T%s%d.%d%c H%d%%"), temp10 < 0 ? "-" : "",
             abs(temp10) / 10, abs(temp10) % 10, 0xDF, (hum10 + 5) / 10);
  line(1, b);
  if (!n) { lcd.setCursor(14, 1); lcd.print("OK"); }
}

void drawSetClock(uint8_t e, bool draw) {
  if (e == EV_LONG) { if (clockSub > 1) { clockSub--; potReset(); } else { backToParams(6); return; } e = EV_NONE; }
  if (!rtcPresent) {
    if (e != EV_NONE || potLive()) backToParams(6);
    else if (draw) noRtcScreen();
    return;
  }
  if (e == EV_SHORT) {
    if (clockSub == 1) { clockSub = 2; potReset(); }
    else { setClock(edH, edM); backToParams(6); return; }
  }
  if (clockSub == 1 && potLive()) edH = potSelect(24, edH, 4);
  if (clockSub == 2 && potLive()) edM = potSelect(60, edM, 4);

  if (!draw) return;
  char b[17];
  color(150, 100, 0);
  lineP(0, clockSub == 1 ? TR("Heure : heures", "Time: hours", "Hora: horas", "Zeit: Stunden") : TR("Heure : minutes", "Time: minutes", "Hora: minutos", "Zeit: Minuten"));
  snprintf_P(b, sizeof(b), clockSub == 1 ? TR("%02d:%02d OK=suite", "%02d:%02d OK=next", "%02d:%02d OK=sigue", "%02d:%02d OK=weiter") : TR("%02d:%02d OK=valider", "%02d:%02d OK=confirm", "%02d:%02d OK=acepta", "%02d:%02d OK=fertig"), edH, edM);
  line(1, b);
}

bool serviceAlarmExit(bool draw) {
  if (!alarmExitPending) return false;
  if ((long)(millis() - alarmExitAt) >= 0) {
    alarmExitPending = false;
  alarmSavedPending = false;
    openMenu();
    return true;
  }
  if (draw) {
    color(150, 70, 0);
    lineP(0, TR("Desactivation", "Disabling", "Desactivando", "Deaktivieren"));
    lineP(1, TR("OK", "OK", "OK", "OK"));
  }
  return true;
}
void drawAlarm(uint8_t e, bool draw) {
  if (e == EV_LONG && alarmSub != 0 && !alarmSavedPending) { alarmSub--; potReset(); e = EV_NONE; }
  if (!rtcPresent) {
    if (e != EV_NONE || potLive()) { openMenu(); return; }
    if (draw) { color(60, 60, 120); lineP(0, TR("Alarme : il faut", "Alarm: needs the", "Alarma: requiere", "Wecker: braucht")); lineP(1, TR("le module RTC", "RTC module", "modulo RTC", "RTC-Modul")); }
    return;
  }
  if (alarmSavedPending) {
    if ((long)(millis() - alarmSavedReturnAt) >= 0) {
      alarmSavedPending = false;
      alarmSub = 0;
      openMenu();
      return;
    } else {
      if (!draw) return;
      color(150, 70, 0);
      lineP(0, TR("     Alarme     ", "     Alarm      ", "     Alarma     ", "     Wecker     "));
      lineP(1, PSTR("       OK       "));
      return;
    }
  }
  if (serviceAlarmExit(draw)) return;
  if (alarmSub == 0) {
    if (e == EV_SHORT)             { cfg.armed = 1; potReset(); saveCfg(); alarmSub = 1; }   // Short press: enable, then enter the settings.
    else if (e == EV_DOUBLE)       { cfg.armed = 0; potReset(); saveCfg(); alarmExitPending = true; alarmExitAt = millis() + 1500UL; } // Double click: disable, then leave after confirmation.
    else if (potLive())          { openMenu(); return; }
  } else if (e == EV_SHORT) {
    stopAlarmPreview();
    potReset();
    if (alarmSub == 4) { alarmSub = 5; saveCfg(1500UL); alarmSavedPending = true; alarmSavedReturnAt = millis() + 1500UL; }
    else if (alarmSub == 5) alarmSub = 0;
    else alarmSub++;
  }

  if (serviceAlarmExit(draw)) return;
  if (alarmSub == 1 && potLive()) cfg.aH = potSelect(24, cfg.aH, 4);
  if (alarmSub == 2 && potLive()) cfg.aM = potSelect(60, cfg.aM, 4);
  if (alarmSub == 3 && potLive()) alarmCfg.melody = potSelect(N_ALARM_TUNES, alarmCfg.melody, 15);
  if (alarmSub == 4 && potLive()) alarmCfg.volume = potSelect(3, alarmCfg.volume, 20);
  updateAlarmPreview();

  if (!draw) return;
  char b[17];
  if (alarmSub == 0) {
    color(150, 70, 0);
    lineP(0, TR("CLIC: Activer", "CLICK: Enable", "CLIC: Activar", "KLICK: Aktivier."));
    lineP(1, TR("2 CLICS: Desact.", "2 CLICKS: Off", "2 CLICS: Desact", "2 KLICKS: Aus"));
    return;
  }
  if (alarmSub == 5) {
    color(150, 70, 0);
    lineP(0, TR("     Alarme     ", "     Alarm      ", "     Alarma     ", "     Wecker     "));
    lineP(1, PSTR("       OK       "));
    return;
  }
  color(150, 100, 0);
  switch (alarmSub) {
    case 1:  lineP(0, TR("Alarme : heures", "Alarm: hours", "Alarma: horas", "Wecker: Stunden"));  snprintf_P(b, sizeof(b), PSTR("%02d h"), cfg.aH); break;
    case 2:  lineP(0, TR("Alarme : minutes", "Alarm: minutes", "Alarma: minutos", "Wecker: Minuten")); snprintf_P(b, sizeof(b), PSTR("%02d min"), cfg.aM); break;
    case 3:  lineP(0, TR("Alarme: melodie", "Alarm: melody", "Alarma: melodia", "Wecker: Melodie")); strcpy_P(b, alarmTuneName()); break;
    default: lineP(0, TR("Alarme: volume", "Alarm: volume", "Alarma: volumen", "Wecker: Lautst."));
             strcpy_P(b, volumeName(alarmCfg.volume)); break;
  }
  line(1, b);
}

void drawPomo(uint8_t e, bool draw) {
  if (e == EV_DOUBLE && pomoIdle()) { openMenu(); return; }   // already at zero: double click = back to the menu
  if (e == EV_DOUBLE) { pomoReset(); potReset(); }   // reset ; otherwise the timer keeps running in the background
  else if (e == EV_SHORT) {
    if (pomoIdle()) saveCfg();                       // remembers the chosen duration
    pomoRun = !pomoRun;
    pomoLast = millis();
    potReset();                                      // the dial only counts from now on
  }

  // Once started, the dial opens the menu without stopping the countdown.
  if (!pomoIdle() && potLive()) { openMenu(); return; }
  if (pomoIdle() && potLive()) {                     // when stopped: the potentiometer sets the duration
    uint8_t cur = workIndex(cfg.workMin);
    uint8_t idx = potSelect(N_WORK, cur, 3);
    if (idx != cur) { cfg.workMin = workValue(idx); pomoRemMs = phaseMs(0); }
  }

  if (!draw) return;
  char b[17];
  unsigned long rs = (pomoRemMs + 999UL) / 1000UL;
  if (pomoPhase == 0)      { strcpy_P(b, TR_PTR(PSTR("MINUTEUR"), PSTR("TIMER"), PSTR("TEMPORIZ."), PSTR("TIMER"))); }
  else if (pomoPhase == 1) { snprintf_P(b, sizeof(b), TR("PAUSE COURTE", "SHORT BREAK", "PAUSA CORTA", "KURZE PAUSE")); }
  else                     { snprintf_P(b, sizeof(b), TR("PAUSE LONGUE", "LONG BREAK", "PAUSA LARGA", "LANGE PAUSE")); }
  pomoColor();
  line(0, b);
  badges(M_POMO);
  char t[12], d[12];
  if (pomoIdle()) {
    fmtDuration(d, sizeof(d), cfg.workMin);
    snprintf_P(b, sizeof(b), PSTR("%s  OK=start"), d);
  } else {
    if (rs >= 3600) snprintf_P(t, sizeof(t), PSTR("%d:%02d:%02d"), (int)(rs / 3600), (int)((rs / 60) % 60), (int)(rs % 60));
    else            snprintf_P(t, sizeof(t), PSTR("%02d:%02d"), (int)(rs / 60), (int)(rs % 60));
    snprintf_P(b, sizeof(b), pomoRun ? TR_PTR(PSTR("%s en cours"), TXT_s_running_3, PSTR("%s en curso"), TXT_s_laeuft_2) : TR("%s pause", "%s paused", "%s pausa", "%s Pause"), t);
  }
  line(1, b);
}

void drawChrono(uint8_t e, bool draw) {
  bool idle = !chronoRun && chronoMs() == 0;       // Chronomètre à zéro, en attente d’un appui court.
  if (e == EV_DOUBLE && idle) { openMenu(); return; }   // Si déjà à zéro : double clic pour revenir au menu.
  if (e == EV_DOUBLE) { chronoRun = false; chronoAccum = 0; potReset(); }   // Remise à zéro ; sinon le chronomètre continue en arrière-plan.
  else if (e == EV_SHORT) {
    if (chronoRun) { chronoAccum = millis() - chronoStart; chronoRun = false; }
    else           { chronoStart = millis() - chronoAccum; chronoRun = true; }
    potReset();                                      // La molette est prise en compte à partir de cet instant.
  }
  if (potLive()) { openMenu(); return; }       // La molette ouvre le menu ; le chronomètre continue en arrière-plan.

  if (!draw) return;
  color(110, 40, 170);
  unsigned long ms = chronoMs();
  unsigned long h = ms / 3600000UL;
  unsigned long m = (ms / 60000UL) % 60;
  unsigned long sec = (ms / 1000UL) % 60;
  char t[12], b[17];
  if (h > 0) snprintf_P(t, sizeof(t), PSTR("%lu:%02lu:%02lu"), h % 100, m, sec);
  else       snprintf_P(t, sizeof(t), PSTR("%02lu:%02lu.%lu"), m, sec, (ms / 100UL) % 10);
  lineP(0, TR("CHRONO", "STOPWATCH", "CRONOMETRO", "STOPPUHR"));
  badges(M_CHRONO);
  snprintf_P(b, sizeof(b), chronoRun ? TR_PTR(PSTR("%s marche"), TXT_s_running_3, PSTR("%s marcha"), TXT_s_laeuft_2) : (ms == 0 ? PSTR("%s OK=start") : TR("%s arret", "%s stopped", "%s parado", "%s gestoppt")), t);
  line(1, b);
}

void drawTally(uint8_t e, bool draw, bool tap) {
  if (e == EV_LONG && tallySub != 0) { tallySub = 0; potReset(); e = EV_NONE; }
  if (tallySub == 0) {
    if (tap && tallyCount < 9999) { tallyCount++; lastDraw = 0; }   // Each press is counted immediately.
    if (potLive()) { tallySub = 1; tallyT0 = millis(); }            // Dial: -1 or reset.
  } else {
    bool wait = millis() - tallyT0 <= 600;
    if (!wait && e == EV_DOUBLE)     { tallyCount = 0; tallySub = 0; potReset(); }
    else if (!wait && e == EV_SHORT) { if (tallyCount) tallyCount--; tallySub = 0; potReset(); }
    else if (millis() - tallyT0 > 5000) { tallySub = 0; potReset(); }
  }
  if (!draw) return;
  char b[17];
  if (tallySub != 0) {
    color(150, 100, 0);
    lineP(0, TR("-1 ou RAZ ?", "-1 or reset?", "-1 o a cero?", "-1 oder Reset?"));
    lineP(1, TR("OK=-1  dbl=RAZ", "OK=-1  dbl=RESET", "OK=-1 dbl=cero", "OK=-1 dbl=Reset"));
    return;
  }
  color(0, 130, 130);
  lineP(0, TR("COMPTEUR", "COUNTER", "CONTADOR", "ZAEHLER"));
  badges(M_TALLY);
  snprintf_P(b, sizeof(b), PSTR("%16u"), tallyCount);
  line(1, b);
}

void drawDie(uint8_t e, bool draw, bool tap) {
  if (e == EV_DOUBLE) { openMenu(); return; }          // double click: back to the menu
  uint8_t faces = pgm_read_byte(&DIE_FACES[dieIdx]);
  if (dieRolling) {
    if ((long)(millis() - dieEnd) >= 0) {              // end of the roll: final result
      seedRng(micros() + analogRead(PIN_POT));
      dieVal = (dieShow >= 1 && dieShow <= faces) ? dieShow : rollDie(faces);   // The result is the last number shown: it no longer changes when the roll stops.
      dieRolling = false;
      beep(1, 60);
      lastDraw = 0;
    }
  } else {
    if (potLive()) {                                   // the dial chooses the number of sides
      uint8_t n = potSelect(N_DIE, dieIdx, 20);
      if (n != dieIdx) { dieIdx = n; dieVal = 0; faces = pgm_read_byte(&DIE_FACES[dieIdx]); }
    }
    if (tap) {                                         // each press rolls the die immediately
      seedRng(micros() + analogRead(PIN_POT));
      dieRolling = true; dieEnd = millis() + 900; dieVal = 0; dieShow = 0;
    }
  }
  if (!draw) return;
  char b[17];
  color(170, 60, 130);
  // Line 1: title and gesture hint (columns 13 to 15 are kept for the indicators); line 2: D + faces on the left, text or number right-aligned.
  if (dieRolling)       lineP(0, TR("DE  lance...", "DICE rolling", "DADO tirando", "WUERFEL rollt"));
  else if (dieVal != 0) lineP(0, TR("DE OK=relance", "DICE OK=roll", "DADO OK=tirar", "WUERFEL OK"));
  else                  lineP(0, TR("DE  <> faces", "DICE <> sides", "DADO <> caras", "WUERFEL <>"));
  badges(M_DIE);
  char r[12];                                          // partie droite de la ligne 2, alignée sur la colonne 15
  if (dieRolling) {
    dieShow = rollDie(faces);
    buzz(1800, 5);                                     // small tick during the roll
    snprintf_P(r, sizeof(r), PSTR("%3u"), dieShow);
  } else if (dieVal != 0) {
    snprintf_P(r, sizeof(r), PSTR("[%3u]"), dieVal);
  } else {
    strcpy_P(r, TR("OK=lancer", "OK=roll", "OK=tirar", "OK=wuerfeln"));
  }
  uint8_t ln = snprintf_P(b, sizeof(b), PSTR("D%-3u"), faces);
  uint8_t rl = strlen(r);
  while (ln < 16 - rl) b[ln++] = ' ';
  strcpy(b + ln, r);
  line(1, b);
}

// Music: dial = choose a track, short press = play / pause / resume, double click = back to the menu.
void drawMusic(uint8_t e, bool draw) {
  if (e == EV_DOUBLE) { openMenu(); return; }
  if (e == EV_PRESS || e == EV_SHORT) {
    unsigned long now = millis();
    if (musicState == 1) {
      long remaining = (long)(musicNoteEnd - now);
      musicRemain = remaining > 0 ? remaining : 0;
      musicToneStop();
      musicState = 2;
    } else {
      musicState = 1;
      if (musicRemain > 0) {
        musicNoteEnd = now + musicRemain;
        if (musicFreq) musicToneStart(musicFreq); else musicToneStop();
      }
    }
    lastDraw = 0;
  }
  if (musicState != 1 && potLive()) {
    uint8_t selected = potSelect(N_MUSIC, musicTrack, 15);
    if (selected != musicTrack) {
      musicToneStop();
      musicTrack = selected; musicState = 0; musicPos = 0; musicRemain = 0; lastDraw = 0;
    }
  }
  if (musicState == 1) {
    unsigned long now = millis();
    if (musicRemain > 0 && (long)(now - musicNoteEnd) >= 0) musicRemain = 0;
    if (musicRemain <= 0) {
      uint8_t noteCount = pgm_read_byte(&MUSIC_NOTE_COUNTS[musicTrack]);
      if (musicPos >= noteCount) { musicToneStop(); musicState = 0; musicPos = 0; lastDraw = 0; }
      else {
        musicRemain = playTrackNote(musicTrack, musicPos, cfg.volume, musicFreq);
        musicPos++;
        musicNoteEnd = now + musicRemain;
      }
    }
  }
  if (!draw) return;
  color(120, 60, 180);
  char b[17], title[13];
  strcpy_P(title, packedText(MUSIC_NAMES, musicTrack));
  snprintf_P(b, sizeof(b), PSTR("%u/%u %s"), musicTrack + 1, N_MUSIC, title);
  line(0, b);
  badges(M_MUSIC);
  if (musicState == 1)      lineP(1, TR("Lecture OK=pause", "Playing OK=pause", "Suena OK=pausa", "Spielt OK=Pause"));
  else if (musicState == 2) lineP(1, TR("Pause OK=lire", "Paused OK=play", "Pausa OK=tocar", "Pause OK=Start"));
  else                   lineP(1, TR("Molette=choix OK", "Dial=choose OK", "Dial=gira OK", "Dreh=wahl OK"));
}

// Chess clock: a short press hands over to the other player, the dial chooses the time control (when stopped)
void drawChess(uint8_t e, bool draw) {
  if (chessState == 0) {
    if (e == EV_DOUBLE) { openMenu(); return; }
    if (potLive()) chessIdx = potSelect(N_CHESS, chessIdx, 20);
    if (e == EV_SHORT) {                               // start: player 1 begins
      long t = (long)pgm_read_byte(&CHESS_MIN[chessIdx]) * 60000L;
      chessBase[0] = t; chessBase[1] = t;
      chessSide = 0; chessState = 1; chessTurn = millis();
      beep(1, 60); lastR = 255;
    }
  } else if (chessState == 1) {
    if (e == EV_SHORT) {                               // player change, at the instant of the press
      unsigned long at = tapMs;
      if ((long)(at - chessTurn) < 0) at = chessTurn;
      chessBase[chessSide] += (long)pgm_read_byte(&CHESS_INC[chessIdx]) * 1000L - (long)(at - chessTurn);
      chessSide ^= 1; chessTurn = at;
      buzz(2000, 10);
    } else if (e == EV_DOUBLE) chessPause();
    if (chessState == 1 && chessLeft(chessSide) == 0) {   // flag fallen
      chessBase[chessSide] = 0; chessState = 3;
      beep(3, 200); lastR = 255;
    }
  } else if (chessState == 2) {
    if (e == EV_SHORT)       { chessState = 1; chessTurn = millis(); lastR = 255; }   // resume
    else if (e == EV_DOUBLE) { chessState = 0; lastR = 255; }                          // reset
  } else {
    if (e == EV_SHORT || e == EV_DOUBLE) { chessState = 0; lastR = 255; }
  }
  if (!draw) return;
  char b[17], tag[10];
  if (chessState == 0) {
    color(40, 90, 160);
    lineP(0, TR("PEND. ECHECS", "CHESS CLOCK", "AJEDREZ", "SCHACHUHR"));
    badges(M_CHESS);
    strcpy_P(tag, TR_PTR(TXT_OK_start_21, TXT_OK_start_21, PSTR("OK=inicio"), PSTR("OK=Start")));
    snprintf_P(b, sizeof(b), PSTR("%u+%u  %s"), pgm_read_byte(&CHESS_MIN[chessIdx]), pgm_read_byte(&CHESS_INC[chessIdx]), tag);
    line(1, b);
    return;
  }
  if (chessState == 3) color(255, 0, 0);
  else if (chessState == 2) color(150, 100, 0);
  else if (chessSide == 0) color(0, 110, 200);
  else color(200, 110, 0);
  for (uint8_t sd = 0; sd < 2; sd++) {
    long sec = (chessLeft(sd) + 999) / 1000;
    tag[0] = 0;
    if (chessState == 3 && sd == chessSide) strcpy_P(tag, TR_PTR(TXT_FIN_12, PSTR("END"), TXT_FIN_12, PSTR("ENDE")));
    else if (chessState == 2 && sd == chessSide) strcpy_P(tag, TR_PTR(TXT_PAUSE_23, TXT_PAUSE_23, PSTR("PAUSA"), TXT_PAUSE_23));
    snprintf_P(b, sizeof(b), PSTR("%c%d  %02d:%02d  %s"), (chessState != 3 && sd == chessSide) ? '>' : ' ', sd + 1, (int)(sec / 60), (int)(sec % 60), tag);
    line(sd, b);
  }
}

void drawBright(uint8_t e, bool draw) {
  if (e == EV_SHORT) { saveCfg(); backToParams(1); return; }

  if (potLive()) {
    brightIdx = potSelect(8, brightIdx, 10);
    cfg.bright = BRIGHT_LEVELS[brightIdx];
    applyColor();                                    // immediate preview
  }

  if (!draw) return;
  char bar[9], b[17];
  for (uint8_t i = 0; i < 8; i++) bar[i] = (i <= brightIdx) ? '#' : '-';
  bar[8] = 0;
  color(40, 90, 160);
  lineP(0, TR_PTR(TXT_Luminosite_15, TXT_Brightness_7, TXT_Brillo_8, TXT_Helligkeit_13));
  snprintf_P(b, sizeof(b), PSTR("<> %3d%% %s"), cfg.bright, bar);
  line(1, b);
}

// ---------- Startup intro ----------
const char BRAND1[] PROGMEM = "PR BELAMINE";     // 2nd intro screen, line 1
const char BRAND2[] PROGMEM = "MAKERLAB";        // 2nd intro screen, line 2

void centered(uint8_t row, const char* t) {
  char b[17];
  uint8_t len = strlen(t);
  if (len > 16) len = 16;
  uint8_t pad = (16 - len) / 2;
  memset(b, ' ', 16);                            // manual centering: "%*s" does not exist on the UNO
  memcpy(b + pad, t, len);
  b[16] = 0;
  line(row, b);
}
void centeredP(uint8_t row, PGM_P s) {
  char t[17];
  strncpy_P(t, s, 16); t[16] = 0;
  centered(row, t);
}
void introSound() {                              // Four-note melody.
  if (!cfg.sound || nightActive()) return;
  buzz(1500, 90); delay(40);
  buzz(2000, 90); delay(40);
  buzz(2600, 90); delay(40);
  buzz(3100, 340);                               // Fourth note, held longer.
  digitalWrite(PIN_BUZZER, LOW);
}
void intro() {
  lastR = 40; lastG = 90; lastB = 160;
  for (uint8_t i = 0; i <= 100; i += 5) {        // the backlight fades in slowly
    lcd.setRGB(40 * i / 100 * cfg.bright / 100, 90 * i / 100 * cfg.bright / 100, 160 * i / 100 * cfg.bright / 100);
    if (i == 0) { centeredP(0, TXT_DESK_WATCH_11); centeredP(1, TXT_EMPTY_0); }
    delay(25);
  }
  centeredP(0, TXT_DESK_WATCH_11); centeredP(1, TXT_EMPTY_0);   // rewritten: in case the screen was not ready
  introSound();
  delay(1000);
  centeredP(0, BRAND1); centeredP(1, BRAND2);      // second screen: the brand
  delay(1500);
  centeredP(0, TR("MELODIE EDITION", "MELODY EDITION", "MELODIA EDITION", "MELODIE EDITION"));
  lcd.setCursor(0, 1);
  for (uint8_t i = 0; i < 16; ++i) { lcd.write((uint8_t)255); delay(60); }
}

// ---------- Sleep (very long press) ----------
void sleepNow() {
  stopAlarmPreview();
  mode = M_SLEEP;
  color(255, 0, 0);
  centeredP(0, TR("A bientot", "Goodbye", "Hasta pronto", "Tschuess"));
  centeredP(1, TXT_EMPTY_0);
  delay(700);
  lcd.clear();
  lcd.setRGB(0, 0, 0);
  standby = true;
  lastR = 255;
}
void wakeUp() {
  standby = false;
  lastActivity = millis();
  lcd.clear();
  goHome();
  lastR = 255; lastDraw = 0;
  potReset();
  if (cfg.sound) introSound();
}

// ---------- Program ----------
void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  Wire.begin();
  loadCfg();
  lcd.begin(16, 2);
  {                                              // Menu arrows: down, up, right (characters 1 to 3; zero ends the texts).
    static const uint8_t ARROWS[3][8] PROGMEM = {
      { 0x04, 0x04, 0x04, 0x04, 0x15, 0x0E, 0x04, 0x00 },
      { 0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x04, 0x00 },
      { 0x00, 0x04, 0x02, 0x1F, 0x02, 0x04, 0x00, 0x00 } };
    for (uint8_t i = 0; i < 3; i++) { uint8_t g[8]; memcpy_P(g, ARROWS[i], 8); lcd.createChar(i + 1, g); }
  }
  delay(150);                                    // lets the screen finish its initialization

  intro();

  Wire.beginTransmission(RTC_ADDR);
  rtcPresent = (Wire.endTransmission() == 0);
  pomoReset();
  readClock();
  shtOk = readSht();
  if (shtOk) updateAlerts();

  color(40, 90, 160);                            // module check
  centeredP(0, rtcPresent ? TR("Horloge RTC OK", "RTC clock OK", "Reloj RTC OK", "RTC-Uhr OK") : TR("Sans horloge RTC", "No RTC clock", "Sin reloj RTC", "Keine RTC-Uhr"));
  centeredP(1, shtOk ? TR_PTR(PSTR("Capteur OK"), TXT_Sensor_OK_26, TXT_Sensor_OK_26, TXT_Sensor_OK_26) : TR_PTR(TXT_Capteur_absent_9, TXT_Sensor_missing_29, TXT_Sensor_ausente_27, TXT_Sensor_fehlt_28));
  delay(1200);
  goHome();
  potReset();
  lastPotSample = potRead();
  lastActivity = millis();
}

void loop() {
  uint8_t e = readButton();
  bool tap = tapFlag; tapFlag = false;
  bool dial = dialAct; dialAct = false;
  unsigned long now = millis();

  if (now - lastClockRead >= 200) { lastClockRead = now; readClock(); }
  if (now - lastSht >= 2000)      { lastSht = now; shtOk = readSht(); if (shtOk) updateAlerts(); }

  pomoTick();
  checkAlarm();
  if (!standby && nightActive() != nightPrev) applyColor();      // automatic night mode starts or stops
  updateValidationFlash();

  digitalWrite(PIN_LED, standby ? HIGH : LOW);   // LED on in sleep mode.

  // activity: press or dial movement (sampled every 200 ms)
  if (e != EV_NONE || tap || dial) lastActivity = now;
  if ((ringing || snoozing) && (e != EV_NONE || tap || dial)) {   // Ringing or snoozing: any press or turn of the dial stops today's alarm and opens the menu.
    stopRing();
    if (standby) { standby = false; lcd.clear(); }
    openMenu();
    return;
  }

  if (standby && !ringing && (e != EV_NONE || dial)) { lastActivity = now; wakeUp(); e = EV_NONE; }   // a press wakes it up
  if (!ringing && !standby && cfg.sleep != 0 && !(mode == M_CHESS && chessState == 1) &&
      now - lastActivity >= (unsigned long)pgm_read_byte(&SLEEP_MIN[cfg.sleep]) * 60000UL) { sleepNow(); e = EV_NONE; }   // auto sleep
  if (standby && !ringing) return;                    // screen off: only the alarm and the background functions keep running
  if (!ringing && e == EV_LONG && !(mode == M_TH && thSub != 0) && !(mode == M_ALARM && alarmSub != 0) && mode != M_SETCLOCK && !(mode == M_TALLY && tallySub != 0)) { backToMenu(); e = EV_NONE; }

  unsigned long period = (mode == M_CHRONO || mode == M_TALLY || mode == M_DIE || mode == M_CHESS) ? 100UL : 200UL;
  bool draw = (now - lastDraw >= period) || (e != EV_NONE) || dial;
  if (draw) lastDraw = now;

  if (ringing) { handleRing(e, draw); return; }

  switch (mode) {
    case M_PARAMS:   drawParams(e, draw);   break;
    case M_HOME:     drawHome(e, draw);     break;
    case M_CLOCK:    drawClockPage(e, draw); break;
    case M_TH:       drawTempHum(e, draw);  break;
    case M_ALARM:    drawAlarm(e, draw);    break;
    case M_POMO:     drawPomo(e, draw);     break;
    case M_CHRONO:   drawChrono(e, draw);   break;
    case M_TALLY:    drawTally(e, draw, tap); break;
    case M_DIE:      drawDie(e, draw, tap);   break;
    case M_CHESS:    drawChess(e, draw);      break;
    case M_MUSIC:    drawMusic(e, draw);      break;
    case M_SETCLOCK: drawSetClock(e, draw); break;
    case M_BRIGHT:   drawBright(e, draw);   break;
    case M_MENU:     drawMenu(e, draw);     break;
  }
}
