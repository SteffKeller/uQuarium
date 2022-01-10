
/************************************************************************
 * Titel :		lcdmenu.h
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Header für Library zur Realisierung einer Menüstruktur auf einem 
				4x20Zeilen LCD, angesteuert über I2C Bus mit Portexpander PCF8574.
 * Autor :		Stefan Keller
 * Lehrgang:	Techniker HF ET 08-11 Klasse A
 * Datum :		6.9.20111
 * Ursprung:	www.tobias-schlegel.de
 * Note:		Um Die Library nutzen zu können waren einige Anpassungen nötig
 *				neben der normalen Menüstruktur in den Menu Array musste die 
 *				Ausgabe Routine umgeschrieben werden. Ebenfalls musste ein 
 *				vagabundierender Pointer im Menü Core gefangen werden
 ************************************************************************/
/*
Copyright (C) 2009  Tobias Schlegel,
                    tobias@drschlegel.de
                    www.tobias-schlegel.de

This program is free software; you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation; either version 3 of the License,
or any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program; if not, see <http://www.gnu.org/licenses/>.
*/


#include <avr/io.h>
#include "i2clcd.h"			// Library für LCD über I2C
#include "taster_timer.h"   // Library für Taster Entprellung
#include <avr/pgmspace.h>

#define ui_Max_Menu_Depth   4              //How many menu levels dowes the menu maximally have?
                                            //Mainmenu + 2 submenu + 1 subsubmenu = 3
#define ui_init_LCD_on_init_menu 0          //change this to 1 if you like lcdmenu to init your LCD when calling init_menu();

//STOP EDITING!
typedef struct _Menu_Entry Menu_Entry;      //Define the Menu-Entry-Type globally
typedef void (*FuncPtr) ( void );           //Define a pointer-to-a-function globally



void init_menu(void);                       //Initializes the Menu
void menu(uint8_t key);                     //Updates the internal state machine
void displayMenu(void);                     //Outputs the menu on the Display

/*************************************************************************
* Deklarationen der Menü Funktionen, die in main.h initialisiert werden
*************************************************************************/
extern void funkt_menu_temperatur_offet(void);
extern void funkt_menu_heizer(void);
extern void funkt_menu_luefter(void);
extern void funkt_menu_alarm(void);
extern void funkt_menu_datum(void);
extern void funkt_menu_uhr(void);
extern void funkt_menu_lampe_ein(void);
extern void funkt_menu_lampe_aus(void);
extern void funkt_menu_regenzeit1(void);
extern void funkt_menu_regenzeit2(void);
extern void funkt_menu_regenzeit3(void);
extern void funkt_menu_regenzeit4(void);
extern void funkt_menu_regenzeit5(void);
extern void funkt_menu_regenzeitzaehler(void);
extern void funkt_menu_regenpumpe_einzeit(void);
extern void funkt_menu_mondlicht_ein(void);
extern void funkt_menu_mondlicht_aus(void);
extern void funkt_menu_mondlicht_helligkeit(void);
extern void funkt_menu_about(void);
extern void funkt_menu_max_werte(void);
extern void funkt_menu_phwert_kalibrieren(void);


#define menu_has_submenu   0           //Causes the menu to call a submenu specified by the "sub" pointer.
#define menu_has_function  1           //Causes the menu to call the function specified by the "CallMe" pointer

