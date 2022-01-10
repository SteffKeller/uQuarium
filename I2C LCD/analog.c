////////////////////////////////////////////////////////////////////////
//
//
//Analog Digital wandler Atmega8 Port C
//
//
/////////////////////////////////////////////////////////////////////////
#include	<avr/io.h>		// AVR Register und Konstantendefinitionen
#include "analog.h"


/*------------------------------------------------------------------
 * adc_read_channel(..) -  ADC Wert vom AD-Wandler des Atemga644p holen hier 10Bit -> 0-1023;
 * PE:uint8_t mux // ADC Kanal 0-7
 * PA:uint16_t  //  Wert vom ADC 0-1023
 *///---------------------------------------------------------------
uint16_t adc_read_channel(uint8_t mux)
{

	uint8_t i;
	uint16_t result;
	
	ADCSRA |= (1<<ADEN); //ADC aktivieren
	ADMUX |= mux; // Kanal waehlen
	ADCSRA |= (1<<ADSC); // Dummy-Readout um den ADC "warmlaufen zu lassen" 
	while ( ADCSRA & (1<<ADSC) ); // auf Abschluss der Konvertierung warten
		
	result = ADCW; // ADCW muss gelesen werden, sonst wird Ergebnis der nächsten Wandlung nicht übernommen.
	
	result = 0; // Dummy-Readout verwerfen
	for( i=0; i<16; i++ ) // Eigentliche Messung - Mittelwert aus 8 aufeinanderfolgenden Wandlungen
	{
		ADCSRA |= (1<<ADSC); // eine Wandlung "single conversion"
		while ( ADCSRA & (1<<ADSC) ); // auf Abschluss der Konvertierung warten
		result += ADCW; // Wandlungsergebnisse aufaddieren
	}
	ADCSRA &= ~(1<<ADEN); // ADC deaktivieren 
	result /= 16; // Summe durch 16 teilen = arithm. Mittelwert
	return result;
}


/*------------------------------------------------------------------
 * adc_init(..) - AD Wandler des Atemga 644p initialisieren
 * PE:void
 * PA:void
 *///---------------------------------------------------------------
void adc_init(void)
{
	 
	// ADC-Frequenz muss zwischen 50k und 200kHz liegen ->Frequenzteiler auf 128 -> 16MHz/128 = ADC-Frequenz 125kHz
	ADCSRA  |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
	ADMUX |= (0<<REFS1) | (0<<REFS0); // externe Referenzspannung nutzen -> Referenz = 4.096V
}

/*------------------------------------------------------------------
 * phwert_kalibrieren(..) - Aus den ermittelten Referenzwerten die Funktionswerte zur berechung des PH-Werts bestimmen
 *							Lineare Funktoinsgleichung F(y) = m*x+q -> Ph-Wert = Steigung * ADC-Wert + Nullversatz
 *							Um die Umrechnungsfehler duch die Festkommaaritmetik möglichst klein zu halten wird mit 32 Bit Variablen gearbeitet
 *							Um den Realen PH Wert zu erhalten muss durch 10000 gerechnet werden PH 7.0 = 700000
 * PE:uint16_t * ph4 // ADC_Wert bei Referenzlösung Ph 4
 * PE:uint16_t * ph7 // ADC-Wert bei Referenzlösung Ph 7
 * PE:uint16_t * ph9 // ADc_Wert bei Referenzlösung Ph 9 
 * PE:Ph_reverenz * ph_werte // Struct mit den Referenzwerten zur PH berechung
 * PA:void
 *///---------------------------------------------------------------
void phwert_kalibrieren(uint16_t *ph4, uint16_t *ph7, uint16_t *ph9, Ph_reverenz *ph_werte)
{
	uint16_t delta_x47, delta_x79;  // Hilfsvariablen um die Steigung zu berechnen
	
	const uint32_t delta_y47 = 30000; // PH 7.0-4.0 = 3.0 entspricht 30000 Hilfsvariable für Steigung
	const uint32_t delta_y79 = 20000; // Ph 9.0-7.0 = 2.0 entspricht 20000 Hilfsvariable für Steigung
	
	unsigned char buffer[10];
	
	// Da der Funktionsverlauf der PH Sonde nicht absolut linear ist wird eine 3 Punkt kalibrierung durchgeführt
	// Dabei wird der Funktionsverlauf separat einmal für Ph-Wert kleiner 7 uns einmal für Ph_Wert grösser 7 berechnet
	
	// Funktionserte berechnen falls der Ph_wert höher als 7 ist
	delta_x79 = *ph9 - *ph7;  // Die Differzenz des ADC Wertes vom pH Wert 7 und 9 berechnen -> delta X für die Funktionsgleichung
	ph_werte->steigung_ph79 = delta_y79 / delta_x79; // Steigung berechnen -> m = delta_y / delta_x
	ph_werte->nullversatz_ph79 = 70000 - (ph_werte->steigung_ph79 * *ph7); // Nullversatz berechnen -> q = Y-Wert - ( Steigung * X-Wert)
	
	// Funktionserte berechnen falls der Ph_wert tiefer als 7 ist
	delta_x47 = *ph7 - *ph4;  // Die Differzenz des ADC Wertes vom pH Wert 4 und 7 berechnen -> delta X für die Funktionsgleichung
	ph_werte->steigung_ph47 = delta_y47 / delta_x47; // Steigung berechnen -> m = delta_y / delta_x
	ph_werte->nullversatz_ph47 = 40000 - (ph_werte->steigung_ph47 * *ph4); // Nullversatz berechnen -> q = Y-Wert - ( Steigung * X-Wert)
	
	ph_werte->adc_ref_ph4 = *ph4; // Die ADC Referenzwerte für PH 4,7, und 9 im struct speichern
	ph_werte->adc_ref_ph7 = *ph7;
	ph_werte->adc_ref_ph9 = *ph9;
	
}	
	 
	
	


/*------------------------------------------------------------------
 * read_phwert(..) - PH Werte holen -> Digitalwert vom ADC einlesen und in Ph-Wert umrechnen
 * PE:Ph_reverenz * ph_ref_werte // Struktur mit den Referenzwerte der Kalibirung
 * PA:uint32_t // PH-Wert als Festkommazahl Ph7.0 = 70000 Ph 4.0 = 40000
 *///---------------------------------------------------------------
uint16_t read_phwert(Ph_reverenz *ph_ref_werte )
{
	uint16_t adc_wert;
	
	adc_wert = adc_read_channel(PH_ADC_CHANNEL); // ADC-Wert vom AD Wandler holen
	
	if (adc_wert < ph_ref_werte->adc_ref_ph7) // Wenn Ph kleiner 7.0 -> Umrechnung mit Funktionswerten Ph kleiner 7.0
	{
		//Ph Wert errechnen mit Ph(ADC-Wert)= m*ADC-Wert+q und retournieren
		//Das Resultat wird noch durch 10 dividiert um den Ph_wert als 16 Bit Variable zu erhalten Ph 7.0 = 7000
		return ((ph_ref_werte->steigung_ph47 * adc_wert + ph_ref_werte->nullversatz_ph47) / 10);	
	}
	else // Wenn Ph grösser 7.0 -> Umrechnung mit Funktionswerten Ph grösser 7.0
	{
		//Ph Wert errechnen mit Ph(ADC-Wert)= m*ADC-Wert+q und retournieren
		//Das Resultat wird noch durch 10 dividiert um den Ph_wert als 16 Bit Variable zu erhalten Ph 7.0 = 7000
		return ((ph_ref_werte->steigung_ph79 * adc_wert + ph_ref_werte->nullversatz_ph79) / 10); 
	}
	
}


