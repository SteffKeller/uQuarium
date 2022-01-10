/************************************************************************
 * Titel :		taster_timer.c
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Library für Timer und Tasten entprellen
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

#include "taster_timer.h"

/*************************************************************************
 * ISR(TIMER0_OVF_vect)  - Interrupt Timer 0 Overflow
 * Hier werden die eingelesen und entprellt. Die Flags für die Ansteuerung der
 * Hard werden alle 0.5s und 1s gesetzt
 * Timer wird alle 10ms aufgerufen
*************************************************************************/
ISR(TIMER0_OVF_vect)                           
{
    static uint8_t ct0, ct1, rpt;
    static uint8_t messcounter1 = 0;
    static uint8_t messcounter2 = 0;
    uint8_t i;

    TCNT0 = (uint8_t)(int16_t)-(F_CPU / 1024 * 10e-3 + 0.5);  // preload for 10ms
    //PORTC ^= (1<<PC7);
    i = key_state ^ ~KEY_PIN;                       // key changed ?
    ct0 = ~( ct0 & i );                             // reset or count ct0
    ct1 = ct0 ^ (ct1 & i);                          // reset or count ct1
    i &= ct0 & ct1;                                 // count until roll over ?
    key_state ^= i;                                 // then toggle debounced state
    key_press |= key_state & i;                     // 0->1: key press detect

    if( (key_state & REPEAT_MASK) == 0 )            // check repeat function
        rpt = REPEAT_START;                          // start delay
    if( --rpt == 0 )
    {
        rpt = REPEAT_NEXT;                            // repeat delay
        key_rpt |= key_state & REPEAT_MASK;

    }
	/************************************************************************
	 * Anpassung: Flag setzen alle 0.5s und 1s
	************************************************************************/

    messcounter1++; // Alle 10ms +1
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
    if(messcounter1 == 50) // Wenn 0.5s vorbei
    {
        messcounter1 = 0; // Counter resetten
        messflag_05s = 1; // Messflag setzen
        messcounter2++;	  // Counter 2 alle 0.5s erhören 
        if (messcounter2 == 2) // Wenn 1s vorbei
        {
            messcounter2 = 0; // Counter resetten
            messflag_1s =1;   // Messflag setzen
			/************************************************************************
			*  Counter für regenpumpe_einzeit und pH Kalibrierung 
			*  Falls Countdown aktiv, diesen alle 1s bis auf 0 runterzählen 
			************************************************************************/
			if (timer_counter) timer_counter--; 
		}
    }
}

/*------------------------------------------------------------------
 * get_key_pressed(..) - Key wird gedrückt
 * PE: key_mask= Tasten
 *check if a key has been pressed. Each pressed key is reported
 * only once
 *///---------------------------------------------------------------

uint8_t get_key_press( uint8_t key_mask )
{
    cli();                                          // read and clear atomic !
    key_mask &= key_press;                          // read key(s)
    key_press ^= key_mask;                          // clear key(s)
    sei();
    return key_mask;
}

///////////////////////////////////////////////////////////////////
//
// check if a key has been pressed long enough such that the
// key repeat functionality kicks in. After a small setup delay
// the key is reported beeing pressed in subsequent calls
// to this function. This simulates the user repeatedly
// pressing and releasing the key.
//
uint8_t get_key_rpt( uint8_t key_mask )
{
    cli();                                          // read and clear atomic !
    key_mask &= key_rpt;                            // read key(s)
    key_rpt ^= key_mask;                            // clear key(s)
    sei();
    return key_mask;
}

///////////////////////////////////////////////////////////////////
//
uint8_t get_key_short( uint8_t key_mask )
{
    cli();                                          // read key state and key press atomic !
    return get_key_press( ~key_state & key_mask );
}

///////////////////////////////////////////////////////////////////
//
uint8_t get_key_long( uint8_t key_mask )
{
    return get_key_press( get_key_rpt( key_mask ));
}
void init_taster()
{
	timer_counter =0;
    KEY_DDR &= ~ALL_KEYS;                // configure key port for input
    KEY_PORT |= ALL_KEYS;                // and turn on pull up resistors
}

