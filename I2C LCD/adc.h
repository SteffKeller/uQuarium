/************************************************************************
 * Titel :		adc.c
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Header für Library Analog Digital Converter 
 * Autor :		Stefan Keller
 * Lehrgang:	Techniker HF ET 08-11 Klasse A
 * Datum :		4.9.20111
 ************************************************************************/
/*************************************************************************
* Includes 
*************************************************************************/
#include	<avr/io.h>		// AVR Register und Konstantendefinitionen
/*************************************************************************
* Defines Konstanten
*************************************************************************/
#define PH_ADC_CHANNEL 4	// Anschluss pH Atmega ADC Kanal
/*************************************************************************
* Variablen
*************************************************************************/
typedef struct Ph_reverenz
{
	uint16_t adc_ref_ph4;		//Referenzwert ADC bei pH 4.0
	uint16_t adc_ref_ph7;		//Referenzwert ADC bei pH 7.0
	uint16_t adc_ref_ph9;		//Referenzwert ADC bei pH 9.0
	uint32_t steigung_ph47;		// Steigung bei pH > 7.0
	uint32_t nullversatz_ph47;	// Nullversatz bei pH > 7.0
	uint32_t steigung_ph79;		// Steigung bei pH < 7.0
	uint32_t nullversatz_ph79;	// Nullversatz bei pH > 7.0
	
}Ph_reverenz;
Ph_reverenz phwert_referenzen;	// pH Referenzwerte
/*************************************************************************
* Deklarationen der Funktionen
*************************************************************************/
/************************************************************************
 * adc_init(..) - AD Wandler des Atemga 644p initialisieren
 * PE:void
 * PA:void
************************************************************************/
void adc_init(void);
/************************************************************************
 * adc_read_channel(..) -  ADC Wert vom AD-Wandler des Atemga644p holen hier 10Bit -> 0-1023;
 * PE:uint8_t mux // ADC Kanal 0-7
 * PA:uint16_t  //  Wert vom ADC 0-1023
************************************************************************/
uint16_t adc_read_channel(uint8_t mux);
/************************************************************************
 * phwert_kalibrieren(..) - Aus den ermittelten Referenzwerten die Funktionswerte zur Berechnung des PH-Werts bestimmen
 * Lineare Funktionsgleichung F(y) = m*x+q -> Ph-Wert = Steigung * ADC-Wert + Nullversatz
 * Um die Umrechnungsfehler durch die Festkommaarithmetik möglichst klein zu halten wird mit 32 Bit Variablen gearbeitet
 * Um den Realen PH Wert zu erhalten muss durch 10000 gerechnet werden PH 7.0 = 700000
 * PE:uint16_t * ph4 // ADC_Wert bei Referenzlösung Ph 4
 * PE:uint16_t * ph7 // ADC-Wert bei Referenzlösung Ph 7
 * PE:uint16_t * ph9 // ADc_Wert bei Referenzlösung Ph 9 
 * PE:Ph_reverenz * ph_werte // Struct mit den Referenzwerten zur PH Berechnung
 * PA:void
************************************************************************/
void phwert_kalibrieren(uint16_t *ph4, uint16_t *ph7, uint16_t *ph9, Ph_reverenz *ph_werte);
/************************************************************************
 * read_phwert(..) - PH Werte holen -> Digitalwert vom ADC einlesen und in Ph-Wert umrechnen
 * PE:Ph_reverenz * ph_ref_werte // Struktur mit den Referenzwerte der Kalibrierung
 * PA:uint32_t // PH-Wert als Festkommazahl Ph7.0 = 70000 Ph 4.0 = 40000
************************************************************************/
uint16_t read_phwert(Ph_reverenz *ph_ref_werte);
