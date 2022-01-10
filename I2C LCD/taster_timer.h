/************************************************************************
 * Titel :		taster_timer.h
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Header für Library für Timer und Tasten entprellen
 * Autor :		Stefan Keller
 * Lehrgang:	Techniker HF ET 08-11 Klasse A
 * Datum :		4.9.20111
 * Ursprung:	Erstellt nach Tutorial http://www.mikrocontroller.net/articles/Entprellung
 * Note:		Tasten Entprellroutinen sind original belassen.
 *				Der Timer und die Interruptroutine wurden angepasst.
 *				So steuert der Timer auch das zeitliche Aus und Einlesen der Hardware
 *******************************************************************/

/* Beispiel
 * Taster kurz drücken:
 * if( get_key_short( 1<<KEY_OK )) foo;

   Taster lang drücken:
   if( get_key_long( 1<<KEY_LEFT)) foo;

   Taste gedrückt halten:
   if( get_key_press( 1<<KEY_OK ) || get_key_rpt( 1<<KEY_OK )) foo;
 */
#ifndef _TASTER_H
#define _TASTER_H
/*************************************************************************
* Quarzfrequenz des Atemga einstellen
*************************************************************************/
#ifndef F_CPU
#define F_CPU 16000000
#endif
/*************************************************************************
* Includes 
*************************************************************************/
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/*************************************************************************
* Defines Konstanten und Hardware Makros
*************************************************************************/
#define KEY_DDR         DDRB
#define KEY_PORT        PORTB
#define KEY_PIN         PINB
#define KEY_UP            0
#define KEY_LEFT          1
#define KEY_OK            2
#define KEY_RIGHT         3
#define KEY_DOWN          4
#define KEY_ESC           5
#define KEY_F			  6
#define KEY_RUN           7

#define ALL_KEYS        (1<<KEY_UP | 1<<KEY_LEFT | 1<<KEY_OK | 1<<KEY_RIGHT | 1<<KEY_DOWN | 1<<KEY_ESC | 1<<KEY_F | 1<<KEY_RUN)

#define REPEAT_MASK     (1<<KEY_UP | 1<<KEY_LEFT | 1<<KEY_OK | 1<<KEY_RIGHT | 1<<KEY_DOWN | 1<<KEY_ESC | 1<<KEY_F | 1<<KEY_RUN)     // repeat: KEY_LEFT, KEY_OK
#define REPEAT_START    50                     // after 500ms
#define REPEAT_NEXT     10                     // every 200ms

#define LED_DDR         DDRC
#define LED_PORT        PORTC
#define LED0            3
#define LED1            4
#define LED2            5

/*************************************************************************
* Variablen
*************************************************************************/
volatile uint8_t key_state;                                // debounced and inverted key state:
volatile uint8_t key_press;                                // key press detect
volatile uint8_t key_rpt;                                  // key long press and repeat
volatile uint8_t messflag_05s;
volatile uint8_t messflag_1s;
volatile uint32_t timer_counter;
/*************************************************************************
* Deklarationen der Funktionen
*************************************************************************/
/*************************************************************************
 * ISR(TIMER0_OVF_vect)  - Interrupt Timer 0 Overflow
 * Hier werden die eingelesen und entprellt. Die Flags für die Ansteuerung der
 * Hard werden alle 0.5s und 1s gesetzt
 * Timer wird alle 10ms aufgerufen
*************************************************************************/
ISR(TIMER0_OVF_vect);                           // every 10ms

/*------------------------------------------------------------------
 * get_key_pressed(..) - Key wird gedrückt
 * PE: key_mask= Tasten
 *check if a key has been pressed. Each pressed key is reported
 * only once
 *///---------------------------------------------------------------
uint8_t get_key_press( uint8_t key_mask );

///////////////////////////////////////////////////////////////////
//
// check if a key has been pressed long enough such that the
// key repeat functionality kicks in. After a small setup delay
// the key is reported beeing pressed in subsequent calls
// to this function. This simulates the user repeatedly
// pressing and releasing the key.
//
uint8_t get_key_rpt( uint8_t key_mask );

///////////////////////////////////////////////////////////////////
//
uint8_t get_key_short( uint8_t key_mask );

///////////////////////////////////////////////////////////////////
//
uint8_t get_key_long( uint8_t key_mask );
void init_taster();
#endif
