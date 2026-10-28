// Desk Watch : TEACHER EDITION — examen, température et humidité, minuteur Pomodoro, compteur, chronomètre, réveil, dé, pendule d'échecs et mélodies, pilotés par un bouton et une molette.
//
// Matériel : Arduino UNO + Grove Base Shield ; écran Grove LCD RGB V4.0 (I2C) ; capteur SHT31 ou SHT40 (I2C 0x44, détecté automatiquement) ;
//            buzzer sur D3 ; bouton Grove sur D4 ; potentiomètre rotatif Grove (molette) sur A1 ; module RTC DS1307 ou DS3231 OBLIGATOIRE (I2C 0x68).
// Le RTC fournit l'heure, le réveil et le mode nuit automatique. Sans RTC, l'appareil démarre et Temp et Hum reste utilisable (heure affichée « --:--:-- »),
// mais l'horloge, le réglage de l'heure et le réveil affichent « Installer module RTC », et le mode nuit automatique est inactif.
// Bibliothèque requise : « Grove - LCD RGB Backlight » (rgb_lcd.h). Le SHT et le RTC sont lus directement en I2C : aucune autre bibliothèque.
// TEACHER reprend MELODY (bouton et molette) et ajoute l'application EXAMEN : compte à rebours et collecte des copies sur un seul écran.
//
// ---- Commandes ----
//   MOLETTE             : défiler le menu, changer une valeur ; sa position choisit l'entrée ou la valeur (tourner à droite = suivant ou plus).
//   APPUI COURT         : OK (confirmé 0,3 s après le relâchement, pour laisser place au double clic).
//   DOUBLE CLIC         : deux appuis en moins de 0,3 s ; action propre à la page (voir ci-dessous).
//   APPUI LONG          : (0,8 s ou plus) retour : menu, annuler, étape précédente.
//   La mise en veille se lance depuis l'entrée « Mode veille » du menu ; un appui ou un tour de molette réveille l'appareil.
//   Réglages du câblage : BTN_ACTIVE (bouton) et POT_MAX (1023 en 5 V, 675 si le sélecteur du shield est sur 3,3 V).
//
// ---- Menu (11 entrées) ----
//   Examen, Temp et Hum, Minuteur, Compteur, Chrono, Réveil, Dé, Pendule d'échecs, Mélodie, Paramètres, Mode veille.
//   Le favori n'est pas une entrée du menu : il se choisit dans Paramètres (Menu par défaut, Temp et Hum, Minuteur, Chrono, Compteur, Dé, Pendule d'échecs, Examen) et fixe la page affichée au démarrage.
//   Paramètres (8) : luminosité, volume, son de démarrage, veille automatique, mode nuit, réglage de l'heure, favori, réglage usine.
//   Le son de démarrage est désactivé par défaut (réglable dans Paramètres). Les réglages sont enregistrés en EEPROM.
//
// ---- Pages ----
//   Examen      : réglage de la durée (1 à 240 min, molette puis appui court) et des copies attendues (1 à 99), puis appui court pour démarrer (suggestion : 35 min / 24 copies).
//                 Pendant l'examen, chaque appui court compte une copie immédiatement (même après la fin du temps) ; deux appuis rapides comptent deux copies.
//                 Appui long = commandes (molette puis appui court) : retour, retirer une copie, pause ou reprise, +5 min, terminer (avec confirmation), menu principal ; double clic = fermer les commandes.
//                 À zéro : deux bips courts, la collecte reste possible. Toutes les copies rendues : écran vert. Le message final alterne avec le temps de travail effectif, hors pauses.
//                 Le décompte continue en arrière-plan quand on ouvre le menu ; la veille automatique est suspendue pendant la session. La session reste en RAM : elle ne survit pas à une coupure.
//   Temp et Hum : ligne 1 = heure RTC et indicateurs des fonctions en arrière-plan ; ligne 2 = température, humidité et OK.
//                 En alerte, la ligne 2 alterne toutes les 2 s : mesures / alerte température / mesures / alerte humidité.
//                 Le rétroéclairage colore l'écran : trop haut = rouge, trop bas = bleu, OK = turquoise.
//                 DOUBLE CLIC = régler les quatre seuils (Tmin, Tmax, Hmin, Hmax) : molette = valeur, appui court = seuil suivant, puis appui court pour enregistrer ; appui long = seuil précédent.
//                 Les seuils sont enregistrés et réutilisés au démarrage. Appui court = menu. Le menu s'ouvre sur l'entrée de la page quittée ; tourner la molette reprend la main.
//                 La molette change de page (flèches <> à gauche de l'horloge) : 1 mesures, 2 minimum, 3 maximum, 4 moyenne, 5 « AUT » (en alternance avec les flèches) qui fait défiler les quatre pages toutes les 3 s.
//   Minuteur (Pomodoro) : appui court = démarrer ou pause ; double clic = remise à zéro (puis retour au menu si déjà à zéro).
//                 À l'arrêt, la molette règle la durée de travail (1 à 60 min, puis 65 min à 4 h par pas de 5). Pauses de 5 min, longue pause de 15 min toutes les 4 séances.
//                 Une fois lancé, tourner la molette ouvre le menu ; le décompte continue en arrière-plan.
//   Compteur    : chaque appui court ajoute 1 immédiatement. Tourner la molette ouvre l'écran d'action : appui court = -1, double clic = remise à zéro, 5 s sans réponse ou appui long = annuler.
//   Chrono      : appui court = démarrer ou arrêter ; double clic = remise à zéro ; si déjà à zéro, double clic = menu ; tourner la molette = menu (le chrono continue).
//   Réveil      : appui court = activer et régler (heures, minutes, mélodie, volume ; appui court = étape suivante, appui long = étape précédente), double clic = désactiver. Huit sonneries, avec préécoute.
//                 Tourner la molette sur l'écran d'accueil du réveil ouvre le menu.
//                 Pendant la sonnerie : tout appui ou tour de molette l'arrête pour la journée et ouvre le menu. Sans réaction : arrêt après 120 s, reprise 5 min plus tard, jusqu'à un contact.
//   Dé          : appui court = lancer (cliquetis 1 s, puis résultat) ; molette = nombre de faces (2, 4, 6, 8, 10, 12, 20, 30, 100) ; double clic = menu.
//   Pendule d'échecs : molette = cadence à l'arrêt (1+0, 3+0, 3+2, 5+0, 5+3, 10+0, 10+5, 15+10, 20+0, 30+0, 60+0) ; appui court = démarrer. En partie, appui court = passer à l'autre joueur ;
//                 double clic = pause ; ensuite appui court = reprendre, double clic = remise à zéro, appui long = menu.
//   Mélodie     : molette = choisir ; appui court = lire, pause ou reprise ; double clic = menu.
//   Mode nuit   : Non / Auto (22 h à 6 h, avec RTC) / Oui : luminosité réduite à 8 %, bips d'alerte et mélodie de démarrage coupés ; le réveil sonne normalement.
//   Veille automatique : jamais, 1, 2, 5, 10, 15, 30 min ou 1 h sans activité (appui ou mouvement de la molette) ; désactivée pendant une partie d'échecs en cours.
//   Mode veille : écran éteint, réveil toujours actif. La LED de l'UNO (broche 13) s'allume en veille.
//   Indicateurs en haut à droite : ! = alerte température / humidité ; R = réveil activé ; C = chronomètre en cours ; M = minuteur en cours (colonne masquée 1 s toutes les 10 s).
//   Démarrage : DESK WATCH, courte mélodie (si activée), PR BELAMINE MAKERLAB, barre de chargement TEACHER, vérification des modules.
//
// Les noms internes « molette » (potLive, potReset, potSelect…) désignent le potentiomètre rotatif.

#include <Wire.h>
#include <EEPROM.h>
#include <rgb_lcd.h>

// Textes répétés mutualisés en mémoire flash, sans changer les traductions.
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
#define TR_PTR(fr, en, es, de) (fr)

// ---------- Réglages ----------
const uint8_t PIN_BUZZER = 3;
const uint8_t PIN_BUTTON = 4;
const uint8_t PIN_LED    = 13;         // LED L de l’UNO ; pour une LED rouge externe, brancher un module Grove LED et adapter cette broche.
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

enum { M_TH, M_POMO, M_CHRONO, M_TALLY, M_DIE, M_CHESS, M_MUSIC, M_PARAMS, M_HOME, M_ALARM, M_SETCLOCK, M_BRIGHT, M_MENU, M_CLOCK, M_SLEEP, M_EXAM };
const uint8_t N_MENU = 11;
const uint8_t N_PAR = 8;   // visible settings (the language choice was removed: one language per edition)
uint8_t mode = M_HOME, prevMode = M_HOME;   // Valeurs de l’énumération M_*.
uint8_t menuIdx = 0;
uint8_t brightBackup = 100, brightIdx = 7;

enum { EV_NONE, EV_SHORT, EV_LONG, EV_VLONG, EV_DOUBLE, EV_PRESS };

struct Settings { uint8_t magic, aH, aM, armed, workMin, bright, tLo2, tHi2, hLo, hHi, sound, lang, volume, home, night, sleep; } cfg;   // lang : 0 français, 1 anglais, 2 espagnol, 3 allemand ; home : favori (0 menu, 1 Temp et Hum, 2 minuteur, 3 chrono, 4 compteur, 5 dé, 6 pendule d’échecs) ; night : mode nuit (0 non, 1 automatique avec RTC, 2 oui) ; sleep : indice du délai dans SLEEP_MIN (0 = jamais).

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
uint8_t alarmSub = 0;       // 0 activation, 1 hours, 2 minutes, 3 melody, 4 volume, 5 confirmation.
bool alarmExitPending = false;
unsigned long alarmExitAt = 0;
bool alarmSavedPending = false;
unsigned long alarmSavedReturnAt = 0;
// Extension EEPROM distincte pour préserver les réglages existants et leurs adresses.
struct AlarmSettings { uint8_t magic, melody, volume; } alarmCfg;
uint8_t alarmPos = 0;
unsigned long alarmNoteEnd = 0;
bool alarmPreview = false;
uint8_t previewMelody = 0, previewVolume = 0;
uint8_t parSub = 0, parBackup = 0;   // Réglage ouvert et valeur sauvegardée avant modification.
uint8_t parIdx = 1;          // Page Paramètres : 1 luminosité, 2 volume, 3 son, 4 veille, 5 mode nuit, 6 heure, 7 favori, 8 réglage usine
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
const uint8_t N_CHESS = 11;
static const uint8_t CHESS_MIN[N_CHESS] PROGMEM = { 1, 3, 3, 5, 5, 10, 10, 15, 20, 30, 60 };
static const uint8_t CHESS_INC[N_CHESS] PROGMEM = { 0, 0, 2, 0, 3, 0, 5, 10, 0, 0, 0 };
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
static const char MUSIC_NAMES[] PROGMEM =
  "Anniversaire\0"
  "Pour Elise\0"
  "Mario Bros\0"
  "Tetris\0";
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

PGM_P packedText(PGM_P text, uint8_t index);
PGM_P alarmTuneName() {
  return alarmCfg.melody < N_MUSIC ? packedText(MUSIC_NAMES, alarmCfg.melody)
                                 : ALARM_EXTRA_NAMES[alarmCfg.melody - N_MUSIC];
}

uint8_t musicTrack = 0;
uint8_t musicState = 0, musicPos = 0;  // 0 = prêt, 1 = lecture, 2 = pause ; indice de la prochaine note à jouer.
int musicRemain = 0;                  // Durée restante de la note courante, en millisecondes.
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
bool potActive = false; int potAnchor = 0; bool dialAct = false;   // Molette : ancre, mouvement détecté, activité récente.
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

// Mode nuit actif si « Oui » est choisi ou, en mode automatique, entre NIGHT_START et NIGHT_END avec le RTC.
bool nightActive() {
  if (cfg.night == 2) return true;
  return cfg.night == 1 && rtcPresent && (nowH >= NIGHT_START || nowH < NIGHT_END);
}
bool nightPrev = false;
bool validationFlashActive = false;
unsigned long validationFlashDuration = 1000UL;
unsigned long validationFlashStart = 0;

// Applique la couleur demandée selon la luminosité choisie ; atténue la nuit, sauf pendant la sonnerie.
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
  if (mode == M_POMO && (r | g | b)) statusColor = true;   // Minuteur : couleurs propres à chaque état, voir drawPomo.
  if (mode == M_EXAM && (r | g | b)) statusColor = true;
  if (!statusColor) {
    switch (mode) {
    case M_TH: r=0; g=145; b=145; break;
    case M_POMO: r=205; g=80; b=0; break;
    case M_CHRONO: r=0; g=105; b=205; break;
    case M_TALLY: r=0; g=155; b=45; break;
    case M_DIE: r=125; g=40; b=170; break;
    case M_CHESS: r=170; g=40; b=125; break;
    case M_EXAM: r=40; g=90; b=160; break;
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

// Écrit exactement seize caractères, depuis la RAM ou la flash, puis complète avec des espaces.
// Cette routine commune évite les tampons intermédiaires et économise la mémoire programme.
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

// Les textes flash séparés par des zéros évitent le remplissage des tableaux à largeur fixe.
PGM_P packedText(PGM_P text, uint8_t index) {
  while (index--) text += strlen_P(text) + 1;
  return text;
}

PGM_P volumeName(uint8_t volume) {
  static const char N0[] PROGMEM = "Min", N4[] PROGMEM = "Max";
  return volume == 0 ? N0 : volume == 4 ? N4 : volume == 1 ? TR("Bas", "Low", "Bajo", "Leise") : volume == 2 ? TR("Moyen", "Mid", "Medio", "Mitte") : TR("Fort", "High", "Alto", "Laut");
}

void musicToneStop() {
  TCCR2A = 0;
  TCCR2B = 0;
  PORTD &= ~_BV(PD3);
}

void musicToneStartAtVolume(unsigned int freq, uint8_t volume) {
  static const uint8_t DUTY[5] = { 5, 10, 20, 35, 50 };
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

// Lecture commune au réveil et à sa préécoute, sans attente bloquante.
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

// Buzzer : signal rectangulaire à la fréquence demandée ; le rapport cyclique règle le volume.
// Rapports de 5 %, 20 % ou 50 % : une impulsion plus courte atténue le buzzer piézoélectrique. Fonction bloquante pendant ms millisecondes.
void buzz(unsigned int freq, unsigned int ms) {
  if (alarmPreview) musicToneStop();   // Laisse temporairement la broche aux bips de notification.
  static const uint8_t DUTY[5] = { 5, 10, 20, 35, 50 };
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

// ---------- Bouton et molette ----------
// readButton() lit le bouton D4 : EV_SHORT (appui court, confirmé après le délai du double clic), EV_DOUBLE, EV_LONG (0,8 s ou plus)
// et EV_PRESS (musique, immédiat). Il détecte aussi un mouvement de la molette (dialAct).
uint8_t readButton() {
  static bool down = false, pending = false, second = false, musicPressHandled = false;
  static unsigned long t0 = 0, tRel = 0;
  bool p = (digitalRead(PIN_BUTTON) == BTN_ACTIVE);
  unsigned long now = millis();
  btnHeld = (p && down) ? now - t0 : 0;

  if (now - lastPotCheck >= 60) {                  // Mouvement de la molette : réveil, arrêt de la sonnerie, veille automatique.
    lastPotCheck = now;
    int pv = potRead();
    if (abs(pv - lastPotSample) > 25) { lastPotSample = pv; dialAct = true; }
  }

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
    return EV_LONG;
  }
  return EV_NONE;
}

// ---------- Configuration EEPROM ----------
void loadCfg() {
  EEPROM.get(0, cfg);
  if (cfg.magic == 0xAB) {
    if (cfg.home == 2) cfg.home = 0; else if (cfg.home >= 3 && cfg.home <= 5) cfg.home--;
    cfg.magic = 0xAC; EEPROM.put(0, cfg);
  }
  if (cfg.magic != 0xAC || cfg.aH > 23 || cfg.aM > 59 || cfg.armed > 1 ||
      cfg.workMin < 1 || cfg.workMin > 240) {
    cfg.magic = 0xAC; cfg.aH = 7; cfg.aM = 0; cfg.armed = 0; cfg.workMin = 25;
    cfg.bright = 85; cfg.sound = 0; cfg.volume = 2; cfg.home = 0; cfg.night = 1; cfg.sleep = 0; cfg.tLo2 = 0xFF;       // Les seuils sont vérifiés plus bas.
  }
  if (cfg.sleep > 7) cfg.sleep = 0;                     // Veille automatique désactivée par défaut.
  if (cfg.night > 2) cfg.night = 1;                     // Mode nuit automatique par défaut.
  if (cfg.home > 7) cfg.home = 0;                       // Favori Menu par défaut.
  if (cfg.volume > 4) cfg.volume = 2;                   // Volume moyen par défaut.
  if (cfg.sound > 1) cfg.sound = 0;                    // Son de démarrage désactivé par défaut.
  // Rétablit les seuils par défaut s’ils sont absents ou incohérents.
  if (cfg.tLo2 < T_MIN2 || cfg.tHi2 > T_MAX2 || cfg.tHi2 < cfg.tLo2 + T_GAP2 ||
      cfg.hLo < H_MIN || cfg.hHi > H_MAX || cfg.hHi < cfg.hLo + H_GAP) {
    cfg.tLo2 = T_LO_DEFAULT2; cfg.tHi2 = T_HI_DEFAULT2;
    cfg.hLo = H_LO_DEFAULT;   cfg.hHi = H_HI_DEFAULT;
  }
  if (cfg.bright < 1 || cfg.bright > 100) cfg.bright = 85;   // Ancienne sauvegarde ou valeur incorrecte.
  EEPROM.get(sizeof(cfg), alarmCfg);
  if (alarmCfg.magic != 0xB1 || alarmCfg.melody >= N_ALARM_TUNES || alarmCfg.volume > 4) {
    alarmCfg.magic = 0xB1; alarmCfg.melody = 0; alarmCfg.volume = cfg.volume;
  }
}
void saveCfg(unsigned long validationDuration = 1000UL) { EEPROM.put(0, cfg); EEPROM.put(sizeof(cfg), alarmCfg); startValidationFlash(validationDuration); }

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
  startValidationFlash();
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

// Capteur d’humidité : SHT31 ou SHT40, détecté automatiquement (même adresse I2C 0x44, commandes différentes, même CRC).
uint8_t shtType = 0;   // 0 = pas encore détecté, 1 = SHT31, 2 = SHT40
bool shtTry(uint8_t type, uint8_t *b) {
  Wire.beginTransmission(SHT_ADDR);
  if (type == 1) { Wire.write((uint8_t)0x24); Wire.write((uint8_t)0x00); }   // SHT31 : mesure avec répétabilité élevée.
  else Wire.write((uint8_t)0xFD);                                            // SHT40 : mesure de haute précision.
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
  for (uint8_t t = shtType ? shtType : 1; t <= (shtType ? shtType : 2); t++) {   // type connu : un seul essai ; sinon SHT31 puis SHT40
    if (!shtTry(t, b)) continue;
    got = true;
    if (crc8(b, 2) == b[2] && crc8(b + 3, 2) == b[5]) { shtType = t; crcOk = true; break; }
  }
  if (!crcOk) {
    if (got && shtType && ++crcBad < 3) return shtOk;   // lecture corrompue : on garde les valeurs précédentes ; panne signalée après 3 mauvaises lectures de suite
    shtType = 0;                                        // capteur absent ou changé : nouvelle détection à la prochaine lecture
    return false;
  }
  crcBad = 0;
  uint16_t rt = ((uint16_t)b[0] << 8) | b[1];
  uint16_t rh = ((uint16_t)b[3] << 8) | b[4];
  temp10 = (int)((1750L * rt) / 65535L) - 450 + TEMP_OFFSET10;                    // −45 + 175 × S / 65535, identique pour les deux capteurs
  hum10  = (shtType == 1 ? (int)((1000L * rh) / 65535L) : (int)((1250L * rh) / 65535L) - 60) + HUM_OFFSET10;   // SHT31 : 100 × S / 65535 ; SHT40 : −6 + 125 × S / 65535
  if (hum10 < 0) hum10 = 0;
  if (hum10 > 1000) hum10 = 1000;
  if (thSampleCount == 30) { thTempSum -= thTempSamples[thSamplePos]; thHumSum -= thHumSamples[thSamplePos]; }
  else thSampleCount++;
  thTempSamples[thSamplePos] = temp10; thHumSamples[thSamplePos] = hum10;
  thTempSum += temp10; thHumSum += hum10;
  thSamplePos = (thSamplePos + 1) % 30;
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
    lineP(1, TR("Toucher = arret", "Touch = stop", "Tocar = parar", "Beruehren=Stopp"));
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
uint8_t parValue(uint8_t i) { return i == 2 ? cfg.volume : i == 3 ? cfg.sound : i == 4 ? cfg.sleep : i == 5 ? cfg.night : i == 7 ? cfg.home : 0; }
void setParValue(uint8_t i, uint8_t v) { if (i == 2) cfg.volume = v; else if (i == 3) cfg.sound = v; else if (i == 4) cfg.sleep = v; else if (i == 5) cfg.night = v; else if (i == 7) cfg.home = v; }

void chessPause() {                    // Quitter la page met la pendule d’échecs en pause.
  if (chessState == 1) { chessBase[chessSide] -= (long)(millis() - chessTurn); chessState = 2; }
}

// EXAMEN : réglage, collecte immédiate et commandes séparées des appuis de comptage.
enum { EX_DURATION, EX_TARGET, EX_READY, EX_RUNNING, EX_PAUSED, EX_ELAPSED, EX_FINISHED };
uint8_t examState = EX_DURATION, examMinutes = 35, examTarget = 24, examCopies = 0;
uint8_t examSub = 0, examAction = 0;
unsigned long examRemaining = 0, examLast = 0;
unsigned long examElapsed = 0;   // Temps de travail effectif, hors pauses ; figé après la dernière copie ou l’expiration.

bool examSession() { return examState >= EX_RUNNING; }

void examColor() {
  if (!examSession()) color(40, 90, 160);
  else color(examCopies == examTarget ? 0 : examState == EX_ELAPSED ? 255 : 40,
             examCopies == examTarget ? 160 : examState == EX_ELAPSED ? 0 : 90, 40);
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

// Démarrage/réveil : ouvre la page choisie dans les paramètres ; 0 = menu.
void goHome() {
  uint8_t h = cfg.home;
  enterMode(h == 0 ? M_MENU : h == 1 ? M_TH : h == 2 ? M_POMO : h == 3 ? M_CHRONO : h == 4 ? M_TALLY : h == 5 ? M_DIE : h == 6 ? M_CHESS : M_EXAM);
}

// Revient aux paramètres sur l’entrée d’indice i.
void backToParams(uint8_t i) { enterMode(M_PARAMS); parIdx = i; }

void openMenu() {
  stopAlarmPreview();
  if (mode == M_MUSIC) musicToneStop();
  if (mode == M_CHESS) chessPause();
  prevMode = mode;
  menuIdx = 0;                                         // Le menu s’ouvre sur l’entrée de la page quittée (1/10 pour Temp et Hum) ; la molette prend la main dès qu’elle bouge.
  { uint8_t menuMode(uint8_t); for (uint8_t i = 0; i < N_MENU - 1; i++) if (menuMode(i) == mode) { menuIdx = i; break; } }
  mode = M_MENU;
  clockSub = 0; alarmSub = 0; thSub = 0; parSub = 0; tallySub = 0;
  potReset();
  lastDraw = 0;
}

void backToMenu() {
  if (mode == M_MENU) { enterMode(prevMode); return; }          // Dans le menu : retour à la page précédente.
  if (mode == M_BRIGHT) { cfg.bright = brightBackup; applyColor(); backToParams(1); return; }   // Annule le changement de luminosité.
  if (mode == M_ALARM && alarmSub != 0) saveCfg();
  if (mode == M_TH && thSub != 0) saveCfg();
  if (mode == M_PARAMS && parSub != 0) { setParValue(parIdx, parBackup); parSub = 0; potReset(); lastDraw = 0; return; }
  openMenu();
}

// Entrée du menu : position de la molette, répartie sur les dix entrées.
uint8_t menuFromPot() {
  int z = potRead() / ((POT_MAX + 1) / N_MENU);
  return (uint8_t)(z > N_MENU - 1 ? N_MENU - 1 : z);
}

// Lettres des fonctions actives en arrière-plan, sauf celle de la page affichée ; colonnes 13 à 15, numérotées depuis zéro.
void badges(uint8_t self) {
  char all[5];                                       // Ordre : ! R C M ; le minuteur reste le plus à droite.
  uint8_t n = 0;
  if (self != M_TH     && anyAlert())                all[n++] = '!';
  if (self != M_ALARM  && cfg.armed && rtcPresent)   all[n++] = 'R';
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
void sleepNow();
// Couleurs du minuteur : prêt = bleu, travail = orange, pause du minuteur = jaune, pause courte = vert, pause longue = cyan.
void pomoColor() {
  if (pomoIdle()) color(40, 90, 160);
  else if (!pomoRun) color(200, 150, 0);
  else if (pomoPhase == 0) color(255, 70, 0);
  else if (pomoPhase == 1) color(0, 170, 50);
  else color(0, 170, 200);
}

void drawMenu(uint8_t e, bool draw) {
  static const char names[] PROGMEM =
  "Examen\0"
  "Heure Temp Hum\0"
  "Minuteur\0"
  "Compteur\0"
  "Chrono\0"
  "Reveil\0"
  "De\0"
  "Pendule Echecs\0"
  "Melodie\0"
  "Parametres\0"
  "Mode veille\0"
;
  if (potLive()) menuIdx = menuFromPot();
  if (e == EV_SHORT) { if (menuIdx == N_MENU - 1) sleepNow(); else enterMode(menuMode(menuIdx)); return; }
  if (e == EV_LONG || e == EV_VLONG) { enterMode(prevMode); return; }
  if (!draw) return;
  char b[17];
  uint8_t displayMode = mode;
  mode = menuIdx == N_MENU - 1 ? M_SLEEP : menuMode(menuIdx);
  if (mode == M_POMO) pomoColor(); else if (mode == M_EXAM) examColor(); else color(0, 0, 0);   // Même couleur dans le menu et dans la page.
  mode = displayMode;
  snprintf_P(b, sizeof(b), PSTR("MENU       %2d/%d"), menuIdx + 1, N_MENU);
  line(0, b);
  char nm[20]; strcpy_P(nm, packedText(names, menuIdx));
  snprintf_P(b, sizeof(b), PSTR("> %s"), nm);
  line(1, b);
}

uint8_t menuMode(uint8_t i) {
  static const uint8_t modes[N_MENU] PROGMEM = { M_EXAM, M_TH, M_POMO, M_TALLY, M_CHRONO, M_ALARM, M_DIE, M_CHESS, M_MUSIC, M_PARAMS, M_HOME };
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
// Favoris possibles : 0 menu, 1 Temp et Hum, 2 minuteur, 3 chronomètre, 4 compteur, 5 dé, 6 pendule d’échecs.
static const char homeName[] PROGMEM =
  "Menu\0"
  "Heure Temp Hum\0"
  "Minuteur\0"
  "Chrono\0"
  "Compteur\0"
  "De\0"
  "Pendule Echecs\0"
  "Examen\0"
;
// Paramètres : luminosité, volume, son de démarrage, veille automatique, mode nuit, heure et favori.

void drawParams(uint8_t e, bool draw) {
  if (e == EV_DOUBLE) { backToMenu(); return; }          // Double clic : retour au menu et annulation du réglage ouvert.
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
      uint8_t count = parIdx == 2 ? 5 : parIdx == 3 ? 2 : parIdx == 4 ? 8 : parIdx == 5 ? 3 : parIdx == 7 ? 8 : 0;
      uint8_t v = potSelect(count, cur, parIdx == 2 ? 40 : 30);
      if (parIdx != 8 && v != cur) { setParValue(parIdx, v); if (parIdx == 2) beep(1, 100); }
    }
    if (e == EV_SHORT) {
      if (parIdx == 8) {
        cfg.magic = 0xAC; cfg.aH = 7; cfg.aM = 0; cfg.armed = 0; cfg.workMin = 25;
        cfg.bright = 85; cfg.sound = 0; cfg.volume = 2; cfg.home = 0; cfg.night = 1; cfg.sleep = 0;
        cfg.tLo2 = T_LO_DEFAULT2; cfg.tHi2 = T_HI_DEFAULT2; cfg.hLo = H_LO_DEFAULT; cfg.hHi = H_HI_DEFAULT;
        alarmCfg.magic = 0xB1; alarmCfg.melody = 0; alarmCfg.volume = 2;
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
  char b[17], nm[14], val[18];
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
    lineP(1, parSub ? PSTR("OK confirmer") : PSTR("OK=ouvrir"));
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
    char bar[6] = "     ";
    for (uint8_t i = 0; i <= cfg.volume; i++) bar[i] = '#';                            // Cinq niveaux : un dièse par niveau.
    strcpy_P(val, volumeName(cfg.volume));
    snprintf_P(b, sizeof(b), parSub ? PSTR("<>%-5s %-5s OK") : PSTR("%-5s %-5s  OK"), val, bar);
    line(1, b);
    return;
  }
  snprintf_P(b, sizeof(b), parSub ? PSTR("<> %-9s  OK") : PSTR("%-9s OK=chg"), val);
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
  const char* pre = (thView == 4 && (millis() / 1500UL) % 2 == 0) ? "AUT " : "<>  ";   // pages 1 à 4 : flèches <> à gauche de l’horloge ; page 5 : « AUT » en alternance avec les flèches (1,5 s) ; l’horloge occupe les colonnes 4 à 11
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

  // Deuxième ligne : mesures et message complet alternent ; les mesures précèdent chaque alerte.
  uint8_t view = thView;                                           // 0 mesures, 1 minimum, 2 maximum, 3 moyenne, 4 AUT (défile les quatre pages toutes les 3 s)
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
  // Le symbole degré correspond au caractère LCD 0xDF ; les mesures occupent au plus treize colonnes.
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
    if (e == EV_SHORT)             { cfg.armed = 1; potReset(); saveCfg(); alarmSub = 1; }   // Appui court : activer puis entrer dans les réglages.
    else if (e == EV_DOUBLE)       { cfg.armed = 0; potReset(); saveCfg(); alarmExitPending = true; alarmExitAt = millis() + 1500UL; } // Double clic : désactiver puis quitter après confirmation.
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
  if (alarmSub == 4 && potLive()) alarmCfg.volume = potSelect(5, alarmCfg.volume, 20);
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
    if (tap && tallyCount < 9999) { tallyCount++; lastDraw = 0; }   // Chaque appui est compté immédiatement.
    if (potLive()) { tallySub = 1; tallyT0 = millis(); }            // Molette : -1 ou remise à zéro.
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
  if (e == EV_DOUBLE) { openMenu(); return; }          // Double clic : revenir au menu.
  uint8_t faces = pgm_read_byte(&DIE_FACES[dieIdx]);
  if (dieRolling) {
    if ((long)(millis() - dieEnd) >= 0) {              // Fin du lancer : résultat définitif.
      seedRng(micros() + analogRead(PIN_POT));
      dieVal = (dieShow >= 1 && dieShow <= faces) ? dieShow : rollDie(faces);   // Le résultat est le dernier nombre affiché : il ne change plus à l’arrêt.
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
      dieRolling = true; dieEnd = millis() + 900; dieVal = 0; dieShow = 0;
    }
  }
  if (!draw) return;
  char b[17];
  color(170, 60, 130);
  // Ligne 1 : titre et rappel des gestes (colonnes 13 à 15 réservées aux indicateurs) ; ligne 2 : D + faces à gauche, texte ou nombre aligné à droite.
  if (dieRolling)       lineP(0, TR("DE  lance...", "DICE rolling", "DADO tirando", "WUERFEL rollt"));
  else if (dieVal != 0) lineP(0, TR("DE OK=relance", "DICE OK=roll", "DADO OK=tirar", "WUERFEL OK"));
  else                  lineP(0, TR("DE  <> faces", "DICE <> sides", "DADO <> caras", "WUERFEL <>"));
  badges(M_DIE);
  char r[12];                                          // partie droite de la ligne 2, alignée sur la colonne 15
  if (dieRolling) {
    dieShow = rollDie(faces);
    buzz(1800, 5);                                     // Petit cliquetis pendant le lancer.
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
  strcpy_P(title, packedText(MUSIC_NAMES, musicTrack));
  snprintf_P(b, sizeof(b), PSTR("%u/%u %s"), musicTrack + 1, N_MUSIC, title);
  line(0, b);
  badges(M_MUSIC);
  if (musicState == 1)      lineP(1, TR("Lecture OK=pause", "Playing OK=pause", "Suena OK=pausa", "Spielt OK=Pause"));
  else if (musicState == 2) lineP(1, TR("Pause OK=lire", "Paused OK=play", "Pausa OK=tocar", "Pause OK=Start"));
  else                   lineP(1, TR("Molette=choix OK", "Dial=choose OK", "Dial=gira OK", "Dreh=wahl OK"));
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
    applyColor();                                    // Prévisualisation immédiate.
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
void introSound() {                              // Mélodie de quatre notes.
  if (!cfg.sound || nightActive()) return;
  buzz(1500, 90); delay(40);
  buzz(2000, 90); delay(40);
  buzz(2600, 90); delay(40);
  buzz(3100, 340);                               // Quatrième note, plus longue.
  digitalWrite(PIN_BUZZER, LOW);
}
void intro() {
  lastR = 40; lastG = 90; lastB = 160;
  for (uint8_t i = 0; i <= 100; i += 5) {        // Allumage progressif du rétroéclairage.
    lcd.setRGB(40 * i / 100 * cfg.bright / 100, 90 * i / 100 * cfg.bright / 100, 160 * i / 100 * cfg.bright / 100);
    if (i == 0) { centeredP(0, TXT_DESK_WATCH_11); centeredP(1, TXT_EMPTY_0); }
    delay(25);
  }
  centeredP(0, TXT_DESK_WATCH_11); centeredP(1, TXT_EMPTY_0);   // Réécrit le titre au cas où l’écran n’était pas encore prêt.
  introSound();
  delay(1000);
  centeredP(0, BRAND1); centeredP(1, BRAND2);      // Deuxième écran : la marque.
  delay(1500);
  centeredP(0, PSTR("TEACHER EDITION"));
  lcd.setCursor(0, 1);
  for (uint8_t i = 0; i < 16; ++i) { lcd.write((uint8_t)255); delay(60); }
}

// ---------- Veille par appui très long ----------
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

// ---------- Programme principal ----------
void examTick() {
  unsigned long now = millis();
  if (examState == EX_RUNNING) {
    unsigned long dt = now - examLast;
    if (examCopies < examTarget) examElapsed += min(dt, examRemaining);
    if (dt >= examRemaining) {
      examRemaining = 0; examState = EX_ELAPSED;
      if (!ringing) beep(2, 100);
    } else examRemaining -= dt;
  }
  examLast = millis();
}

void drawExam(uint8_t e, bool draw, bool tap) {
  if (examState == EX_FINISHED && examSub == 0 && e == EV_SHORT) {
    examState = EX_DURATION; potReset(); return;
  }
  if (e == EV_LONG) {
    if (!examSession()) { openMenu(); return; }
    examSub = examSub ? 0 : 1; examAction = 0; potReset();
  }
  if (examSub == 2) {
    if (e == EV_SHORT) { examState = EX_FINISHED; examSub = 0; potReset(); return; }
    if (e == EV_DOUBLE) { examSub = 0; potReset(); return; }
    if (draw) {
      color(150, 100, 0);
      lineP(0, TR("Terminer examen?", "End exam?", "Terminar examen?", "Pruefung enden?"));
      lineP(1, TR("OK=oui  dbl=non", "OK=yes  dbl=no", "OK=si   dbl=no", "OK=ja  dbl=nein"));
    }
    return;
  }
  if (examSub == 1) {
    if (potLive()) examAction = potSelect(6, examAction, 15);
    if (e == EV_DOUBLE) { examSub = 0; potReset(); return; }
    if (e == EV_SHORT) {
      examSub = 0;
      if (examAction == 1 && examCopies) examCopies--;
      if (examAction == 2) {
        if (examState == EX_RUNNING) examState = EX_PAUSED;
        else if (examRemaining) { examState = EX_RUNNING; examLast = millis(); }
      }
      if (examAction == 3) {
        examRemaining = min(examRemaining + 300000UL, 14400000UL);
        if (examState == EX_ELAPSED) examState = EX_RUNNING;
        examLast = millis();
      }
      if (examAction == 4) examSub = 2;
      if (examAction == 5) { openMenu(); return; }
      potReset(); return;
    }
    if (draw) {
      color(150, 100, 0);
      lineP(0, TR("EXAMEN: actions", "EXAM: actions", "EXAMEN: acciones", "PRUEFUNG: Menue"));
      PGM_P label = examAction == 0 ? TR("Retour", "Back", "Volver", "Zurueck")
        : examAction == 1 ? TR("Retirer 1 copie", "Subtract 1 paper", "Restar 1 copia", "1 Blatt weniger")
        : examAction == 2 ? (examState == EX_RUNNING ? TR("Pause", "Pause", "Pausa", "Pause") : TR("Reprendre", "Resume", "Continuar", "Fortsetzen"))
        : examAction == 3 ? PSTR("+5 min")
        : examAction == 4 ? TR("Terminer", "End exam", "Terminar", "Beenden")
        : TR("Menu principal", "Main menu", "Menu principal", "Hauptmenue");
      lineP(1, label);
    }
    return;
  }
  if (!examSession()) {
    if (e == EV_SHORT) {
      if (examState == EX_READY) {
        examCopies = 0; examElapsed = 0; examRemaining = (unsigned long)examMinutes * 60000UL;
        examState = EX_RUNNING; examLast = millis();
      } else examState++;
      potReset();
    }
    if (examState == EX_DURATION && potLive()) examMinutes = 1 + potSelect(240, examMinutes - 1, 2);
    if (examState == EX_TARGET && potLive()) examTarget = 1 + potSelect(99, examTarget - 1, 4);
    if (draw && !examSession()) {
      char b[17]; color(40, 90, 160);
      lineP(0, examState == EX_DURATION ? TR("Duree examen", "Exam duration", "Duracion examen", "Pruefungsdauer")
        : examState == EX_TARGET ? TR("Copies attendues", "Expected papers", "Copias previstas", "Anzahl Blaetter")
        : TR("EXAMEN PRET", "EXAM READY", "EXAMEN LISTO", "PRUEFUNG BEREIT"));
      if (examState == EX_DURATION) snprintf_P(b, sizeof(b), PSTR("%u min   OK"), examMinutes);
      else if (examState == EX_TARGET) snprintf_P(b, sizeof(b), PSTR("%u   OK"), examTarget);
      else snprintf_P(b, sizeof(b), PSTR("%umin %u  OK"), examMinutes, examTarget);
      line(1, b); return;
    }
    if (!examSession()) return;
  }
  // Chaque relâchement court compte, y compris les deux appuis d’un double clic.
  if (tap && examState != EX_FINISHED && examCopies < examTarget) { examCopies++; draw = true; }
  if (!draw) return;
  char b[17]; unsigned long secs = (examRemaining + 999UL) / 1000UL;
  examColor();
  if (examState == EX_FINISHED || ((examState == EX_ELAPSED || examCopies == examTarget) && (millis() / 2000UL) % 2)) {
    unsigned long elapsed = examElapsed / 1000UL;
    snprintf_P(b, sizeof(b), PSTR("TOTAL     %3lu:%02lu"), elapsed / 60UL, elapsed % 60UL);
    line(0, b);
  }
  else if (examCopies == examTarget) lineP(0, TR("COPIES RENDUES", "ALL PAPERS IN", "TODAS ENTREGADAS", "ALLE ABGEGEBEN"));
  else if (examState == EX_ELAPSED) lineP(0, TR("TEMPS ECOULE", "TIME IS UP", "TIEMPO AGOTADO", "ZEIT ABGELAUFEN"));
  else {
    snprintf_P(b, sizeof(b), examState == EX_PAUSED ? PSTR("PAUSE     %3lu:%02lu") : PSTR("EXAMEN    %3lu:%02lu"), secs / 60UL, secs % 60UL);
    line(0, b);
  }
  snprintf_P(b, sizeof(b), TR("Copies     %02u/%02u", "Papers     %02u/%02u", "Copias     %02u/%02u", "Blaetter   %02u/%02u"), examCopies, examTarget);
  line(1, b);
}

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  Wire.begin();
  loadCfg();
  lcd.begin(16, 2);
  {                                              // Flèches du menu : bas, haut, droite (caractères 1 à 3 ; le zéro termine les textes).
    static const uint8_t ARROWS[3][8] PROGMEM = {
      { 0x04, 0x04, 0x04, 0x04, 0x15, 0x0E, 0x04, 0x00 },
      { 0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x04, 0x00 },
      { 0x00, 0x04, 0x02, 0x1F, 0x02, 0x04, 0x00, 0x00 } };
    for (uint8_t i = 0; i < 3; i++) { uint8_t g[8]; memcpy_P(g, ARROWS[i], 8); lcd.createChar(i + 1, g); }
  }
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
  examTick();
  checkAlarm();
  if (!standby && nightActive() != nightPrev) applyColor();      // Début ou fin automatique du mode nuit.
  updateValidationFlash();

  digitalWrite(PIN_LED, standby ? HIGH : LOW);   // LED allumée en veille.

  // Activité : appui ou mouvement de la molette, échantillonné toutes les 200 ms.
  if (e != EV_NONE || tap || dial) lastActivity = now;
  if ((ringing || snoozing) && (e != EV_NONE || tap || dial)) {   // Sonnerie ou snooze : tout appui ou tour de molette arrête le réveil du jour et ouvre le menu.
    stopRing();
    if (standby) { standby = false; lcd.clear(); }
    openMenu();
    return;
  }

  if (standby && !ringing && (e != EV_NONE || dial)) { lastActivity = now; wakeUp(); e = EV_NONE; }   // Un appui réveille l’appareil.
  if (!ringing && !standby && cfg.sleep != 0 && !examSession() && !(mode == M_CHESS && chessState == 1) &&
      now - lastActivity >= (unsigned long)pgm_read_byte(&SLEEP_MIN[cfg.sleep]) * 60000UL) { sleepNow(); e = EV_NONE; }   // Veille automatique.
  if (standby && !ringing) return;                    // Écran éteint : seuls le réveil et les fonctions en arrière-plan restent actifs.
  if (!ringing && e == EV_LONG && mode != M_EXAM && !(mode == M_TH && thSub != 0) && !(mode == M_ALARM && alarmSub != 0) && mode != M_SETCLOCK && !(mode == M_TALLY && tallySub != 0)) { backToMenu(); e = EV_NONE; }

  unsigned long period = (mode == M_CHRONO || mode == M_TALLY || mode == M_DIE || mode == M_CHESS) ? 100UL : 200UL;
  bool draw = (now - lastDraw >= period) || (e != EV_NONE) || dial;
  if (draw) lastDraw = now;

  if (ringing) { handleRing(e, draw); return; }

  switch (mode) {
    case M_EXAM:     drawExam(e, draw, tap); break;
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
