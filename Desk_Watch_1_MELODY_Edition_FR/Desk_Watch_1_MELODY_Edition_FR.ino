// Desk Watch 1 — MELODY Edition : température et humidité / minuteur Pomodoro / chronomètre / compteur / dé / pendule d’échecs / réveil / horloge
// Matériel : Arduino UNO + Grove Base Shield, écran LCD RGB V4.0 (I2C),
//            SHT31 (I2C 0x44), buzzer D3, bouton D4, potentiomètre (molette) A1,
//            module RTC DS1307 ou DS3231 en option (I2C 0x68).
// Bibliothèque requise : « Grove - LCD RGB Backlight » (rgb_lcd.h).
// Le SHT31 et le RTC sont lus directement en I2C : aucune autre bibliothèque nécessaire.
//
// Les fonctions restent actives en arrière-plan : humidité, réveil, Pomodoro et chronomètre.
//
// Navigation :
//   Appui long (partout) : ouvrir le MENU ; dans le MENU, revenir à la page précédente.
//   Accueil : heure RTC et mesures sur un même écran ; affichage stable si tout est OK, alternance uniquement en cas d’alerte ; appui court = MENU.
//   Menu : Favori, Temp et Hum, Minuteur, Compteur, Chrono, Réveil, Dé, Pendule d’échecs, Mélodie, Paramètres.
//   Paramètres : luminosité, volume, son de démarrage, veille automatique, mode nuit, réglage de l’heure, favori.
//   Mode nuit : Non / Auto (22 h à 6 h, avec RTC) / Oui : luminosité réduite à 8 %, bips d’alerte et mélodie de démarrage coupés ; le réveil sonne normalement.
//   Molette : tourner à droite = valeur ou entrée suivante.
//   Paramètres : molette = choisir ; appui court = ouvrir ou confirmer.
//   Démarrage : DESK WATCH, courte mélodie, PR BELAMINE MAKERLAB, barre de chargement MELODY Edition, vérification des modules.
//   MENU : molette = parcourir ; appui court = entrer.
//
// Fonctions :
//   Temp et Hum : ligne 1 = heure RTC et indicateurs des fonctions en arrière-plan, toujours visibles ; ligne 2 = température, humidité et OK.
//                 En alerte, la ligne 2 alterne toutes les 2 s : mesures / alerte température / mesures / alerte humidité.
//                 Le rétroéclairage RGB colore tout l’écran selon l’alerte affichée.
//                 Trop haut = rouge ; trop bas = bleu ; OK = vert. Double clic = régler les quatre seuils
//                 (température basse, température haute, humidité basse, humidité haute) à la molette ; appui court = suivant puis valider.
//                 Les seuils sont enregistrés et réutilisés au démarrage.
//   Compteur : chaque appui court ajoute 1 immédiatement ; molette = demander la remise à zéro (OK = oui, double clic ou délai de 5 s = non).
//   Dé : appui court = lancer (cliquetis pendant 1 s, puis résultat) ; molette = nombre de faces (2, 4, 6, 8, 10, 12, 20, 30, 100) ; double clic = menu.
//   Mélodie : molette = choisir ; appui court = lire, mettre en pause ou reprendre ; double clic = menu.
//   Pendule d’échecs : molette = cadence (1+0 à 60+0, 5+3, etc.) ; appui court = démarrer puis changer de joueur ; double clic = pause, remise à zéro ou menu.
//   Réveil : appui court = régler heures, minutes, activation, sonnerie et volume ; double clic = activer ou désactiver.
//            La molette choisit la valeur ; un clic avance et enregistre après le volume. La sonnerie se répète jusqu’à l’arrêt ou au report.
//            Les réglages de sonnerie et de volume proposent une préécoute, qui s’arrête en quittant le réglage.
//            Choix de sonnerie : préécoute au volume minimum. Réglage du volume : préécoute au niveau choisi (Faible, Moyen, Fort).
//            Huit sonneries : les quatre mélodies de l’app et quatre créations pour le réveil : Aurora, Cosmic Pulse, Arcade Hero et Epic Rise.
//   Pomodoro : appui court = démarrer ou mettre en pause ; double clic = remettre à zéro (puis, si déjà à zéro, revenir au menu).
//              À l’arrêt, le potentiomètre règle la durée de travail, de 1 min à 4 h.
//   Chronomètre : appui court = démarrer ou arrêter ; double clic = remettre à zéro ; si déjà à zéro, double clic ou molette = menu.
//   Réglage de l’heure : potentiomètre = valeur ; appui court = suivant puis confirmer ; appui long = annuler.
//   Luminosité : potentiomètre = niveau (5 à 100 %) ; appui court = confirmer ; appui long = annuler.
//   Pendant la sonnerie : appui court ou double clic = reporter de 5 min ; appui long = arrêter.
//   Appui très long (plus de 2,5 s) : mettre en veille (écran éteint, réveil actif) ; un appui réveille l’appareil.
//   Veille automatique : délai réglable (jamais, 1, 2, 5, 10, 15, 30 min, 1 h) depuis le dernier appui ou mouvement de molette.
//                         La veille automatique est désactivée pendant une partie d’échecs en cours.
//   LED de l’UNO (broche 13) : allumée en veille et pendant un appui dépassant 2,5 s.
//   Un appui court est confirmé 0,3 s après le relâchement, pour permettre le double clic.
//   Indicateurs en haut à droite : ! = alerte température/humidité ; A = réveil activé ; C = chronomètre en cours.
//   Minuteur en cours : M sur toutes les pages ; dernière colonne, masquée 1 s toutes les 10 s.

#include <Wire.h>
#include <EEPROM.h>
#include <rgb_lcd.h>

// ---------- Réglages ----------
const uint8_t PIN_BUZZER = 3;
const uint8_t PIN_BUTTON = 4;
const uint8_t PIN_LED    = 13;         // LED L de l’UNO ; pour une LED rouge externe, brancher un module Grove LED et adapter cette broche.
const unsigned long LED_HOLD_MS = 2500;   // Durée d’appui qui allume la LED avant le passage en veille.
const uint8_t PIN_POT    = A1;
const uint8_t BTN_ACTIVE = HIGH;          // Module Grove : HIGH signifie que le bouton est appuyé.
const int  POT_MAX   = 1023;              // Utiliser 675 si le sélecteur du shield est positionné sur 3,3 V.

// Seuils d’alerte : valeur >= seuil haut = trop haut ; valeur <= seuil bas = trop bas, comme sur la station V6.
// Valeurs par défaut, réglables par double clic sur Temp et Hum et enregistrées en mémoire.
const uint8_t T_LO_DEFAULT2 = 40, T_HI_DEFAULT2 = 56;   // Température en demi-degrés : 20,0 et 28,0 °C.
const uint8_t H_LO_DEFAULT  = 40, H_HI_DEFAULT  = 60;   // Humidité en pourcentage.
const uint8_t T_MIN2 = 20, T_MAX2 = 80, T_GAP2 = 2;     // Plage de 10 à 40 °C, par pas de 0,5 °C ; écart minimal de 1 °C.
const uint8_t H_MIN  = 10, H_MAX  = 90, H_GAP  = 5;     // Plage de 10 à 90 % ; écart minimal de 5 points.
const int T_MARGE10 = 3, H_MARGE10 = 20;                 // Marge anti-oscillation autour des seuils, en dixièmes : 0,3 °C et 2 %.
const int TEMP_OFFSET10 = 0, HUM_OFFSET10 = 0;           // Correction en dixièmes après comparaison avec un instrument de référence.

const unsigned long SNOOZE_MS  = 5UL * 60000UL;
const unsigned long RING_MAX_S = 120;     // Durée maximale de sonnerie avant report automatique.
const uint8_t NIGHT_START = 22, NIGHT_END = 6;   // Plage horaire du rétroéclairage atténué.

const uint8_t SHORT_BREAK_MIN = 5, LONG_BREAK_MIN = 15, LONG_EVERY = 4;
// Durées de travail : de 1 à 60 min par pas de 1 min, puis de 65 min à 4 h par pas de 5 min.
const uint8_t N_WORK = 96;
uint8_t workValue(uint8_t i) { return i < 60 ? i + 1 : 60 + (i - 59) * 5; }

const uint8_t SHT_ADDR = 0x44, RTC_ADDR = 0x68;

const uint8_t BRIGHT_LEVELS[8] = { 5, 10, 20, 30, 50, 70, 85, 100 };   // Niveaux de rétroéclairage, en pourcentage.

// ---------- États ----------
rgb_lcd lcd;

enum { M_TH, M_POMO, M_CHRONO, M_TALLY, M_DIE, M_CHESS, M_MUSIC, M_PARAMS, M_HOME, M_ALARM, M_SETCLOCK, M_BRIGHT, M_MENU, M_CLOCK };
const uint8_t N_MENU = 10;
const uint8_t N_PAR = 7;   // visible settings (the language choice was removed: one language per edition)
uint8_t mode = M_HOME, prevMode = M_HOME;   // Valeurs de l’énumération M_*.
uint8_t menuIdx = 0;
uint8_t brightBackup = 100, brightIdx = 7;

enum { EV_NONE, EV_SHORT, EV_LONG, EV_VLONG, EV_DOUBLE, EV_PRESS };

struct Settings { uint8_t magic, aH, aM, armed, workMin, bright, tLo2, tHi2, hLo, hHi, sound, lang, volume, home, night, sleep; } cfg;   // lang : 0 français, 1 anglais, 2 espagnol, 3 allemand ; home : favori (0 auto, 1 Temp et Hum, 2 horloge, 3 minuteur, 4 chrono, 5 compteur) ; night : mode nuit (0 non, 1 automatique avec RTC, 2 oui) ; sleep : indice du délai dans SLEEP_MIN (0 = jamais).

// Textes de cette édition (une seule langue par version : le nom du fichier l’indique), conservés en mémoire flash.
#define TR(fr, en, es, de) PSTR(fr)

// Horloge.
bool rtcPresent = false;
unsigned long swSecs = 0, swRefMs = 0;
uint8_t nowH = 0, nowM = 0, nowS = 0;

// Capteur.
int temp10 = 0, hum10 = 0;      // Température en dixièmes de °C et humidité en dixièmes de % ; calculs entiers pour économiser la mémoire.
bool shtOk = false;
uint8_t tState = 0, hState = 0;   // 0 = OK, 1 = trop haut, 2 = trop bas.

// Réveil.
uint8_t clockSub = 0;       // Réglage de l’heure : 1 = heures, 2 = minutes.
uint8_t alarmSub = 0;       // Réveil : 0 = affichage, 1 = heures, 2 = minutes, 3 = activation, 4 = sonnerie, 5 = volume.
// Extension EEPROM distincte pour préserver les réglages existants et leurs adresses.
struct AlarmSettings { uint8_t magic, melody, volume; } alarmCfg;
uint8_t alarmPos = 0;
unsigned long alarmNoteEnd = 0;
bool alarmPreview = false;
uint8_t previewMelody = 0, previewVolume = 0;
uint8_t parSub = 0, parBackup = 0;   // Réglage ouvert et valeur sauvegardée avant modification.
uint8_t parIdx = 1;          // Page Paramètres (identifiants internes) : 1 luminosité, 2 volume, 3 son de démarrage, 4 veille automatique, 5 mode nuit, 6 heure, 7 favori
unsigned int tallyCount = 0;  // Compteur manuel.
uint8_t tallySub = 0;         // 0 = comptage, 1 = confirmation de remise à zéro.
unsigned long tallyT0 = 0;
unsigned long tapMs = 0;      // Instant du dernier relâchement après un appui court.
bool tapFlag = false;         // Un appui court vient d’être relâché ; comptage immédiat sans attendre le délai du double clic.
// Faces possibles du dé : 2 = pile ou face ; 30 = tirer un élève de la classe au sort.
const uint8_t N_DIE = 9;
static const uint8_t DIE_FACES[N_DIE] PROGMEM = { 2, 4, 6, 8, 10, 12, 20, 30, 100 };
uint8_t dieIdx = 2, dieVal = 0, dieShow = 0;   // Indice du dé choisi (D6 au démarrage), dernier résultat et valeur affichée pendant le lancer.
bool dieRolling = false;
unsigned long dieEnd = 0;
// Cadences d’échecs : durée en minutes et incrément en secondes.
const uint8_t N_CHESS = 10;
static const uint8_t CHESS_MIN[N_CHESS] PROGMEM = { 1, 3, 3, 5, 5, 10, 10, 15, 30, 60 };
static const uint8_t CHESS_INC[N_CHESS] PROGMEM = { 0, 0, 2, 0, 3, 0, 5, 10, 0, 0 };
uint8_t chessIdx = 4;                 // Cadence initiale : 5 min + 3 s par coup.
uint8_t chessState = 0;               // 0 = attente, 1 = partie en cours, 2 = pause, 3 = temps écoulé.
uint8_t chessSide = 0;                // Joueur dont le temps s’écoule : 0 ou 1.
long chessBase[2];                    // Temps restant de chaque joueur au début de son tour, en millisecondes.
unsigned long chessTurn = 0;          // Instant du début du tour.
long chessLeft(uint8_t sd) {
  long v = chessBase[sd];
  if (chessState == 1 && sd == chessSide) v -= (long)(millis() - chessTurn);
  return v < 0 ? 0 : v;
}
// Notes en hertz et durées par unités de 10 ms, conservées en mémoire flash.
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
static const char MUSIC_NAMES[1][N_MUSIC][13] PROGMEM = {
  { "Anniversaire", "Pour Elise", "Mario Bros", "Tetris" } };
// Créations réservées au réveil : fréquences en hertz et durées par unités de 10 ms ; zéro indique un silence.
// Aurora : arpèges majeurs fluides, puis résolution lumineuse dans les aigus.
static const uint16_t AURORA_NOTE[] PROGMEM = {
  523, 659, 784, 1047, 988, 784, 659, 0,
  587, 740, 880, 1175, 1047, 880, 740, 0,
  659, 784, 1047, 1319, 1175, 988, 1047, 0 };
static const uint8_t AURORA_LEN[] PROGMEM = {
  18, 18, 18, 36, 18, 18, 36, 12,
  18, 18, 18, 36, 18, 18, 36, 12,
  18, 18, 24, 36, 18, 18, 72, 24 };
// Cosmic Pulse : pulsations mineures syncopées et réponse en cascade.
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
// Arcade Hero : motifs courts et entraînants, sauts d’octave et finale de victoire.
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
// Epic Rise : fanfare pointée, progression ascendante et dernière note aiguë tenue.
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

PGM_P alarmTuneName() {
  return alarmCfg.melody < N_MUSIC ? MUSIC_NAMES[0][alarmCfg.melody]
                                 : ALARM_EXTRA_NAMES[alarmCfg.melody - N_MUSIC];
}

uint8_t musicTrack = 0;
uint8_t musicState = 0, musicPos = 0;  // 0 = prêt, 1 = lecture, 2 = pause ; indice de la prochaine note à jouer.
int musicRemain = 0;                  // Durée restante de la note courante, en millisecondes.
unsigned int musicFreq = 0;
unsigned long musicNoteEnd = 0;
uint8_t thSub = 0;          // Seuils : 0 = affichage, 1 = température basse, 2 = température haute, 3 = humidité basse, 4 = humidité haute.
uint8_t edH = 0, edM = 0;
bool ringing = false, snoozing = false;
bool standby = false;       // Veille : écran éteint, réveil toujours actif.
static const uint8_t SLEEP_MIN[8] PROGMEM = { 0, 1, 2, 5, 10, 15, 30, 60 };   // Délais de veille automatique en minutes ; 0 = jamais.
unsigned long lastActivity = 0;   // Dernier appui ou mouvement de la molette.
int lastPotSample = 0;
unsigned long lastPotCheck = 0;
unsigned long btnHeld = 0;        // Durée de l’appui en cours, en millisecondes ; zéro si le bouton est relâché.
unsigned long ringStart = 0, snoozeUntil = 0;
int lastFiredMin = -1;

// Minuteur Pomodoro.
uint8_t pomoPhase = 0;      // 0 = travail, 1 = pause courte, 2 = pause longue.
uint8_t pomoCount = 0;
bool pomoRun = false;
unsigned long pomoRemMs = 0, pomoLast = 0;

// Chronomètre.
bool chronoRun = false;
unsigned long chronoStart = 0, chronoAccum = 0;
unsigned long chronoMs() { return chronoRun ? millis() - chronoStart : chronoAccum; }

// Potentiomètre et affichage.
int potAnchor = 0; bool potActive = false;
uint8_t lastR = 255, lastG = 255, lastB = 255;
unsigned long lastDraw = 0, lastClockRead = 0, lastSht = 0;

// ---------- Fonctions utilitaires ----------
uint8_t bcd2dec(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
uint8_t dec2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

// Lecture lissée : moyenne de huit mesures ; la première, perturbée par le multiplexeur, est ignorée.
int potRead() {
  analogRead(PIN_POT);
  long sum = 0;
  for (uint8_t i = 0; i < 8; i++) sum += analogRead(PIN_POT);
  int v = (int)(sum / 8);
  if (v > POT_MAX) v = POT_MAX;
  return POT_MAX - v;                              // Sens inversé : tourner à droite augmente la valeur.
}
// Choix parmi n valeurs à la molette, avec hystérésis aux limites des plages.
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
// Affichage « 25 min » jusqu’à 60 minutes, puis « 1h05 ».
void fmtDuration(char* b, size_t n, unsigned m) {
  if (m <= 60) snprintf_P(b, n, PSTR("%u min"), m);
  else         snprintf_P(b, n, PSTR("%uh%02u"), m / 60, m % 60);
}
void potReset() { potAnchor = potRead(); potActive = false; }
bool potLive() {
  int d = potRead() - potAnchor;
  if (d < 0) d = -d;
  if (!potActive && d > 30) potActive = true;
  return potActive;
}

// Mode nuit actif si « Oui » est choisi ou, en mode automatique, entre NIGHT_START et NIGHT_END avec le RTC.
bool nightActive() {
  if (cfg.night == 2) return true;
  return cfg.night == 1 && rtcPresent && (nowH >= NIGHT_START || nowH < NIGHT_END);
}
bool nightPrev = false;

// Applique la couleur demandée selon la luminosité choisie ; atténue la nuit, sauf pendant la sonnerie.
void applyColor() {
  uint8_t k = (nightActive() && !ringing) ? 8 : 100;
  nightPrev = nightActive();
  lcd.setRGB((uint32_t)lastR * cfg.bright * k / 10000,
             (uint32_t)lastG * cfg.bright * k / 10000,
             (uint32_t)lastB * cfg.bright * k / 10000);
}

void color(uint8_t r, uint8_t g, uint8_t b) {
  if (r == lastR && g == lastG && b == lastB) return;
  lastR = r; lastG = g; lastB = b;
  applyColor();
}

void line(uint8_t row, const char* s) {
  char b[17];
  snprintf_P(b, sizeof(b), PSTR("%-16s"), s);
  lcd.setCursor(0, row);
  lcd.print(b);
}

// Texte en mémoire flash pour économiser la RAM de l’Arduino UNO.
void lineP(uint8_t row, PGM_P s) {
  char b[17];
  strncpy_P(b, s, 16); b[16] = 0;
  line(row, b);
}

// Les textes flash séparés par des zéros évitent le remplissage des tableaux à largeur fixe.
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
  static const uint8_t DUTY[3] = { 5, 20, 50 };
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

// Lecture commune au réveil et à sa préécoute, sans attente bloquante pour le bouton ni la molette.
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
  if (alarmSub != 4 && alarmSub != 5) { stopAlarmPreview(); return; }
  uint8_t volume = alarmSub == 4 ? 0 : alarmCfg.volume;
  if (!alarmPreview || previewMelody != alarmCfg.melody || previewVolume != volume) {
    musicToneStop();
    alarmPos = 0; alarmNoteEnd = millis();
    previewMelody = alarmCfg.melody; previewVolume = volume;
    alarmPreview = true;
  }
  alarmMelodyTick(millis(), volume);
}

// Buzzer : signal rectangulaire à la fréquence demandée ; le rapport cyclique règle le volume.
// Rapports de 5 %, 20 % ou 50 % : une impulsion plus courte atténue le buzzer piézoélectrique. Fonction bloquante pendant ms millisecondes.
void buzz(unsigned int freq, unsigned int ms) {
  if (alarmPreview) musicToneStop();   // Laisse temporairement la broche aux bips de notification.
  static const uint8_t DUTY[3] = { 5, 20, 50 };
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

const unsigned long DOUBLE_MS = 300;       // Délai maximal entre deux appuis pour reconnaître un double clic.

uint8_t readButton() {
  static bool down = false, pending = false, second = false, musicPressHandled = false;
  static unsigned long t0 = 0, tRel = 0;
  bool p = (digitalRead(PIN_BUTTON) == BTN_ACTIVE);
  unsigned long now = millis();
  btnHeld = (p && down) ? now - t0 : 0;

  // Un appui court isolé est confirmé lorsque le délai du double clic est écoulé.
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
    return d < 2500 ? EV_LONG : EV_VLONG;
  }
  return EV_NONE;
}

// ---------- Configuration EEPROM ----------
void loadCfg() {
  EEPROM.get(0, cfg);
  if (cfg.magic != 0xA9 || cfg.aH > 23 || cfg.aM > 59 || cfg.armed > 1 ||
      cfg.workMin < 1 || cfg.workMin > 240) {
    cfg.magic = 0xA9; cfg.aH = 7; cfg.aM = 0; cfg.armed = 0; cfg.workMin = 25;
    cfg.bright = 100; cfg.sound = 1; cfg.volume = 2; cfg.home = 0; cfg.night = 1; cfg.sleep = 0; cfg.tLo2 = 0xFF;       // Les seuils sont vérifiés plus bas.
  }
  if (cfg.sleep > 7) cfg.sleep = 0;                     // Veille automatique désactivée par défaut.
  if (cfg.night > 2) cfg.night = 1;                     // Mode nuit automatique par défaut.
  if (cfg.home > 5) cfg.home = 0;                       // Favori automatique par défaut.
  if (cfg.volume > 2) cfg.volume = 2;                   // Volume maximal par défaut.
  if (cfg.sound > 1) cfg.sound = 1;                    // Son de démarrage activé par défaut.
  // Rétablit les seuils par défaut s’ils sont absents ou incohérents.
  if (cfg.tLo2 < T_MIN2 || cfg.tHi2 > T_MAX2 || cfg.tHi2 < cfg.tLo2 + T_GAP2 ||
      cfg.hLo < H_MIN || cfg.hHi > H_MAX || cfg.hHi < cfg.hLo + H_GAP) {
    cfg.tLo2 = T_LO_DEFAULT2; cfg.tHi2 = T_HI_DEFAULT2;
    cfg.hLo = H_LO_DEFAULT;   cfg.hHi = H_HI_DEFAULT;
  }
  if (cfg.bright < 1 || cfg.bright > 100) cfg.bright = 100;   // Ancienne sauvegarde ou valeur incorrecte.
  EEPROM.get(sizeof(cfg), alarmCfg);
  if (alarmCfg.magic != 0xB1 || alarmCfg.melody >= N_ALARM_TUNES || alarmCfg.volume > 2) {
    alarmCfg.magic = 0xB1; alarmCfg.melody = 0; alarmCfg.volume = cfg.volume;
  }
}
void saveCfg() { EEPROM.put(0, cfg); EEPROM.put(sizeof(cfg), alarmCfg); }

// ---------- Horloge ----------
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
    Wire.write((uint8_t)0);          // Secondes à zéro ; efface aussi le bit CH du DS1307.
    Wire.write(dec2bcd(m));
    Wire.write(dec2bcd(h));          // Format 24 heures.
    Wire.endTransmission();
  } else {
    swSecs = (unsigned long)h * 3600UL + (unsigned long)m * 60UL;
    swRefMs = millis();
  }
  readClock();
}

// ---------- Capteur SHT31 ----------
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

bool readSht() {
  Wire.beginTransmission(SHT_ADDR);
  Wire.write((uint8_t)0x24); Wire.write((uint8_t)0x00);   // Mesure avec répétabilité élevée.
  if (Wire.endTransmission() != 0) return false;
  delay(20);
  if (Wire.requestFrom(SHT_ADDR, (uint8_t)6) != 6) return false;
  uint8_t b[6];
  for (uint8_t i = 0; i < 6; i++) b[i] = Wire.read();
  static uint8_t crcBad = 0;
  if (crc8(b, 2) != b[2] || crc8(b + 3, 2) != b[5]) {   // corrupted read: keep the previous values
    return ++crcBad < 3 ? shtOk : false;                // report a sensor fault after 3 bad reads in a row
  }
  crcBad = 0;
  uint16_t rt = ((uint16_t)b[0] << 8) | b[1];
  uint16_t rh = ((uint16_t)b[3] << 8) | b[4];
  temp10 = (int)((1750L * rt) / 65535L) - 450 + TEMP_OFFSET10;
  hum10  = (int)((1000L * rh) / 65535L) + HUM_OFFSET10;
  if (hum10 < 0) hum10 = 0;
  if (hum10 > 1000) hum10 = 1000;
  return true;
}

// Actualise l’état avec une marge ; renvoie vrai lors d’un nouveau dépassement haut ou bas.
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
  if (ringing || standby || nightActive()) return;   // Silence pendant la sonnerie, en veille et la nuit.
  if (st == 1) beep(2, 120); else beep(1, 700);   // Seuil haut : deux bips courts ; seuil bas : un bip long.
}
void updateAlerts() {
  if (updateState(tState, temp10, cfg.tLo2 * 5, cfg.tHi2 * 5, T_MARGE10))   alertBeep(tState);
  if (updateState(hState, hum10, cfg.hLo * 10, cfg.hHi * 10, H_MARGE10))   alertBeep(hState);
}
bool anyAlert() { return tState != 0 || hState != 0; }

// ---------- Réveil ----------
void startRing() {
  if (alarmPreview) { stopAlarmPreview(); alarmSub = 0; }   // Le vrai réveil est prioritaire sur la préécoute.
  if (musicState == 1) {
    long remaining = (long)(musicNoteEnd - millis());
    musicRemain = remaining > 0 ? remaining : 0;
    musicState = 2;                     // La lecture musicale pourra être reprise manuellement après le réveil.
  }
  musicToneStop();
  alarmPos = 0; alarmNoteEnd = millis();
  if (standby) { standby = false; lcd.clear(); }   // Le réveil rallume l’écran.
  ringing = true; snoozing = false;
  ringStart = millis();
  lastR = 255;                          // Force l’actualisation de la couleur.
}
void stopRing() {
  ringing = false; snoozing = false;
  lastR = 255;
  musicToneStop();
}

void checkAlarm() {
  if (ringing || !rtcPresent) return;   // Pas de réveil sans horloge fiable.
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
    lineP(1, TR("OK=snooze 5 min", "OK=snooze 5 min", "OK=repetir 5 min", "OK=Snooze 5 min"));
  }
}

// ---------- Minuteur Pomodoro ----------
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

// ---------- Écrans ----------
// ---------- Navigation ----------
// Lecture et modification des valeurs des paramètres selon leur indice.
uint8_t parValue(uint8_t i) { return i == 2 ? cfg.volume : i == 3 ? cfg.sound : i == 4 ? cfg.sleep : i == 5 ? cfg.night : cfg.home; }
void setParValue(uint8_t i, uint8_t v) { if (i == 2) cfg.volume = v; else if (i == 3) cfg.sound = v; else if (i == 4) cfg.sleep = v; else if (i == 5) cfg.night = v; else cfg.home = v; }

void chessPause() {                    // Quitter la page met la pendule d’échecs en pause.
  if (chessState == 1) { chessBase[chessSide] -= (long)(millis() - chessTurn); chessState = 2; }
}

void enterMode(uint8_t m) {
  stopAlarmPreview();
  if (mode == M_CHESS && m != M_CHESS) chessPause();
  if (mode == M_MUSIC && m != M_MUSIC) musicToneStop();
  mode = m;
  clockSub = 0; alarmSub = 0; thSub = 0; parIdx = 1; parSub = 0; tallySub = 0;
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

// Accueil : ouvre la page choisie dans les paramètres ; 0 = automatique.
void goHome() {
  uint8_t h = cfg.home;
  if (h == 2 && !rtcPresent) h = 1;                  // Sans RTC, le favori Horloge est remplacé par Temp et Hum.
  enterMode(h == 0 ? M_HOME : h == 1 ? M_TH : h == 2 ? M_CLOCK : h == 3 ? M_POMO : h == 4 ? M_CHRONO : M_TALLY);
}

// Revient aux paramètres sur l’entrée d’indice i.
void backToParams(uint8_t i) { enterMode(M_PARAMS); parIdx = i; }

void openMenu() {
  stopAlarmPreview();
  if (mode == M_MUSIC) musicToneStop();
  if (mode == M_CHESS) chessPause();
  prevMode = mode;
  int t = potRead() / ((POT_MAX + 1) / N_MENU);       // L’entrée du menu suit la position de la molette.
  menuIdx = (t > N_MENU - 1) ? N_MENU - 1 : t;
  mode = M_MENU;
  clockSub = 0; alarmSub = 0; thSub = 0; parSub = 0; tallySub = 0;
  potReset();
  lastDraw = 0;
}

void backToMenu() {
  if (mode == M_MENU) { enterMode(prevMode); return; }          // Dans le menu : retour à la page précédente.
  if (mode == M_BRIGHT) { cfg.bright = brightBackup; applyColor(); }   // Annule le changement de luminosité.
  if (mode == M_ALARM && alarmSub != 0) saveCfg();
  if (mode == M_TH && thSub != 0) saveCfg();
  if (mode == M_PARAMS && parSub != 0) setParValue(parIdx, parBackup);   // Annule le paramètre en cours de modification.
  openMenu();
}

// Entrée du menu choisie au potentiomètre, avec hystérésis pour stabiliser l’affichage.
uint8_t menuFromPot() {
  const int zone = (POT_MAX + 1) / N_MENU;
  int raw = potRead();
  int target = raw / zone;
  if (target > N_MENU - 1) target = N_MENU - 1;
  int cur = menuIdx;
  if (target > cur && raw >= target * zone + 15) return target;
  if (target < cur && raw <= cur * zone - 15) return target;
  return menuIdx;
}

// Lettres des fonctions actives en arrière-plan, sauf celle de la page affichée ; colonnes 13 à 15, numérotées depuis zéro.
void badges(uint8_t self) {
  char all[5];                                       // Ordre : ! A C M ; le minuteur reste le plus à droite.
  uint8_t n = 0;
  if (self != M_TH     && anyAlert())                all[n++] = '!';
  if (self != M_ALARM  && cfg.armed && rtcPresent)   all[n++] = 'A';
  if (self != M_CHRONO && chronoRun)                 all[n++] = 'C';
  if (self != M_POMO   && pomoRun)                   all[n++] = ((millis() / 1000UL) % 10 == 9) ? ' ' : 'M';   // M pour le minuteur partout ; masqué une seconde toutes les dix secondes.
  char s[4] = { ' ', ' ', ' ', 0 };                  // Colonnes 13 à 15, alignement à droite.
  uint8_t keep = n > 3 ? 3 : n;
  for (uint8_t i = 0; i < keep; i++) s[3 - keep + i] = all[n - keep + i];
  lcd.setCursor(13, 0);
  lcd.print(s);
}

// ---------- Pages ----------
uint8_t menuMode(uint8_t i);
void drawMenu(uint8_t e, bool draw) {
  static const char names[] PROGMEM =
  "Favori\0"
  "Temp et Hum\0"
  "Minuteur\0"
  "Compteur\0"
  "Chrono\0"
  "Reveil\0"
  "De\0"
  "Pend. echecs\0"
  "Melodie\0"
  "Parametres\0"
;
  menuIdx = menuFromPot();
  if (e == EV_SHORT) { if (menuIdx == 0) goHome(); else enterMode(menuMode(menuIdx)); return; }
  if (e == EV_LONG || e == EV_VLONG) { enterMode(prevMode); return; }
  if (!draw) return;
  char b[17];
  color(40, 80, 160);
  snprintf_P(b, sizeof(b), PSTR("MENU       %2d/%d"), menuIdx + 1, N_MENU);
  line(0, b);
  char nm[13]; strcpy_P(nm, packedText(names, menuIdx));
  snprintf_P(b, sizeof(b), PSTR("> %s"), nm);
  line(1, b);
}

uint8_t menuMode(uint8_t i) {
  static const uint8_t modes[N_MENU] PROGMEM = { M_HOME, M_TH, M_POMO, M_TALLY, M_CHRONO, M_ALARM, M_DIE, M_CHESS, M_MUSIC, M_PARAMS };
  return pgm_read_byte(&modes[i]);
}

// Texte compact : « T23.4C  H 45% ».
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
  color(40, 90, 160);                                 // Le mode nuit atténue cette couleur.
  snprintf_P(b, sizeof(b), PSTR("    %02d:%02d:%02d"), nowH, nowM, nowS);
  line(0, b);
  badges(M_CLOCK);
  thText(b, sizeof(b));
  line(1, b);
}

void introSound();
// Favoris possibles : 0 automatique, 1 Temp et Hum, 2 horloge, 3 minuteur, 4 chronomètre, 5 compteur.
static const char homeName[] PROGMEM =
  "Auto\0"
  "Temp et Hum\0"
  "Horloge\0"
  "Minuteur\0"
  "Chrono\0"
  "Compteur\0"
;
// Paramètres : luminosité, volume, son de démarrage, veille automatique, mode nuit, heure et favori.

void drawParams(uint8_t e, bool draw) {
  if (e == EV_DOUBLE) { backToMenu(); return; }          // Double clic : retour au menu et annulation du réglage ouvert.
  if (parSub == 0) {
    if (potLive()) parIdx = 1 + potSelect(N_PAR, parIdx - 1, 30);
    if (e == EV_SHORT) {
      if (parIdx == 1)      { enterMode(M_BRIGHT); return; }
      else if (parIdx == 6) { enterMode(M_SETCLOCK); return; }
      parSub = 1; parBackup = parValue(parIdx); potReset();
    }
  } else {
    if (potLive()) {
      uint8_t cur = parValue(parIdx);
      uint8_t count = parIdx == 2 ? 3 : parIdx == 3 ? 2 : parIdx == 4 ? 8 : parIdx == 5 ? 3 : 6;
      uint8_t v = potSelect(count, cur, parIdx == 2 ? 40 : 30);
      if (v != cur) { setParValue(parIdx, v); if (parIdx == 2) beep(1, 100); }
    }
    if (e == EV_SHORT) {
      saveCfg(); parSub = 0; potReset();
      if (parIdx == 3 && cfg.sound) introSound();
    }
  }
  if (!draw) return;
  char b[17], nm[14], val[14];
  color(40, 90, 160);
  strcpy_P(nm, parIdx == 1 ? TR("Luminosite", "Brightness", "Brillo", "Helligkeit")
             : parIdx == 2 ? TR("Volume", "Volume", "Volumen", "Lautstaerke")
             : parIdx == 3 ? TR("Son demarr.", "Boot sound", "Sonido inic.", "Startton")
             : parIdx == 4 ? TR("Veille auto", "Auto sleep", "Reposo auto", "Auto-Standby")
             : parIdx == 5 ? TR("Mode nuit", "Night mode", "Modo noche", "Nachtmodus")
             : parIdx == 6 ? TR("Regler heure", "Set time", "Ajustar hora", "Uhr stellen")
                           : TR("Favori", "Favorite", "Favorito", "Favorit"));
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
    snprintf_P(b, sizeof(b), parSub ? PSTR("> %-12s") : PSTR("%-12s OK"), val);
    line(1, b);
    return;
  }
  if (parIdx == 4) {
    uint8_t m = pgm_read_byte(&SLEEP_MIN[cfg.sleep]);
    if (m == 0)       strcpy_P(val, TR("Jamais", "Never", "Nunca", "Nie"));
    else if (m == 60) strcpy_P(val, PSTR("1 h"));
    else              snprintf_P(val, sizeof(val), PSTR("%u min"), m);
    snprintf_P(b, sizeof(b), parSub ? PSTR("> %-12s") : PSTR("%-12s OK"), val);
    line(1, b);
    return;
  }
  if (parIdx == 5) {
    strcpy_P(val, cfg.night == 0 ? TR("Non", "Off", "No", "Aus") : cfg.night == 1 ? TR("Auto 22-6h", "Auto 22-6h", "Auto 22-6h", "Auto 22-6h") : TR("Oui", "On", "Si", "An"));
    snprintf_P(b, sizeof(b), parSub ? PSTR("> %-12s") : PSTR("%-12s OK"), val);
    line(1, b);
    return;
  }
  if (parIdx == 3) strcpy_P(val, cfg.sound ? TR("OUI", "ON", "SI", "JA") : TR("NON", "OFF", "NO", "NEIN"));
  else {
    char bar[4] = "   ";
    for (uint8_t i = 0; i <= cfg.volume; i++) bar[i] = '#';                            // Trois dièses indiquent le volume fort.
    strcpy_P(val, volumeName(cfg.volume));
    snprintf_P(b, sizeof(b), parSub ? PSTR("> %-6s %-3s OK") : PSTR("%-6s %-3s  OK"), val, bar);
    line(1, b);
    return;
  }
  snprintf_P(b, sizeof(b), parSub ? PSTR("> %-9s   OK") : PSTR("%-9s OK=chg"), val);
  line(1, b);
}

void drawTempHum(uint8_t e, bool draw);
// L’accueil utilise la page combinant mesures et horloge, sans changement automatique de page.
void drawHome(uint8_t e, bool draw) {
  if (e == EV_DOUBLE) { enterMode(M_TH); thSub = 1; potReset(); return; }   // Ouvre le réglage des seuils.
  if (e != EV_NONE || potLive()) { openMenu(); return; }   // Bouton ou molette : ouvrir le menu.
  drawTempHum(EV_NONE, draw);
}

// Page Horloge seule, choisie comme favori : bouton ou molette = menu.
void drawClockPage(uint8_t e, bool draw) {
  if (e != EV_NONE || potLive()) { openMenu(); return; }
  drawClock(draw);
}

uint8_t* thPtr(uint8_t sub) { return sub == 1 ? &cfg.tLo2 : sub == 2 ? &cfg.tHi2 : sub == 3 ? &cfg.hLo : &cfg.hHi; }

// Règle le seuil sub à la molette tout en conservant l’écart minimal entre les seuils bas et haut.
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
  if (thSub == 0) {
    if (e == EV_DOUBLE)    { thSub = 1; potReset(); }     // Ouvre le réglage des quatre seuils.
    else if (e != EV_NONE) { openMenu(); return; }
  } else if (e == EV_SHORT) {
    if (thSub < 4) { thSub++; potReset(); }
    else           { thSub = 0; saveCfg(); potReset(); }
  }
  if (thSub != 0 && potLive()) thEdit(thSub);
  if (thSub == 0 && potLive()) { openMenu(); return; }   // Tourner la molette ouvre le menu.

  if (!draw) return;
  char b[17];
  if (thSub != 0) {
    static const char titles[1][4][17] PROGMEM = {
      { "Temp. trop basse", "Temp. trop haute", "Humid trop basse", "Humid trop haute" } };
    color(150, 100, 0);
    lineP(0, titles[0][thSub - 1]);
    uint8_t v = *thPtr(thSub);
    if (thSub <= 2) snprintf_P(b, sizeof(b), PSTR("T%s%d.%dC  OK"), thSub == 1 ? "<=" : ">=", v / 2, (v % 2) * 5);
    else            snprintf_P(b, sizeof(b), PSTR("H%s%d%%  OK"),   thSub == 3 ? "<=" : ">=", v);
    line(1, b);
    return;
  }
  // L’heure et les indicateurs restent visibles, même pendant une alerte du capteur.
  if (rtcPresent) snprintf_P(b, sizeof(b), PSTR("%02d:%02d:%02d"), nowH, nowM, nowS);
  else strcpy_P(b, PSTR("--:--:--"));
  line(0, b); badges(M_TH);
  if (!shtOk) {
    color(60, 60, 60);
    lineP(1, TR("Capteur absent", "Sensor missing", "Sensor ausente", "Sensor fehlt"));
    return;
  }

  // Deuxième ligne : mesures et message complet alternent ; les mesures précèdent chaque alerte.
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
  // Le symbole degré correspond au caractère LCD 0xDF ; les mesures occupent au plus treize colonnes.
  snprintf_P(b, sizeof(b), PSTR("T%s%d.%d%c H%d%%"), temp10 < 0 ? "-" : "",
             abs(temp10) / 10, abs(temp10) % 10, 0xDF, (hum10 + 5) / 10);
  line(1, b);
  if (!n) { lcd.setCursor(14, 1); lcd.print("OK"); }
}

void drawSetClock(uint8_t e, bool draw) {
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

void drawAlarm(uint8_t e, bool draw) {
  if (!rtcPresent) {
    if (e != EV_NONE || potLive()) { openMenu(); return; }
    if (draw) { color(60, 60, 120); lineP(0, TR("Alarme : il faut", "Alarm: needs the", "Alarma: requiere", "Wecker: braucht")); lineP(1, TR("le module RTC", "RTC module", "modulo RTC", "RTC-Modul")); }
    return;
  }
  if (alarmSub == 0) {
    if (e == EV_DOUBLE)     { cfg.armed = !cfg.armed; saveCfg(); }   // Active ou désactive le réveil.
    else if (e == EV_SHORT) { alarmSub = 1; potReset(); }
    else if (potLive())     { openMenu(); return; }
  } else if (e == EV_SHORT) {
    stopAlarmPreview();
    alarmSub++;
    potReset();
    if (alarmSub == 6) { alarmSub = 0; saveCfg(); }
  }

  if (alarmSub == 1 && potLive()) cfg.aH = potSelect(24, cfg.aH, 4);
  if (alarmSub == 2 && potLive()) cfg.aM = potSelect(60, cfg.aM, 4);
  if (alarmSub == 3 && potLive()) cfg.armed = potSelect(2, cfg.armed, 20);
  if (alarmSub == 4 && potLive()) alarmCfg.melody = potSelect(N_ALARM_TUNES, alarmCfg.melody, 15);
  if (alarmSub == 5 && potLive()) alarmCfg.volume = potSelect(3, alarmCfg.volume, 20);
  updateAlarmPreview();

  if (!draw) return;
  char b[17];
  if (alarmSub == 0) {
    color(150, 70, 0);
    snprintf_P(b, sizeof(b), TR("Alarme  %02d:%02d", "Alarm   %02d:%02d", "Alarma  %02d:%02d", "Wecker  %02d:%02d"), cfg.aH, cfg.aM);
    line(0, b);
    badges(M_ALARM);
    if (snoozing) lineP(1, TR("Snooze en cours", "Snoozing", "Repitiendo", "Schlummern"));
    else          lineP(1, cfg.armed ? TR("ON   OK=regler", "ON   OK=set", "ON   OK=ajustar", "AN   OK=stellen") : TR("OFF  OK=regler", "OFF  OK=set", "OFF  OK=ajustar", "AUS  OK=stellen"));
    return;
  }
  color(150, 100, 0);
  switch (alarmSub) {
    case 1:  lineP(0, TR("Alarme : heures", "Alarm: hours", "Alarma: horas", "Wecker: Stunden"));  snprintf_P(b, sizeof(b), PSTR("%02d h"), cfg.aH); break;
    case 2:  lineP(0, TR("Alarme : minutes", "Alarm: minutes", "Alarma: minutos", "Wecker: Minuten")); snprintf_P(b, sizeof(b), PSTR("%02d min"), cfg.aM); break;
    case 4:  lineP(0, TR("Alarme: melodie", "Alarm: melody", "Alarma: melodia", "Wecker: Melodie")); strcpy_P(b, alarmTuneName()); break;
    case 5:  lineP(0, TR("Alarme: volume", "Alarm: volume", "Alarma: volumen", "Wecker: Lautst."));
             strcpy_P(b, volumeName(alarmCfg.volume)); break;
    default: lineP(0, TR("Alarme armee ?", "Alarm armed?", "Alarma activa?", "Wecker aktiv?")); strcpy_P(b, cfg.armed ? TR("OUI", "YES", "SI", "JA") : TR("NON", "NO", "NO", "NEIN")); break;
  }
  line(1, b);
}

void drawPomo(uint8_t e, bool draw) {
  if (e == EV_DOUBLE && pomoIdle()) { openMenu(); return; }   // Si déjà à zéro : double clic pour revenir au menu.
  if (e == EV_DOUBLE) { pomoReset(); potReset(); }   // Remise à zéro ; sinon le minuteur continue en arrière-plan.
  else if (e == EV_SHORT) {
    if (pomoIdle()) saveCfg();                       // Mémorise la durée choisie.
    pomoRun = !pomoRun;
    pomoLast = millis();
    potReset();                                      // La molette est prise en compte à partir de cet instant.
  }

  // Après le démarrage, la molette ouvre le menu sans arrêter le décompte.
  if (!pomoIdle() && potLive()) { openMenu(); return; }
  if (pomoIdle() && potLive()) {                     // À l’arrêt, le potentiomètre règle la durée.
    uint8_t cur = workIndex(cfg.workMin);
    uint8_t idx = potSelect(N_WORK, cur, 3);
    if (idx != cur) { cfg.workMin = workValue(idx); pomoRemMs = phaseMs(0); }
  }

  if (!draw) return;
  char b[17];
  unsigned long rs = (pomoRemMs + 999UL) / 1000UL;
  if (pomoPhase == 0)      { color(220, 30, 0);  snprintf_P(b, sizeof(b), TR("MINUTEUR #%d", "TIMER    #%d", "TEMPORIZ. #%d", "TIMER    #%d"), pomoCount + 1); }
  else if (pomoPhase == 1) { color(0, 170, 50);  snprintf_P(b, sizeof(b), TR("PAUSE COURTE", "SHORT BREAK", "PAUSA CORTA", "KURZE PAUSE")); }
  else                     { color(0, 80, 220);  snprintf_P(b, sizeof(b), TR("PAUSE LONGUE", "LONG BREAK", "PAUSA LARGA", "LANGE PAUSE")); }
  line(0, b);
  badges(M_POMO);
  char t[12], d[12];
  if (pomoIdle()) {
    fmtDuration(d, sizeof(d), cfg.workMin);
    snprintf_P(b, sizeof(b), PSTR("%s  OK=start"), d);
  } else {
    if (rs >= 3600) snprintf_P(t, sizeof(t), PSTR("%d:%02d:%02d"), (int)(rs / 3600), (int)((rs / 60) % 60), (int)(rs % 60));
    else            snprintf_P(t, sizeof(t), PSTR("%02d:%02d"), (int)(rs / 60), (int)(rs % 60));
    snprintf_P(b, sizeof(b), pomoRun ? TR("%s en cours", "%s running", "%s en curso", "%s laeuft") : TR("%s pause", "%s paused", "%s pausa", "%s Pause"), t);
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
  if (potLive()) { openMenu(); return; }            // La molette ouvre le menu ; le chronomètre continue en arrière-plan.

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
  snprintf_P(b, sizeof(b), chronoRun ? TR("%s marche", "%s running", "%s marcha", "%s laeuft") : (ms == 0 ? PSTR("%s OK=start") : TR("%s arret", "%s stopped", "%s parado", "%s gestoppt")), t);
  line(1, b);
}

void drawTally(uint8_t e, bool draw, bool tap) {
  if (tallySub == 0) {
    if (tap && tallyCount < 9999) { tallyCount++; lastDraw = 0; }   // Chaque appui est compté immédiatement.
    if (potLive()) { tallySub = 1; tallyT0 = millis(); }            // Molette : demander la remise à zéro.
  } else {
    if (e == EV_DOUBLE || millis() - tallyT0 > 5000) { tallySub = 0; potReset(); }
    else if (e == EV_SHORT && millis() - tallyT0 > 600) { tallyCount = 0; tallySub = 0; potReset(); }
  }
  if (!draw) return;
  char b[17];
  if (tallySub != 0) {
    color(150, 100, 0);
    lineP(0, TR("Remise a zero ?", "Reset counter?", "Poner a cero?", "Zuruecksetzen?"));
    lineP(1, TR("OK=oui  dbl=non", "OK=yes  dbl=no", "OK=si   dbl=no", "OK=ja  dbl=nein"));
    return;
  }
  color(0, 130, 130);
  lineP(0, TR("COMPTEUR", "COUNTER", "CONTADOR", "ZAEHLER"));
  badges(M_TALLY);
  snprintf_P(b, sizeof(b), PSTR("%16u"), tallyCount);
  line(1, b);
  // Temps actif en bas à gauche ; le total reste à droite, sur quatre colonnes.
  // Si les deux tournent, alterne minuteur et chronomètre toutes les deux secondes.
  if (pomoRun || chronoRun) {
    bool timer = pomoRun && (!chronoRun || (millis() / 2000UL) % 2 == 0);
    unsigned long secs = timer ? (pomoRemMs + 999UL) / 1000UL : chronoMs() / 1000UL;
    char t[12];
    if (secs >= 3600UL)
      snprintf_P(t, sizeof(t), PSTR("%c%lu:%02lu:%02lu"), timer ? 'M' : 'C', secs / 3600UL, (secs / 60UL) % 60, secs % 60);
    else
      snprintf_P(t, sizeof(t), PSTR("%c%02lu:%02lu"), timer ? 'M' : 'C', secs / 60UL, secs % 60);
    lcd.setCursor(0, 1);
    lcd.print(t);
  }
}

void drawDie(uint8_t e, bool draw, bool tap) {
  if (e == EV_DOUBLE) { openMenu(); return; }          // Double clic : revenir au menu.
  uint8_t faces = pgm_read_byte(&DIE_FACES[dieIdx]);
  if (dieRolling) {
    if ((long)(millis() - dieEnd) >= 0) {              // Fin du lancer : résultat définitif.
      seedRng(micros() + analogRead(PIN_POT));
      dieVal = rollDie(faces);
      dieRolling = false;
      beep(1, 60);
      lastDraw = 0;
    }
  } else {
    if (potLive()) {                                   // La molette choisit le nombre de faces.
      uint8_t n = potSelect(N_DIE, dieIdx, 20);
      if (n != dieIdx) { dieIdx = n; dieVal = 0; faces = pgm_read_byte(&DIE_FACES[dieIdx]); }
    }
    if (tap) {                                         // Chaque appui lance immédiatement le dé.
      seedRng(micros() + analogRead(PIN_POT));
      dieRolling = true; dieEnd = millis() + 900; dieVal = 0;
    }
  }
  if (!draw) return;
  char b[17], h[12];
  color(170, 60, 130);
  lineP(0, TR("DE", "DICE", "DADO", "WUERFEL"));
  badges(M_DIE);
  if (dieRolling) {
    dieShow = rollDie(faces);
    buzz(1800, 5);                                     // Petit cliquetis pendant le lancer.
    snprintf_P(b, sizeof(b), PSTR("D%-3u        %3u"), faces, dieShow);
  } else if (dieVal != 0) {
    snprintf_P(b, sizeof(b), PSTR("D%-3u        %3u"), faces, dieVal);
  } else {
    strcpy_P(h, TR("OK=lancer", "OK=roll", "OK=tirar", "OK=wuerfeln"));
    snprintf_P(b, sizeof(b), PSTR("D%-3u  %s"), faces, h);
  }
  line(1, b);
}

// Musique : molette = choisir un morceau ; appui court = lire, mettre en pause ou reprendre ; double clic = menu.
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
  strcpy_P(title, MUSIC_NAMES[0][musicTrack]);
  snprintf_P(b, sizeof(b), PSTR("%u/%u %s"), musicTrack + 1, N_MUSIC, title);
  line(0, b);
  badges(M_MUSIC);
  if (musicState == 1)      lineP(1, TR("Lecture OK=pause", "Playing OK=pause", "Suena OK=pausa", "Spielt OK=Pause"));
  else if (musicState == 2) lineP(1, TR("Pause OK=lire", "Paused OK=play", "Pausa OK=tocar", "Pause OK=Start"));
  else                   lineP(1, TR("Tourner=choix OK", "Dial=choose OK", "Dial=gira OK", "Dreh=wahl OK"));
}

// Pendule d’échecs : un appui court passe au joueur suivant ; la molette choisit la cadence à l’arrêt.
void drawChess(uint8_t e, bool draw) {
  if (chessState == 0) {
    if (e == EV_DOUBLE) { openMenu(); return; }
    if (potLive()) chessIdx = potSelect(N_CHESS, chessIdx, 20);
    if (e == EV_SHORT) {                               // Démarrage : le premier joueur commence.
      long t = (long)pgm_read_byte(&CHESS_MIN[chessIdx]) * 60000L;
      chessBase[0] = t; chessBase[1] = t;
      chessSide = 0; chessState = 1; chessTurn = millis();
      beep(1, 60); lastR = 255;
    }
  } else if (chessState == 1) {
    if (e == EV_SHORT) {                               // Changement de joueur à l’instant de l’appui.
      unsigned long at = tapMs;
      if ((long)(at - chessTurn) < 0) at = chessTurn;
      chessBase[chessSide] += (long)pgm_read_byte(&CHESS_INC[chessIdx]) * 1000L - (long)(at - chessTurn);
      chessSide ^= 1; chessTurn = at;
      buzz(2000, 10);
    } else if (e == EV_DOUBLE) chessPause();
    if (chessState == 1 && chessLeft(chessSide) == 0) {   // Le temps du joueur est écoulé.
      chessBase[chessSide] = 0; chessState = 3;
      beep(3, 200); lastR = 255;
    }
  } else if (chessState == 2) {
    if (e == EV_SHORT)       { chessState = 1; chessTurn = millis(); lastR = 255; }   // Reprise de la partie.
    else if (e == EV_DOUBLE) { chessState = 0; lastR = 255; }                          // Remise à zéro.
  } else {
    if (e == EV_SHORT || e == EV_DOUBLE) { chessState = 0; lastR = 255; }
  }
  if (!draw) return;
  char b[17], tag[10];
  if (chessState == 0) {
    color(40, 90, 160);
    lineP(0, TR("PEND. ECHECS", "CHESS CLOCK", "AJEDREZ", "SCHACHUHR"));
    badges(M_CHESS);
    strcpy_P(tag, TR("OK=start", "OK=start", "OK=inicio", "OK=Start"));
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
    if (chessState == 3 && sd == chessSide) strcpy_P(tag, TR("FIN", "END", "FIN", "ENDE"));
    else if (chessState == 2 && sd == chessSide) strcpy_P(tag, TR("PAUSE", "PAUSE", "PAUSA", "PAUSE"));
    snprintf_P(b, sizeof(b), PSTR("%c%d  %02d:%02d  %s"), (chessState != 3 && sd == chessSide) ? '>' : ' ', sd + 1, (int)(sec / 60), (int)(sec % 60), tag);
    line(sd, b);
  }
}

void drawBright(uint8_t e, bool draw) {
  if (e == EV_SHORT) { saveCfg(); backToParams(1); return; }

  if (potLive()) {
    brightIdx = potSelect(8, brightIdx, 10);
    cfg.bright = BRIGHT_LEVELS[brightIdx];
    applyColor();                                    // Prévisualisation immédiate.
  }

  if (!draw) return;
  char bar[9], b[17];
  for (uint8_t i = 0; i < 8; i++) bar[i] = (i <= brightIdx) ? '#' : '-';
  bar[8] = 0;
  color(40, 90, 160);
  lineP(0, TR("Luminosite", "Brightness", "Brillo", "Helligkeit"));
  snprintf_P(b, sizeof(b), PSTR("%3d%% %s"), cfg.bright, bar);
  line(1, b);
}

// ---------- Séquence de démarrage ----------
const char BRAND1[] PROGMEM = "PR BELAMINE";     // Deuxième écran de démarrage, première ligne.
const char BRAND2[] PROGMEM = "MAKERLAB";        // Deuxième écran de démarrage, deuxième ligne.

void centered(uint8_t row, const char* t) {
  char b[17];
  uint8_t len = strlen(t);
  if (len > 16) len = 16;
  uint8_t pad = (16 - len) / 2;
  memset(b, ' ', 16);                            // Centrage manuel : le format « %*s » n’est pas disponible sur l’UNO.
  memcpy(b + pad, t, len);
  b[16] = 0;
  line(row, b);
}
void centeredP(uint8_t row, PGM_P s) {
  char t[17];
  strncpy_P(t, s, 16); t[16] = 0;
  centered(row, t);
}
void introSound() {                              // Courte mélodie de trois notes.
  if (!cfg.sound || nightActive()) return;
  buzz(1500, 90); delay(40);
  buzz(2000, 90); delay(40);
  buzz(2600, 160);
  digitalWrite(PIN_BUZZER, LOW);
}
void intro() {
  lastR = 40; lastG = 90; lastB = 160;
  for (uint8_t i = 0; i <= 100; i += 5) {        // Allumage progressif du rétroéclairage.
    lcd.setRGB(40 * i / 100 * cfg.bright / 100, 90 * i / 100 * cfg.bright / 100, 160 * i / 100 * cfg.bright / 100);
    if (i == 0) { centeredP(0, PSTR("DESK WATCH")); centeredP(1, PSTR("")); }
    delay(25);
  }
  centeredP(0, PSTR("DESK WATCH")); centeredP(1, PSTR(""));   // Réécrit le titre au cas où l’écran n’était pas encore prêt.
  introSound();
  delay(1000);
  centeredP(0, BRAND1); centeredP(1, BRAND2);      // Deuxième écran : la marque.
  delay(1500);
  centeredP(0, TR("MELODIE EDITION", "MELODY EDITION", "MELODIA EDITION", "MELODIE EDITION"));
  lcd.setCursor(0, 1);
  for (uint8_t i = 0; i < 16; ++i) { lcd.write((uint8_t)255); delay(60); }
}

// ---------- Veille par appui très long ----------
void sleepNow() {
  stopAlarmPreview();
  centeredP(0, TR("A bientot", "Goodbye", "Hasta pronto", "Tschuess"));
  centeredP(1, PSTR(""));
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

// ---------- Programme principal ----------
void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  Wire.begin();
  loadCfg();
  lcd.begin(16, 2);
  delay(150);                                    // Laisse à l’écran le temps de terminer son initialisation.

  intro();

  Wire.beginTransmission(RTC_ADDR);
  rtcPresent = (Wire.endTransmission() == 0);
  pomoReset();
  readClock();
  shtOk = readSht();
  if (shtOk) updateAlerts();

  color(40, 90, 160);                            // Vérification des modules.
  centeredP(0, rtcPresent ? TR("Horloge RTC OK", "RTC clock OK", "Reloj RTC OK", "RTC-Uhr OK") : TR("Sans horloge RTC", "No RTC clock", "Sin reloj RTC", "Keine RTC-Uhr"));
  centeredP(1, shtOk ? TR("Capteur OK", "Sensor OK", "Sensor OK", "Sensor OK") : TR("Capteur absent", "Sensor missing", "Sensor ausente", "Sensor fehlt"));
  delay(1200);
  goHome();
  potReset();
  lastPotSample = potRead();
  lastActivity = millis();
}

void loop() {
  uint8_t e = readButton();
  bool tap = tapFlag; tapFlag = false;
  unsigned long now = millis();

  if (now - lastClockRead >= 200) { lastClockRead = now; readClock(); }
  if (now - lastSht >= 2000)      { lastSht = now; shtOk = readSht(); if (shtOk) updateAlerts(); }

  pomoTick();
  checkAlarm();
  if (!standby && nightActive() != nightPrev) applyColor();      // Début ou fin automatique du mode nuit.

  digitalWrite(PIN_LED, (standby || btnHeld >= LED_HOLD_MS) ? HIGH : LOW);   // LED allumée en veille et pendant l’appui très long.

  // Activité : appui ou mouvement de la molette, échantillonné toutes les 200 ms.
  if (e != EV_NONE || tap) lastActivity = now;
  if (now - lastPotCheck >= 200) {
    lastPotCheck = now;
    int pv = potRead();
    if (abs(pv - lastPotSample) > 25) { lastPotSample = pv; lastActivity = now; }
  }

  if (standby && !ringing && e != EV_NONE) { if (e != EV_VLONG) lastActivity = now; wakeUp(); e = EV_NONE; }   // Un appui réveille l’appareil.
  if (!ringing && e == EV_VLONG) { sleepNow(); e = EV_NONE; }   // Appui très long : mise en veille.
  if (!ringing && !standby && cfg.sleep != 0 && !(mode == M_CHESS && chessState == 1) &&
      now - lastActivity >= (unsigned long)pgm_read_byte(&SLEEP_MIN[cfg.sleep]) * 60000UL) { sleepNow(); e = EV_NONE; }   // Veille automatique.
  if (standby && !ringing) return;                    // Écran éteint : seuls le réveil et les fonctions en arrière-plan restent actifs.
  if (!ringing && e == EV_LONG) { backToMenu(); e = EV_NONE; }

  unsigned long period = (mode == M_CHRONO || mode == M_TALLY || mode == M_DIE || mode == M_CHESS) ? 100UL : 200UL;
  bool draw = (now - lastDraw >= period) || (e != EV_NONE);
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
