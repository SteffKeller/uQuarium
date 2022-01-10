/************************************************************************
 * Titel :		main.h
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Headerdatei für main.c -> Deklarationen
 * Autor :		Stefan Keller
 * Lehrgang:	Techniker HF ET 08-11 Klasse A
 * Datum :		4.9.20111
 ************************************************************************/
#ifndef MAIN_H_
#define MAIN_H_
#endif /* MAIN_H_ */
/*************************************************************************
* Quarzfrequenz des Atemga einstellen
*************************************************************************/
#ifndef F_CPU
#define F_CPU 16000000UL
#endif
/*************************************************************************
* Includes 
*************************************************************************/
#include <avr/io.h>			// AVR Register und Konstantendefinitionen
#include <avr/interrupt.h>	// Interruptverarbeitung
#include <util/delay.h>		// Delays
#include <avr/pgmspace.h>	//Zugriff auf Flash
#include <avr/eeprom.h>		// EEPROM routinen
#include <avr/wdt.h>		// Watchdog
#include "DS1307.h"			// I2C Echtzeituhr
#include "LM76.h"			// I2C Temperatursensor
#include "hih6131.h"
#include "uart.h"			// UART Library
#include "i2clcd.h"			// Ansteuerung LCD Display über I2C
#include "taster_timer.h"	// Timer und Tasten entprellen
#include "lcdmenu.h"		// Menü für Einstellungen
#include "adc.h"			// ADC-Wandler und pH
#include "mem-check.h"
/*************************************************************************
* Makros
*************************************************************************/
#ifndef EEPROM
#define EEPROM  __attribute__ ((section (".eeprom"))) // EEPROM zugriff
#endif

// Alarmgeber, Buzzer	-> Port D Pin 5
#define BUZZER_ON	(PORTD |= 1<<PD5)
#define BUZZER_OFF	(PORTD &= ~(1<<PD5))
#define BUZZER_ZUSTAND (PIND & (1<<PIND5))

// Aquarium Lampe		->	Port A Pin 3
#define LAMPE_ON	(PORTA |= 1<<PA3)
#define LAMPE_OFF	(PORTA &= ~(1<<PA3))
#define LAMPE_ZUSTAND (PINA & (1<<PINA3))

// Heizstab				-> Port A Pin 2
#define HEIZER_ON	(PORTA |= 1<<PA2)
#define HEIZER_OFF	(PORTA &= ~(1<<PA2))
#define HEIZER_ZUSTAND (PINA & (1<<PINA2))

// Lüfter				-> Port A Pin 1
#define LUEFTER_ON	(PORTA |= 1<<PA1)
#define LUEFTER_OFF	(PORTA &= ~(1<<PA1))
#define LUEFTER_ZUSTAND (PINA & (1<<PINA1))

// Aquarium Pumpe		-> Port A Pin 0
#define PUMPE_ON	(PORTA |= 1<<PA0)
#define PUMPE_OFF	(PORTA &= ~(1<<PA0))
#define PUMPE_ZUSTAND (PINA & (1<<PINA0))

// LED für Alarm		-> Port C Pin 6
#define ALARMLED_ON	 (PORTC |= 1<<PC6)
#define ALARMLED_OFF (PORTC &= ~(1<<PC6))

// LED für Run Modus	-> Port X Pin 7
#define RUNLED_ON	(PORTC |= 1<<PC7)
#define RUNLED_OFF	(PORTC &= ~(1<<PC7))

/*************************************************************************
* Defines Konstanten
*************************************************************************/
#define UART_BAUD_RATE		 57600	// Baudrate für RS232 
#define LCDBACKLIGHT_ON_ZEIT 30		// Einschaltzeit des LCD Backlight (sec)
#define WARTEZEIT_PH_KALIBRIEREN 180	// Wartezeit bei pH Sonde kalibrieren (sec)

const uint8_t LM76_ADRESS[] = {0x90,0x92,0x94};
#define hih6131_adress  0x4e

/*************************************************************************
* Variablen
*************************************************************************/
int8_t temperatur_offset[] = {0,0,0}; 
uint16_t temperatur[] = {0,0,0}; 
uint16_t huminity= 0;
uint16_t hih_temperatur = 0;
int8_t mondlicht_helligkeit =0, regenzeitenanzahl= 0;
uint8_t alarm_vektor = 0;	//Vektor zur Alarmzustand
uint8_t MOON_ZUSTAND = 0;	// Mondlicht ein oder aus -> Da wegen PWM nicht direkt über Port abfragbar
uint8_t i;
volatile uint16_t mem;

uint8_t ph_wert = 0; 

char anzeigetext[42];	// Sting Array für Ausgabe auf LCD -> zwei Zeilen
char anzeigetext1[22];	// String Array für Ausgabe auf LCD -> eine Zeile
char anzeigetext2[22];	// String Array für Ausgabe auf LCD -> eine Zeile
const char wochentagname [8][3] PROGMEM= {"  ","Mo","Di","Mi","Do","Fr","Sa","So"}; // Wochentage im Flash
const char LM76_NAME [3][3] PROGMEM= {"B","M","H"};
// Alarm Strings im Flash werden über alarm_vektor aufgerufen	
const char alarm_strings [7][9] PROGMEM= {"Alles OK", // Vektor O -> kein Alarm
									"I2C ERR!",	// Vektor 1	-> Error I2C Übertragung
									"Temp ++!",	// Vektor 2 -> Temperatur zu hoch
									"Temp --!",	// Vektor 3 -> Temperatur zu niedrig
									"pH ++!"  ,	// Vektor 4 -> pH Wert zu hoch *wird nicht genutzt*
									"pH --!"  ,	// Vektor 5 -> pH Wert zu niedrig *wird nicht genutzt*
									"WDT ERR!"};// Vektor 6 -> Reset durch Watchdog

struct uhr 
			zeit,		// Aktuelle Uhrzeit
			lampe_on,	// Einschaltzeit der Aquarium Lampe
			lampe_off,	// Ausschaltzeit der Aquarium Lampe
			ml_on,		// Einschaltzeit des Mondlichts
			ml_off,		// Ausschaltzeit des Mondlichts
			regenpumpe_einzeit;// Pumpenstoppzeit für Fütterung

struct uhr regenzeit[5];

typedef struct schalt_werte // Schaltwerte für Temperatur
{
    int16_t max;		// Maximum Wert
    int16_t min;		// Minimum Wert
	int16_t hysterese;
} schalt_werte;
schalt_werte  alarm, heizer, luefter;

struct					//Zustandsbits für Ausgänge
{						// 0 -> aus, 1 -> ein
    unsigned heizer:1;	//Heizer 
    unsigned luefter:1; //Lüfter
    unsigned lampe:1;	// Aquarium Lampe
    unsigned mondlicht:1;// Mondlicht 
    unsigned run:1;		// Run oder Wartungsmode
	unsigned pumpe:1;	// Aquarium Pumpe

} zustandsbit;

typedef struct Extrem_Werte
{
	int16_t max_wert;	// Maximaler Tageswert
	int16_t min_wert;	// Minimaler Tageswert
	uhr max_zeitpunkt;	// Zeitpunkt bei neuem Max Wert
	uhr min_zeitpunkt;	// Zeitpunkt bei neuem Min Wert
	uint8_t lm76_nr_max;// LM76 Welcher die höchste Temperatur hatte
	uint8_t lm76_nr_min;// LM76 Welcher die kleinste Temperatur hatte
	
}Extrem_Werte;
Extrem_Werte extrem_temperatur, extrem_huminity;

/*************************************************************************
* Statische Variablem im EEPROM entsprechend der Variablen im RAM 
*************************************************************************/
int8_t			temperatur_offset_eeprom[3] EEPROM =	{0,0,0};
uint8_t			mondlicht_helligkeit_eeprom EEPROM =	0;
schalt_werte	alarm_eeprom EEPROM =					{395,370,0};
schalt_werte	heizer_eeprom EEPROM =					{395,370,0};
schalt_werte	luefter_eeprom EEPROM=					{395,370,0};
schalt_werte	ph_schalt_werte_eeprom EEPROM =			{542,542,0};
Ph_reverenz		phwert_referenzen_eeprom EEPROM =		{299,542,666,133,233,140,429463936};
uhr				lampe_on_eeprom EEPROM=					{0,0,12,0,0,0,0};
uhr				lampe_off_eeprom EEPROM =				{0,0,12,0,0,0,0};
uhr				ml_on_eeprom EEPROM=					{0,0,12,0,0,0,0};
uhr				ml_off_eeprom EEPROM=					{0,0,12,0,0,0,0};
uhr				regenpumpe_einzeit_eeprom EEPROM =		{0,1,0,0,0,0,0};
uhr				regenzeit_eeprom[5] EEPROM=				{{0,0,12,0,0,0,0},{0,0,12,0,0,0,0},{0,0,12,0,0,0,0},{0,0,12,0,0,0,0},{0,0,12,0,0,0,0}};
int8_t			regenzeitenanzahl_eeprom EEPROM =		0;

	
/*************************************************************************
* Deklarationen der Funktionen
*************************************************************************/

/*************************************************************************
* Main Funktion 
*************************************************************************/
int main(void);
/*************************************************************************
 * timer_init(..) - Timer 0 und Timer 1 des Atmega 644p initialisieren
 * PE:void
 * PA:void
 *************************************************************************/
void timer_init(void);
/*************************************************************************
 * init_ports(..) - Ports initialisieren -> Datenrichtung bestimmen und Pull-UPs einschalten
 * PA:void
 *************************************************************************/
void init_ports();
/*************************************************************************
 * temperatur_to_string(..) - Temperatur in einen String umwandeln
 * PE:uint32_t aktuelle_temperatur	// Temperatur
 * PE:char * temp_buffer			// Pointer auf Ausgabestring
 * PA:void
 *************************************************************************/
void temperatur_to_string(int16_t aktuelle_temperatur, char* temp_buffer);
/*************************************************************************
 * zeit_to_string(..) - Uhrzeit in String umwandeln
 * PE:struct uhr ausgabe_zeit	// Struct mit Uhrzeit
 * PE:char * temp_buffer		// Pointer auf Ausgabestring
 * PA:void
*************************************************************************/
void zeit_to_string( struct uhr ausgabe_zeit,char* temp_buffer);
/*************************************************************************
 * datum_to_string(..) - Datum in String umwandeln
 * PE:struct uhr ausgabe_datum	// Struct mit Datum 
 * PE:char * temp_buffer	 // Pointer auf Ausgabestring
 * PA:void
*************************************************************************/
void datum_to_string( struct uhr ausgabe_datum,char* temp_buffer);
/*************************************************************************
 * phwert_to_string(..) - PH-Wert in Form von 7000 in Kommazahl 7.0 umwandeln für die LCD Ausgabe
 * PE:uint16_t ph_wert // Aktueller Ph-Wert 7000 entspricht Ph 7.0 
 * PE:char * temp_buffer // Pointer auf Ausgabestring
 * PA:void
 *************************************************************************/
void phwert_to_string( uint16_t ph_wert, char* temp_buffer);
/*************************************************************************
 * menue_zeiten_einstellen(..) - Uhr und Schaltzeiten einstellen und speichern
 * PE:struct uhr * zeit_pointer
 * PA:void
 *************************************************************************/
void menue_zeiten_einstellen(struct uhr *zeit_pointer, struct uhr *eeprom_write);
/*************************************************************************
 * menue_datum_einstellen(..) - Datum einstellen und speichern
 * PE:struct uhr * datum_pointer
 * PA:void
 *************************************************************************/
void menue_datum_einstellen( struct uhr *datum_pointer);
/*************************************************************************
 * menue_temperatur_offset(..) - Temperatur Offset des LM76 einstellen
 * PE:int8_t * offset - Temperatur Versatz in +-0.1 Grad
 * PA:void
 *************************************************************************/
void menue_temperatur_offset(int8_t *offset,uint8_t lm76_addr,uint8_t lm76_nr, int8_t *eeprom_addr);
/*************************************************************************
 * menue_schalttemperaturen_einstellen(..) -  Menü um die Schalttemperaturen vom Heizer, Lüfter und Alarmschwelle zu ändern
 * PE:unsigned char * text1 // Text der an LCD Position (2,1) angezeigt wird
 * PE:unsigned char * text2 // Text der an LCD Position (3,1) angezeigt wird
 * PE:unsigned char * text3 // Text der an LCD Position (4,1) angezeigt wird
 * PE:schalt_temperaturen * schalt_aktuell // Schalttemperatur die geändert werden soll
 * PE:schalt_temperaturen * eeprom // EEPROM Speicherplatz der Schalttemperatur
 * PA:void
 *************************************************************************/
void menue_schalttemperaturen_einstellen( unsigned char *text1, unsigned char *text2, unsigned char *text3, schalt_werte *schalt_aktuell,schalt_werte *eeprom);
/*************************************************************************
 * menu_mondlicht_helligkeit_einstellen(..) - Menü um die Helligkeit des Mondlicht LED einzustellen
 * Die PWM Duty Cycle kann von 0-100% einstellt werden
 * PE:uint8_t * helligkeit_pointer // Pointer auf Wert der Helligkeit 0 -100
 * PE:uint8_t * eeprom  // // EEPROM Speicherplatz der Helligkeit
 * PA:void
 *************************************************************************/
void menu_mondlicht_helligkeit_einstellen( uint8_t * helligkeit_pointer, uint8_t *eeprom);
/*************************************************************************
 * menu_phwert_klaibrieren(..) - Menü um die Sonde für den pH Wert zu kalibrieren
 * Die Sonde wird in die einzelnen Referenz Lösungen getaucht und dann 1 Min gewartet um 
 * einen stabilen Wert zu bekommen, dann wird der ADC Wert ausgelesen. 
 * Um Schluss werden die Funktionen zur Kalibrierung des pH Wertes berechnet
 * PA:void
 *************************************************************************/
void menu_phwert_kalibrieren(void);

/*************************************************************************
* Da den Funktionspointern im Menü keine Parameter mitgegeben werden können
* werden über das Menü die "funkt_" Funktionen aufgerufen und so die eigentlichen 
* Menüfunktionen mit Parameterübergabe aufgerufen
*************************************************************************/
/*************************************************************************
 * funkt_menu_temperatur_offet(..) - Funktionsmenü zur Einstellung des
 * Temperatur Offsets des LM 76
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_temperatur_offet(void);
/*************************************************************************
 * funkt_menu_heizer(..) - Funktionsmenü zur Einstellung der Schalttemperaturen
 * des Heizstabes
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_heizer(void);
/*************************************************************************
 * funkt_menu_luefter(..) - Funktionsmenü zur Einstellung der Schalttemperaturen
 * des Lüfters
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_luefter(void);
/*************************************************************************
 * funkt_menu_alarm(..) - Funktionsmenü zur Einstellung der maximalen und
 * minimalen Temperatur im Aquarium
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_alarm(void);
/*************************************************************************
 * funkt_menu_datum(..) - Funktionsmenü zur Einstellung des aktuellen
 * Datums des DS 1307
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_datum(void);
/*************************************************************************
 * funkt_menu_uhr(..) - Funktionsmenü zur Einstellung der aktuellen 
 * Uhrzeit des DS 1307
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_uhr(void);
/*************************************************************************
 * funkt_menu_lampe_ein(..) - Funktionsmenü zur Einstellung der 
 * Einschaltzeit für die Lampe
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_lampe_ein(void);
/*************************************************************************
 * funkt_menu_lampe_aus(..) - Funktionsmenü zur Einstellung der 
 * Ausschaltzeit für die Lampe
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_lampe_aus(void);
/*************************************************************************
 * funkt_menu_regenpumpe_einzeit(..) - Funktionsmenü zur Einstellung der 
 * Zeit wie lange die Pumpe beim Fütterungsstopp ausgeschaltet werden soll
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_regenpumpe_einzeit(void);
/*************************************************************************
 * funkt_menu_regenpumpe_einzeit(..) - Funktionsmenü zur Einstellung der 
 * Zeit wie lange die Pumpe beim Fütterungsstopp ausgeschaltet werden soll
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_regenzeit1(void);
/*************************************************************************
 * funkt_menu_regenpumpe_einzeit(..) - Funktionsmenü zur Einstellung der 
 * Zeit wie lange die Pumpe beim Fütterungsstopp ausgeschaltet werden soll
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_regenzeit2(void);
/*************************************************************************
 * funkt_menu_regenpumpe_einzeit(..) - Funktionsmenü zur Einstellung der 
 * Zeit wie lange die Pumpe beim Fütterungsstopp ausgeschaltet werden soll
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_regenzeit3(void);
/*************************************************************************
 * funkt_menu_regenpumpe_einzeit(..) - Funktionsmenü zur Einstellung der 
 * Zeit wie lange die Pumpe beim Fütterungsstopp ausgeschaltet werden soll
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_regenzeit4(void);
/*************************************************************************
 * funkt_menu_regenpumpe_einzeit(..) - Funktionsmenü zur Einstellung der 
 * Zeit wie lange die Pumpe beim Fütterungsstopp ausgeschaltet werden soll
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_regenzeit5(void);
/*************************************************************************
 * funkt_menu_mondlicht_ein(..) - Funktionsmenü zur Einstellung der 
 * Einschaltzeit des Mondlichts
 * PE:void
 * PA:void
 */
 void funkt_menu_regenzeitzaehler(void);
/*************************************************************************
 * funkt_menu_mondlicht_ein(..) - Funktionsmenü zur Einstellung der 
 * Einschaltzeit des Mondlichts
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_mondlicht_ein(void);
/*************************************************************************
 * funkt_menu_mondlicht_aus(..) - Funktionsmenü zur Einstellung
 * der Ausschaltzeit des Mondlichts
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_mondlicht_aus(void);
/*************************************************************************
 * funkt_menu_mondlicht_helligkeit(..) - Funktionsmenü für die Dimmung 
 * des Mondlichts
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_mondlicht_helligkeit(void);
/*************************************************************************
 * funkt_menu_phwert_kalibrieren(..) - Funktionsmenü für die Kalibrierung der
 * pH Sonde
 * PE:void
 * PA:void
 *************************************************************************/
void funkt_menu_phwert_kalibrieren(void);
/*************************************************************************
 * funkt_menu_about(..) - Abouttext für Hard und Softwareversionen anzeigen
 * PE:void
 * PA:void
 *************************************************************************/
void funkt_menu_about(void);
/*************************************************************************
 * funkt_menu_max_werte(..) - Menüanzeige der Tages Maximalwerte von Temperatur und pH Wert
 * PE:void
 * PA:void
 *************************************************************************/
void funkt_menu_max_werte(void);
/*************************************************************************
 * vergleiche_temperaturen(..) -  Temperatur mit mit min/max Werten vergleichen und Ergebnis zurückgeben
 * PE:uint32_t * minimum // Minimum Temperatur mit der verglichen wird
 * PE:uint32_t * maximum // Maximum Temperatur mit der verglichen wird
 * PE:uint32_t * aktuelle_temperatur // Aktuelle Temperatur des Wassers mit der verglichen wird
 * PA:uint8_t // Ergebnis zurückgeben 1 -> höher als max 2 -> kleiner als min, 0 -> Ergebnis zwischen den Temperaturen
 *************************************************************************/
uint8_t vergleiche_temperaturen(int16_t *minimum, int16_t *maximum, int16_t *aktuelle_temperatur);
/*************************************************************************
 * vergleiche_zeiten(..) - Vergleiche die Schaltzeiten der Ausgänge mit der aktuellen Zeit und gibt das Ergebnis zurück
 * PE:struct uhr * ein_zeit // Pointer auf struct der Einschaltzeit
 * PE:struct uhr * aus_zeit // Pointer auf struct der Ausschaltzeit
 * PE:struct uhr * aktuelle_zeit // Pointer auf struct der aktuellen Zeit
 * PA:uint8_t // 0 -> Aktuelle Zeit liegt ausserhalb den Schaltzeiten -> aus, 1 -> Aktuelle Zeit liegt zwischen den Schaltzeiten -> ein
 *************************************************************************/
uint8_t vergleiche_zeiten(struct uhr *ein_zeit, struct uhr *aus_zeit, struct uhr *aktuelle_zeit);
/*************************************************************************
 * lampen_schalten(..) - Zustandsbit für die zeitgesteuerten 
 * Ausgänge setzen
 * PE:void
 * PA:void
*************************************************************************/
void lampen_schalten(void);
/*************************************************************************
 * temperatuen_schalten(..) - Zustandbits für die temperaturgesteuerten
 * Ausgänge und Alarmzustand setzen
 * PE:void
 * PA:void
 *************************************************************************/
void temperatuen_schalten(void);
/*************************************************************************
 * ausgaenge_ansteuern(..) - Errechnete Schaltzustände sammeln und 
 * die Hardwareausgänge entsprechen dem Modus der Anlage schalten
 * PE:void
 * PA:void
 *************************************************************************/
void ausgaenge_ansteuern( void );
/*************************************************************************
 * regenpumpe_aktivieren(..) -  Für Pumpenstopp bei der Fütterung wird ein Counter geladen 
 * welcher über den Timer alle 1 sec dekrementiert wird bis Zeit abgelaufen ist
 * PE:struct uhr * stopp_zeit // Pointer auf Zeit wie lange die Pumpe ausgeschaltet werden soll
 * PA:void
 *************************************************************************/
void regenpumpe_aktivieren(struct uhr *stopp_zeit);
/*************************************************************************
 * read_eeprom_daten(..) - Statische Werte nach einen Reset aus dem EEPROM holen
 * PA:void
 *************************************************************************/
void read_eeprom_daten(void);
/*************************************************************************
 * zustand_lcd(..) - Zeichen generieren um die Schaltzustände der Ausgänge auf den Display anzuzeigen
 * PE:uint8_t zustand // 1 = Ein , 0 = Aus
 * PA:uint8_t			// Hex Zeichen retournieren Ein=^  Aus=_
 *************************************************************************/
uint8_t zustand_lcd(uint8_t zustand);
/*************************************************************************
 * send_to_uart(..) - Messwerte über RS 232 an den PC senden
 * PE:void
 * PA:void
 *************************************************************************/
void send_to_uart(void );
/*************************************************************************
 * mondlicht_dimmer(..) - Helligkeit des Mondlichts mittels PWM Timer 1B einstellen
 * PE:uint8_t  helligkeit // Helligkeit in Prozent 0=aus, 50=50%, 100=100%
 * PA:void
 *************************************************************************/
void mondlicht_dimmer(uint8_t helligkeit);
/*************************************************************************
 * reset_tageswerte(..) - Um Mitternacht werden die Tages Min/Max Werte zurückgesetzt
 * PE:void
 * PA:void
 *************************************************************************/
void reset_tageswerte(void);
/*************************************************************************
 * main_anzeige(..) - Hauptanzeige Welche die aktuellen Informationen 
 * auf den LCD anzeigt
 * PE:void
 * PA:void
 *************************************************************************/
void main_anzeige(void);