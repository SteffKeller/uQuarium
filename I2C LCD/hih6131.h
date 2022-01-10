/************************************************************************
 * Titel :		hih6131.h
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Header für Library zur Ansteuerung des LM76 Temperatursensors
 * Autor :		Stefan Keller
 * Lehrgang:	Techniker HF ET 08-11 Klasse A
 * Datum :		18.9.20111
 ************************************************************************/
//
#ifndef _HIH6131_H_
#define _HIH6131_H_
#endif
/*************************************************************************
* Quarzfrequenz des Atemga einstellen
*************************************************************************/
#ifndef F_CPU
#define F_CPU 16000000UL
#endif
/*************************************************************************
* Includes 
*************************************************************************/
#include "i2cmaster.h"
#include <util/delay.h>		// Delays

/*************************************************************************
* Defines Konstanten
*************************************************************************/
//#define LM76_1_ADRESSE 0x90 // Die I2C Adresse des LM 76, hier 0b1001'0000 da beide Adresspins(A0,A1) auf 0 gezogen sind
//#define LM76_2_ADRESSE 0x92 // Die I2C Adresse des LM 76, hier 0b1001'0010 da beide Adresspins(A0,A1) auf 0 gezogen sind
//#define LM76_3_ADRESSE 0x94 // Die I2C Adresse des LM 76, hier 0b1001'0100 da beide Adresspins(A0,A1) auf 0 gezogen sind
//

/*************************************************************************
* Variablen
*************************************************************************/
extern uint8_t alarm_vektor;

/*************************************************************************
* Deklarationen der Funktionen
*************************************************************************/
/*************************************************************************
 * get_lm76_temperatur(..) - Temperatur Bytes vom LM76 holen und zusammensetzen
 * PE:uint8_t adresse // I2C Adresse LM 76
 * PA:int16_t		  // Temperatur als int Variable, 1 Bit entspricht 0.0625°C
*************************************************************************/
void get_hih6131_data( uint8_t adresse, uint16_t *huminity_local, int16_t *temperatur_local );


