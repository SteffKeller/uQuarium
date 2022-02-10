/************************************************************************
 * Titel :		LM76.h
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Header für Library zur Ansteuerung des LM76 Temperatursensors
 * Autor :		Stefan Keller
 * Lehrgang:	Techniker HF ET 08-11 Klasse A
 * Datum :		18.9.20111
 ************************************************************************/
//
#ifndef _LM76_H_
#define _LM76_H_
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

/*************************************************************************
* Defines Konstanten
*************************************************************************/
#define LM76_ADRESSE 0x90 // Die I2C Adresse des LM 76, hier 0b1001'0000 da beide Adresspins(A0,A1) auf 0 gezogen sind
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
int16_t get_lm76_temperatur( uint8_t adresse);
/*************************************************************************
 * get_lm76_temperatur_4x(..) - Temperatur vier mal nacheinander holen, falls 
 * Temperaturen nicht identisch, Alarm auslösen
 * Bei korrekter Funktion des LM76 müssen alle vier Temperaturen gleich sein,
 * da der Sensor nicht genügend Zeit hat um eine neue Temperatur zu berechnen
 * PE:uint8_t adresse	// I2C Adresse LM76
 * PA:int16_t			// Temperatur als int Variable, 1 Bit entspricht 0.0625°C
*************************************************************************/
int16_t get_lm76_temperatur_4x(uint8_t adresse);


