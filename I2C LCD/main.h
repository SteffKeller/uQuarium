/************************************************************************
 * Titel :		main.h
 * Projekt:		Diplomarbeit Aquariumsteuerung �Quarium
 * Funktion:	Headerdatei f�r main.c -> Deklarationen
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
//#include "uart.h"			// UART Library
#include "i2clcd.h"			// Ansteuerung LCD Display �ber I2C
#include "taster_timer.h"	// Timer und Tasten entprellen
#include "lcdmenu.h"		// Men� f�r Einstellungen
#include "adc.h"			// ADC-Wandler und pH
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

// L�fter				-> Port A Pin 1
#define LUEFTER_ON	(PORTA |= 1<<PA1)
#define LUEFTER_OFF	(PORTA &= ~(1<<PA1))
#define LUEFTER_ZUSTAND (PINA & (1<<PINA1))

// Aquarium Pumpe		-> Port A Pin 0
#define OUT_ON	(PORTA |= 1<<PA0)
#define OUT_OFF	(PORTA &= ~(1<<PA0))
#define OUT_ZUSTAND (PINA & (1<<PINA0))

// LED f�r Alarm		-> Port C Pin 6
#define ALARMLED_ON	(PORTC |= 1<<PC6)
#define ALARMLED_OFF (PORTC &= ~(1<<PC6))

// LED f�r Run Modus	-> Port X Pin 7
#define RUNLED_ON	(PORTC |= 1<<PC7)
#define RUNLED_OFF	(PORTC &= ~(1<<PC7))

/*************************************************************************
* Defines Konstanten
*************************************************************************/
#define UART_BAUD_RATE		 57600	// Baudrate f�r RS232 
#define LCDBACKLIGHT_ON_ZEIT 30		// Einschaltzeit des LCD Backlight (sec)
#define WARTEZEIT_PH_KALIBRIEREN 180	// Wartezeit bei pH Sonde kalibrieren (sec)

/*************************************************************************
* Variablen
*************************************************************************/
int8_t temperatur_offset = 0, mondlicht_helligkeit =0;
uint8_t alarm_vektor = 0;	//Vektor zur Alarmzustand
uint8_t MOON_ZUSTAND = 0;	// Mondlicht ein oder aus -> Da wegen PWM nicht direkt �ber Port abfragbar
uint16_t temperatur = 0, ph_wert = 0; 

char anzeigetext[41];	// Sting Array f�r Ausgabe auf LCD -> zwei Zeilen
char anzeigetext1[21];	// String Array f�r Ausgabe auf LCD -> eine Zeile
char anzeigetext2[21];	// String Array f�r Ausgabe auf LCD -> eine Zeile
const char wochentagname [8][3] PROGMEM= {"  ","Mo","Di","Mi","Do","Fr","Sa","So"}; // Wochentage im Flash
// Alarm Strings im Flash werden �ber alarm_vektor aufgerufen	
const char alarm_strings [7][9] PROGMEM= {"Alles OK", // Vektor O -> kein Alarm
									"I2C ERR!",	// Vektor 1	-> Error I2C �bertragung
									"Temp ++!",	// Vektor 2 -> Temperatur zu hoch
									"Temp --!",	// Vektor 3 -> Temperatur zu niedrig
									"pH ++!"  ,	// Vektor 4 -> pH Wert zu hoch *wird nicht genutzt*
									"pH --!"  ,	// Vektor 5 -> pH Wert zu niedrig *wird nicht genutzt*
									"WDT ERR!"};// Vektor 6 -> Reset durch Watchdog

struct uhr 
			zeit,		// Aktuelle Uhrzeit
			lampe_on1,	// Einschaltzeit der Aquarium Lampe
			lampe_off1,	// Ausschaltzeit der Aquarium Lampe
			ml_on,		// Einschaltzeit des Mondlichts
			ml_off,		// Ausschaltzeit des Mondlichts
			futterstopp,// Pumpenstoppzeit f�r F�tterung
			lampe_on2,	// Einschaltzeit der Aquarium Lampe
			lampe_off2,	// Ausschaltzeit der Aquarium Lampe
			out_on1,	// pumpe / out zeit 1
			out_off1,
			out_on2,	// pumpe / out zeit 2
			out_off2;
			

typedef struct schalt_werte // Schaltwerte f�r Temperatur
{
    int16_t max;		// Maximum Wert
    int16_t min;		// Minimum Wert
	int16_t hysterese;
} schalt_werte;
schalt_werte  alarm, heizer, luefter;

struct					//Zustandsbits f�r Ausg�nge
{						// 0 -> aus, 1 -> ein
    unsigned heizer:1;	//Heizer 
    unsigned luefter:1; //L�fter
    unsigned lampe:1;	// Aquarium Lampe
    unsigned mondlicht:1;// Mondlicht 
    unsigned run:1;		// Run oder Wartungsmode
	unsigned pumpe:1;	// Aquarium Pumpe
	unsigned esc_alarm:1;	// Aquarium Pumpe

} zustandsbit;

typedef struct Extrem_Werte
{
	int16_t max_wert;	// Maximaler Tageswert
	int16_t min_wert;	// Minimaler Tageswert
	uhr max_zeitpunkt;	// Zeitpunkt bei neuem Max Wert
	uhr min_zeitpunkt;	// Zeitpunkt bei neuem Min Wert
	
}Extrem_Werte;
Extrem_Werte extrem_temperatur, extrem_ph;

/*************************************************************************
* Statische Variablem im EEPROM entsprechend der Variablen im RAM 
*************************************************************************/
uhr				out_on_eeprom1 EEPROM=					{0,0,14,0,0,0,0};
uhr				out_off_eeprom1 EEPROM =				{0,0,16,0,0,0,0};
uhr				out_on_eeprom2 EEPROM=					{0,0,23,0,0,0,0};
uhr				out_off_eeprom2 EEPROM =				{0,0,10,0,0,0,0};
int8_t			temperatur_offset_eeprom EEPROM =		0;
uint8_t			mondlicht_helligkeit_eeprom EEPROM =	0;
schalt_werte	alarm_eeprom EEPROM =					{395,370,0};
schalt_werte	heizer_eeprom EEPROM =					{395,370,0};
schalt_werte	luefter_eeprom EEPROM=					{395,370,0};
schalt_werte	ph_schalt_werte_eeprom EEPROM =			{542,542,0};
Ph_reverenz		phwert_referenzen_eeprom EEPROM =		{299,542,666,133,233,140,429463936};
uhr				lampe_on_eeprom1 EEPROM=				{0,0,10,0,0,0,0};
uhr				lampe_off_eeprom1 EEPROM =				{0,0,14,0,0,0,0};
uhr				ml_on_eeprom EEPROM=					{29,00,22,0,0,0,0};
uhr				ml_off_eeprom EEPROM=					{59,0,23,0,0,0,0};
uhr				futterstopp_eeprom EEPROM =				{0,10,0,0,0,0,0};
uhr				zeit_eeprom EEPROM =					{0,1,0,0,0,0,0};
uhr				lampe_on_eeprom2 EEPROM=				{0,0,17,0,0,0,0};
uhr				lampe_off_eeprom2 EEPROM =				{30,0,22,0,0,0,0};
uhr				dummy_zeit_eeprom EEPROM =				{0,0,0,0,0,0,0};

	
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
 * phwert_to_string(..) - PH-Wert in Form von 7000 in Kommazahl 7.0 umwandeln f�r die LCD Ausgabe
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
void menue_temperatur_offset( int8_t *offset);
/*************************************************************************
 * menue_schalttemperaturen_einstellen(..) -  Men� um die Schalttemperaturen vom Heizer, L�fter und Alarmschwelle zu �ndern
 * PE:unsigned char * text1 // Text der an LCD Position (2,1) angezeigt wird
 * PE:unsigned char * text2 // Text der an LCD Position (3,1) angezeigt wird
 * PE:unsigned char * text3 // Text der an LCD Position (4,1) angezeigt wird
 * PE:schalt_temperaturen * schalt_aktuell // Schalttemperatur die ge�ndert werden soll
 * PE:schalt_temperaturen * eeprom // EEPROM Speicherplatz der Schalttemperatur
 * PA:void
 *************************************************************************/
void menue_schalttemperaturen_einstellen( unsigned char *text1, unsigned char *text2, unsigned char *text3, schalt_werte *schalt_aktuell,schalt_werte *eeprom);
/*************************************************************************
 * menu_mondlicht_helligkeit_einstellen(..) - Men� um die Helligkeit des Mondlicht LED einzustellen
 * Die PWM Duty Cycle kann von 0-100% einstellt werden
 * PE:uint8_t * helligkeit_pointer // Pointer auf Wert der Helligkeit 0 -100
 * PE:uint8_t * eeprom  // // EEPROM Speicherplatz der Helligkeit
 * PA:void
 *************************************************************************/
void menu_mondlicht_helligkeit_einstellen( uint8_t * helligkeit_pointer, uint8_t *eeprom);
/*************************************************************************
 * menu_phwert_klaibrieren(..) - Men� um die Sonde f�r den pH Wert zu kalibrieren
 * Die Sonde wird in die einzelnen Referenz L�sungen getaucht und dann 1 Min gewartet um 
 * einen stabilen Wert zu bekommen, dann wird der ADC Wert ausgelesen. 
 * Um Schluss werden die Funktionen zur Kalibrierung des pH Wertes berechnet
 * PA:void
 *************************************************************************/
void menu_phwert_kalibrieren(void);

/*************************************************************************
* Da den Funktionspointern im Men� keine Parameter mitgegeben werden k�nnen
* werden �ber das Men� die "funkt_" Funktionen aufgerufen und so die eigentlichen 
* Men�funktionen mit Parameter�bergabe aufgerufen
*************************************************************************/
/*************************************************************************
 * funkt_menu_temperatur_offet(..) - Funktionsmen� zur Einstellung des
 * Temperatur Offsets des LM 76
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_temperatur_offet(void);
/*************************************************************************
 * funkt_menu_heizer(..) - Funktionsmen� zur Einstellung der Schalttemperaturen
 * des Heizstabes
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_heizer(void);
/*************************************************************************
 * funkt_menu_luefter(..) - Funktionsmen� zur Einstellung der Schalttemperaturen
 * des L�fters
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_luefter(void);
/*************************************************************************
 * funkt_menu_alarm(..) - Funktionsmen� zur Einstellung der maximalen und
 * minimalen Temperatur im Aquarium
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_alarm(void);
/*************************************************************************
 * funkt_menu_datum(..) - Funktionsmen� zur Einstellung des aktuellen
 * Datums des DS 1307
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_datum(void);
/*************************************************************************
 * funkt_menu_uhr(..) - Funktionsmen� zur Einstellung der aktuellen 
 * Uhrzeit des DS 1307
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_uhr(void);
/*************************************************************************
 * funkt_menu_lampe_ein_am(..) - Funktionsmen� zur Einstellung der 
 * Einschaltzeit f�r die Lampe
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_lampe_ein_am(void);
/*************************************************************************
 * funkt_menu_lampe_aus_am(..) - Funktionsmen� zur Einstellung der 
 * Ausschaltzeit f�r die Lampe
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_lampe_aus_am(void);
/*************************************************************************
 * funkt_menu_lampe_ein_pm(..) - Funktionsmen� zur Einstellung der 
 * Einschaltzeit f�r die Lampe
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_lampe_ein_pm(void);
/*************************************************************************
 * funkt_menu_lampe_aus_pm(..) - Funktionsmen� zur Einstellung der 
 * Ausschaltzeit f�r die Lampe
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_lampe_aus_pm(void);
/*************************************************************************
 * funkt_menu_futterstop(..) - Funktionsmen� zur Einstellung der 
 * Zeit wie lange die Pumpe beim F�tterungsstopp ausgeschaltet werden soll
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_futterstop(void);
/*************************************************************************
 * funkt_menu_mondlicht_ein(..) - Funktionsmen� zur Einstellung der 
 * Einschaltzeit des Mondlichts
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_mondlicht_ein(void);
/*************************************************************************
 * funkt_menu_mondlicht_aus(..) - Funktionsmen� zur Einstellung
 * der Ausschaltzeit des Mondlichts
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_mondlicht_aus(void);
/*************************************************************************
 * funkt_menu_mondlicht_helligkeit(..) - Funktionsmen� f�r die Dimmung 
 * des Mondlichts
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_mondlicht_helligkeit(void);
/*************************************************************************
 * funkt_menu_phwert_kalibrieren(..) - Funktionsmen� f�r die Kalibrierung der
 * pH Sonde
 * PE:void
 * PA:void
 *************************************************************************/
void funkt_menu_phwert_kalibrieren(void);
/*************************************************************************
 * funkt_menu_about(..) - Abouttext f�r Hard und Softwareversionen anzeigen
 * PE:void
 * PA:void
 *************************************************************************/
void funkt_menu_about(void);
/*************************************************************************
 * funkt_menu_max_werte(..) - Men�anzeige der Tages Maximalwerte von Temperatur und pH Wert
 * PE:void
 * PA:void
 *************************************************************************/
void funkt_menu_max_werte(void);
/*************************************************************************
 * vergleiche_temperaturen(..) -  Temperatur mit mit min/max Werten vergleichen und Ergebnis zur�ckgeben
 * PE:uint32_t * minimum // Minimum Temperatur mit der verglichen wird
 * PE:uint32_t * maximum // Maximum Temperatur mit der verglichen wird
 * PE:uint32_t * aktuelle_temperatur // Aktuelle Temperatur des Wassers mit der verglichen wird
 * PA:uint8_t // Ergebnis zur�ckgeben 1 -> h�her als max 2 -> kleiner als min, 0 -> Ergebnis zwischen den Temperaturen
 *************************************************************************/
uint8_t vergleiche_temperaturen(int16_t *minimum, int16_t *maximum, int16_t *aktuelle_temperatur);
/*************************************************************************
 * vergleiche_zeiten(..) - Vergleiche die Schaltzeiten der Ausg�nge mit der aktuellen Zeit und gibt das Ergebnis zur�ck
 * PE:struct uhr * ein_zeit // Pointer auf struct der Einschaltzeit
 * PE:struct uhr * aus_zeit // Pointer auf struct der Ausschaltzeit
 * PE:struct uhr * aktuelle_zeit // Pointer auf struct der aktuellen Zeit
 * PA:uint8_t // 0 -> Aktuelle Zeit liegt ausserhalb den Schaltzeiten -> aus, 1 -> Aktuelle Zeit liegt zwischen den Schaltzeiten -> ein
 *************************************************************************/
uint8_t vergleiche_zeiten(struct uhr *ein_zeit, struct uhr *aus_zeit, struct uhr *aktuelle_zeit);
/*************************************************************************
 * lampen_schalten(..) - Zustandsbit f�r die zeitgesteuerten 
 * Ausg�nge setzen
 * PE:void
 * PA:void
*************************************************************************/
void lampen_schalten(void);
/*************************************************************************
 * temperatuen_schalten(..) - Zustandbits f�r die temperaturgesteuerten
 * Ausg�nge und Alarmzustand setzen
 * PE:void
 * PA:void
 *************************************************************************/
void temperatuen_schalten(void);
/*************************************************************************
 * ausgaenge_ansteuern(..) - Errechnete Schaltzust�nde sammeln und 
 * die Hardwareausg�nge entsprechen dem Modus der Anlage schalten
 * PE:void
 * PA:void
 *************************************************************************/
void ausgaenge_ansteuern( void );
/*************************************************************************
 * futterstop_aktivieren(..) -  F�r Pumpenstopp bei der F�tterung wird ein Counter geladen 
 * welcher �ber den Timer alle 1 sec dekrementiert wird bis Zeit abgelaufen ist
 * PE:struct uhr * stopp_zeit // Pointer auf Zeit wie lange die Pumpe ausgeschaltet werden soll
 * PA:void
 *************************************************************************/
void futterstop_aktivieren(struct uhr *stopp_zeit);
/*************************************************************************
 * read_eeprom_daten(..) - Statische Werte nach einen Reset aus dem EEPROM holen
 * PA:void
 *************************************************************************/
void read_eeprom_daten(void);
/*************************************************************************
 * zustand_lcd(..) - Zeichen generieren um die Schaltzust�nde der Ausg�nge auf den Display anzuzeigen
 * PE:uint8_t zustand // 1 = Ein , 0 = Aus
 * PA:uint8_t			// Hex Zeichen retournieren Ein=^  Aus=_
 *************************************************************************/
uint8_t zustand_lcd(uint8_t zustand);
/*************************************************************************
 * send_to_uart(..) - Messwerte �ber RS 232 an den PC senden
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
 * reset_tageswerte(..) - Um Mitternacht werden die Tages Min/Max Werte zur�ckgesetzt
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