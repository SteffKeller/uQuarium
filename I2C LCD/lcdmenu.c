/************************************************************************
 * Titel :		lcdmenu.c
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Library zur Realisierung einer Menüstruktur auf einem 
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


#include "lcdmenu.h"

volatile uint8_t        ui_State[ui_Max_Menu_Depth] = {0,0,0};       //Variable containing the State of the Menu
volatile uint8_t        ui_MenuDepth = 0;                            //Contains the Menu depth

Menu_Entry              *ui_TopMenu, *ui_ActualMenu;                 //Pointers facilitating everyting

struct _Menu_Entry{                                                  //The menu-entry-structure
    uint8_t             ID;
    uint8_t             menue_typ;
    uint8_t             ItemsInMenu;
    Menu_Entry          * sub;
    Menu_Entry          * parent;
	const unsigned char *cText;
    FuncPtr             CallMe;
};

/*
Menu entry initialization as follows:

{$UNIQUE entry id (for translation out of external memory),
Behavior; either ui_MenBehav_Following or ui_MenBehav_Exec,
$amount of entries in current menu,
(Menu_Entry *) 0,
Menu_Entry *) 0,
"$$YOUR ENTRY TEXT HERE$$",
(FuncPtr)0}

template: {, ui_MenBehav_, 2, (Menu_Entry *) 0, (Menu_Entry *) 0, "", (FuncPtr)0}
*/
/*************************************************************************
* Sting Array im Flash für die Anzeige im Menü
*************************************************************************/
const char str0[] PROGMEM = "1 Temperatur";			// Top Menü
const char str1[] PROGMEM = "2 Zeit       ";	
const char str2[] PROGMEM = "3 Licht      ";
const char str3[] PROGMEM = "4 PH-Wert    ";
const char str4[] PROGMEM = "5 Tageswerte ";
const char str5[] PROGMEM = "6 About      ";

const char str10[] PROGMEM = "1-1 Temp Offset";		// Temperatur Menü
const char str11[] PROGMEM = "1-2 Heizer";
const char str12[] PROGMEM = "1-3 Luefter";
const char str13[] PROGMEM = "1-4 Alarm ";

const char str20[] PROGMEM = "2-1 Zeit einst.";		// Zeit Menü
const char str21[] PROGMEM = "2-2 Datum einst.";
const char str22[] PROGMEM = "2-3 Futterstopp";

const char str30[] PROGMEM = "3-1 Lampe ein AM";		// Licht Menü
const char str31[] PROGMEM = "3-2 Lampe aus AM";
const char str35[] PROGMEM = "3-3 Lampe ein PM";		// Licht Menü
const char str36[] PROGMEM = "3-4 Lampe aus PM";
const char str32[] PROGMEM = "3-5 ML ein";
const char str33[] PROGMEM = "3-6 ML aus";
const char str34[] PROGMEM = "3-7 ML Helligkeit";

const char str40[] PROGMEM = "4-1 pH kalib.";	// pH Menü




// Temperatur Menü
Menu_Entry Menu1[4] = {
            {10, menu_has_function, sizeof (Menu1) / sizeof (Menu1[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str10, (FuncPtr)funkt_menu_temperatur_offet},
            {11, menu_has_function, sizeof (Menu1) / sizeof (Menu1[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str11, (FuncPtr)funkt_menu_heizer},
			{12, menu_has_function, sizeof (Menu1) / sizeof (Menu1[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str12, (FuncPtr)funkt_menu_luefter},
            {13, menu_has_function, sizeof (Menu1) / sizeof (Menu1[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str13, (FuncPtr)funkt_menu_alarm}
};
// Zeit Menü
Menu_Entry Menu2[3] = {
            {20, menu_has_function, sizeof (Menu2) / sizeof (Menu2[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str20, (FuncPtr)funkt_menu_uhr},
            {21, menu_has_function, sizeof (Menu2) / sizeof (Menu2[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str21, (FuncPtr)funkt_menu_datum},
			{22, menu_has_function, sizeof (Menu2) / sizeof (Menu2[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str22, (FuncPtr)funkt_menu_futterstop}
};
// Licht Menü
Menu_Entry Menu3[7] = {
            {30, menu_has_function, sizeof (Menu3) / sizeof (Menu3[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str30, (FuncPtr)funkt_menu_lampe_ein_am},
            {31, menu_has_function, sizeof (Menu3) / sizeof (Menu3[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str31, (FuncPtr)funkt_menu_lampe_aus_am},
			{32, menu_has_function, sizeof (Menu3) / sizeof (Menu3[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str35, (FuncPtr)funkt_menu_lampe_ein_pm},
            {33, menu_has_function, sizeof (Menu3) / sizeof (Menu3[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str36, (FuncPtr)funkt_menu_lampe_aus_pm},
			{34, menu_has_function, sizeof (Menu3) / sizeof (Menu3[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str32, (FuncPtr)funkt_menu_mondlicht_ein},
            {35, menu_has_function, sizeof (Menu3) / sizeof (Menu3[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str33, (FuncPtr)funkt_menu_mondlicht_aus},
		    {36, menu_has_function, sizeof (Menu3) / sizeof (Menu3[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str34, (FuncPtr)funkt_menu_mondlicht_helligkeit},
};
// pH Menü
Menu_Entry Menu4[1] = {
            {40, menu_has_function, sizeof (Menu4) / sizeof (Menu4[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str40, (FuncPtr)funkt_menu_phwert_kalibrieren}
};

// Top Menü
Menu_Entry MainMenu[6] = { 
            {0, menu_has_submenu, sizeof (MainMenu) / sizeof (MainMenu[0]), (Menu_Entry *) Menu1, (Menu_Entry *) 0, str0, (FuncPtr)0},
            {1, menu_has_submenu, sizeof (MainMenu) / sizeof (MainMenu[0]), (Menu_Entry *) Menu2, (Menu_Entry *) 0, str1, (FuncPtr)0},
			{2, menu_has_submenu, sizeof (MainMenu) / sizeof (MainMenu[0]), (Menu_Entry *) Menu3, (Menu_Entry *) 0, str2, (FuncPtr)0},
			{3, menu_has_submenu, sizeof (MainMenu) / sizeof (MainMenu[0]), (Menu_Entry *) Menu4, (Menu_Entry *) 0, str3, (FuncPtr)0},
			{4, menu_has_function, sizeof (MainMenu) / sizeof (MainMenu[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str4, (FuncPtr)funkt_menu_max_werte},
			{5, menu_has_function, sizeof (MainMenu) / sizeof (MainMenu[0]), (Menu_Entry *) 0, (Menu_Entry *) 0, str5, (FuncPtr)funkt_menu_about}
    };


	



void init_menu(void){       //Initializes menu.

    uint8_t     i = 0;

   /*************************************************************************
   *  Adresse für Rücksprung auf übergeordnetes Menü definieren
   *************************************************************************/
	Menu1[0].parent = MainMenu;
	Menu1[1].parent = MainMenu;
	Menu1[2].parent = MainMenu;
	Menu1[3].parent = MainMenu;
	
    
	Menu2[0].parent = MainMenu;
	Menu2[1].parent = MainMenu;
	Menu2[2].parent = MainMenu;
	
    Menu3[0].parent = MainMenu;
	Menu3[1].parent = MainMenu;
	Menu3[2].parent = MainMenu;
	Menu3[3].parent = MainMenu;
	Menu3[4].parent = MainMenu;
	
	Menu4[0].parent = MainMenu;

	ui_ActualMenu = MainMenu;
    //ui_ActualMenu = ui_TopMenu;
    ui_MenuDepth = 0;           //Start in the topmost menu

    for(i = 0 ; i < ui_Max_Menu_Depth ; i++){
        ui_State[i] = 0;
    }
}

//DO NOT EDIT BELOW! == CORE ==  == CORE ==  == CORE ==  == CORE ==  == CORE ==  == CORE ==  == CORE ==

void menu(uint8_t key){

    uint8_t     max_depth = 0, i = 0;
	

    max_depth = ui_ActualMenu[0].ItemsInMenu;

    switch(key){
        case KEY_UP:        //Go up the menu

            if(ui_State[ui_MenuDepth] == 0){
                ui_State[ui_MenuDepth] = max_depth - 1;
            }else{
                ui_State[ui_MenuDepth]--;
            }
			
			/*************************************************************************
			* Anpassung um Anzeige beim springen durchs Menü zu löschen
			*************************************************************************/
			lcd_command(LCD_CLEAR);
            break;

        case KEY_DOWN:      //Go down the menu

            if(ui_State[ui_MenuDepth] == max_depth - 1){
                ui_State[ui_MenuDepth] = 0;
            }else{
                ui_State[ui_MenuDepth]++;
            }
			lcd_command(LCD_CLEAR);
            break;

        case KEY_LEFT:      //go to the parent menu
		/*
            if(ui_MenuDepth != 0){
                ui_ActualMenu = ui_ActualMenu[ui_State[ui_MenuDepth]].parent;
                ui_MenuDepth--;
            }
			lcd_command(LCD_CLEAR);
            break;
*/
		
		ui_ActualMenu = MainMenu; // richtige Zuweisung
			lcd_command(LCD_CLEAR);
        
            ui_MenuDepth = 0;

            for(i = 0 ; i < ui_Max_Menu_Depth ; i++){
                ui_State[i] = 0;
            }

            break;
        case KEY_RIGHT:
			if(ui_ActualMenu[ui_State[ui_MenuDepth]].menue_typ == menu_has_submenu)
			{
                ui_ActualMenu = ui_ActualMenu[ui_State[ui_MenuDepth]].sub;
                ui_MenuDepth++;
            }else{
                ui_ActualMenu[ui_State[ui_MenuDepth]].CallMe();
            }
			lcd_command(LCD_CLEAR);
            break;
			
        case KEY_OK:

            if(ui_ActualMenu[ui_State[ui_MenuDepth]].menue_typ == menu_has_submenu)
			{
                ui_ActualMenu = ui_ActualMenu[ui_State[ui_MenuDepth]].sub;
                ui_MenuDepth++;
            }else{
                ui_ActualMenu[ui_State[ui_MenuDepth]].CallMe();
            }
			lcd_command(LCD_CLEAR);
            break;

        case KEY_ESC:       //goto main menu
		
		/*************************************************************************
		* Diese Pointerzuweisung führt ins leere, da ui_TopMenu zwar initialisiert 
		* aber nicht deklariert ist. Die ganze Anzeige funktioniert so nicht. 
		*************************************************************************/
		//ui_ActualMenu = ui_TopMenu;  
		/*************************************************************************
		* So stimmt dir Zuweisung, ui_ActualMenu erhält die richtige Adresse auf das
		* Top Menü, und wird im Display angezeigt
		*************************************************************************/
			ui_ActualMenu = MainMenu; // richtige Zuweisung
			lcd_command(LCD_CLEAR);
        
            ui_MenuDepth = 0;

            for(i = 0 ; i < ui_Max_Menu_Depth ; i++){
                ui_State[i] = 0;
            }

            break;

        default:
        break;
    }
}


void displayMenu(void){                     //Displays the menu on the LCD

    uint8_t     i = 0, j = 0, k = 1;

	lcd_gotolr(1,1); // Cursor auf erstes Zeichen oben links

    i = ui_State[ui_MenuDepth];             //The first line is always the "chosen one"

    j = i;

    while(k <= LCD_LINES){

/*************************************************************************
*  Die Ausgeben des Menüs musste umgeschrieben werden, da die Zeilen und Spalten
*  über (Linie/Spalte) und nicht mit (Spalte/Zeile) adressiert werden 
*  und ausserdem nicht 0 terminiert sind wie in der original Routine. 
*  Die Anzeige der Strings funktioniert ausserdem direkt aus dem Flash
*************************************************************************/
        lcd_gotolr(k, 1); // Aus richtige Zeile springen

        if((i == j) && (k != 1)){  // Menü entsprechend der LCD grösse (4x20) anzeigen
			 break;
        }

        if(i == ui_State[ui_MenuDepth]){    //Is this the actually selected Item?
            lcd_putchar('>');                  //Menüpfeil ausgeben
        }

       lcd_gotolr( k,2);                    //rechts vom Menüpfeil
	   
	   //String für das Menü aus dem Flash holen und anzeigen
        lcd_print_progmem(ui_ActualMenu[i].cText); 
        i++;                                //nächster Menüeintrag

        if( i >= ui_ActualMenu[0].ItemsInMenu ){ //run out of entries?
            i = 0;
        }
        k++;    //line counter
    }

}

