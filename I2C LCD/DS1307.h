/************************************************************************
 * Titel :		DS1307.h
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Header für Library zur Ansteuerung des DS1307 RTC ICs
 * Autor :		Stefan Keller
 * Lehrgang:	Techniker HF ET 08-11 Klasse A
 * Datum :		4.9.20111
 ************************************************************************/
#ifndef _DS1307_H_
#define _DS1307_H_
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
#include <avr/io.h>			// AVR Register und Konstantendefinitionen
#include <stdio.h>
#include <util/delay.h>		// Delays
#include "i2cmaster.h"		// I2C Library
/*************************************************************************
* Defines Konstanten
*************************************************************************/
#define DS1307_ADRESSE 0xD0 // I2C Adresse DS1307 -> 0b11010000
/*************************************************************************
* Variablen
*************************************************************************/
typedef struct uhr
{
    uint8_t minute;
    uint8_t sekunde;
    uint8_t stunde;
    uint8_t tag;
    uint8_t wochentag;
    uint8_t monat;
    uint8_t jahr;
} uhr;

extern uint8_t alarm_vektor; // Vektor für Alarme und Fehler
/*************************************************************************
* Deklarationen der Funktionen
*************************************************************************/
/*************************************************************************
 * ds_1307_init(..) - Initialisiert den DS1307 und aktiviert den Oszillator
 * PE:void
 * PA:void
*************************************************************************/
void ds_1307_init();
/************************************************************************
 * get_ds1307_zeit_datum(..) - Aktuelle Zeit und Datum vom DS1307 einlesen
 * PE:Pointer auf struct vom Typ uhr
 * PA:--
************************************************************************/
void get_ds1307_zeit_datum(uhr *zeit_pointer);
/************************************************************************
 * set_ds1307_zeit(..) - Uhrzeit auf DS1307 schreiben
 * PE:Pointer auf struct uhr
 * PA:--
************************************************************************/
void set_ds1307_zeit(uhr *zeit_pointer);
/************************************************************************
 * set_ds1307_datum(..) - Datum auf den DS1307 schreiben
 * PE:Pointer auf struct uhr
 * PA:--
************************************************************************/
void set_ds1307_datum(uhr *zeit_pointer);


