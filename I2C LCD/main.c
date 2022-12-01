/************************************************************************
 * Titel :		main.c
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Hauptprogramm der Steuerung
 * Autor :		Stefan Keller
 * Lehrgang:	Techniker HF ET 08-11 Klasse A
 * Datum :		4.9.20111
 ************************************************************************/

#include "main.h"
/*************************************************************************
* Main Funktion -> Hier beginnt das Programm 
*************************************************************************/
int main(void)
{
 	/*************************************************************************
	*Hardware initialisieren 
	*************************************************************************/
	if (MCUSR & (1<<WDRF))		// Auswerten ob Reset durch Watchdog
	{
		alarm_vektor = 6;		// Watchdog alarm
		MCUSR &= ~(1<<WDRF);	// Watchdog Reset Flag resetten
	}
	wdt_disable();				// Watchdog deaktivieren
    timer_init();				// Timer initialisieren
    read_eeprom_daten();		// gespeicherte Daten aus dem EEPROM laden
    init_taster();				// Tasten entprellen initialisieren
    i2c_init();					// I2C-Bus initialisieren
    ds_1307_init();				// DS1307 initialisieren
    lcd_init();					// LCD initialisieren
    init_ports();				// Ports Ein-Ausgänge initialisieren
	adc_init();					// AD-Wandler initialisieren
    //uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) ); // UART für RS232 initialisieren
    sei();						// Global Interrupt freigeben
	/*************************************************************************
	* Sensorwerte erstes mal auslesen und Anfangswerte der Variablen setzen
	*************************************************************************/
    _delay_ms(1000);            // 1s warten, um LM76 genügend zeit zu geben die erste Temperatur zu berechnen
	wdt_enable(WDTO_1S);		// Watchdog aktivieren -> Timer 1s
    temperatur = get_lm76_temperatur(LM76_ADRESSE)+ temperatur_offset; // Erstes mal die Temperatur vom LM76 holen, so das die Ausgänge von anfang an richtig gesetzt werden
    get_ds1307_zeit_datum(&zeit);// Erstes mal die Zeit vom DS1307 holen, so das die Ausgänge von anfang an richtig gesetzt werden
	ph_wert = read_phwert(&phwert_referenzen); // Erstes mal den pH Wert einlesen, so das die Ausgänge von anfang an richtig gesetzt werden
    extrem_temperatur.max_wert = temperatur; // Tages Min/Max Werte auf aktuelle Temperatur setzen
    extrem_temperatur.min_wert = temperatur;
	extrem_ph.max_wert = ph_wert;			 // Tages Min/Max Werte auf aktuellen pH Wert setzen
	extrem_ph.min_wert = ph_wert;
	zustandsbit.run = 1;		// Steuerung in Run Mode
	zustandsbit.pumpe = 1;		// Pumpe eingeschaltet
	zustandsbit.esc_alarm = 0;		// Pumpe eingeschaltet
	
    /*************************************************************************
    * Menü initialisieren
    *************************************************************************/
    init_menu();				// Menü Struktur initialisieren
    menu(5);					// Hauptmenü laden
	/*************************************************************************
	* LCD einschalten
	*************************************************************************/
    lcd_command(LCD_CLEAR);     // LCD-Anzeige löschen
    lcd_backlight(ON);			// LCD-Hintergrundbeleuchtung an
    lcd_command(LCD_CURSOROFF); // LCD-Cursor off
	static uint8_t lichtaus= 0;
	/*************************************************************************
	* Main Schleife -> In dieser läuft das ganze Programm ab
	*************************************************************************/
    while (true) 
    {	
		wdt_reset();	// Watchdog bei jedem Durchlauf resetten
		main_anzeige(); //Hauptbildschirm anzeigen
		/*************************************************************************
		* Taste OK gedrückt ->Menü aufrufen und darin navigieren und 
		* Einstellungen vornehmen
		*************************************************************************/
		if (get_key_press(1<<KEY_OK)) // Wenn Taste OK gedrückt -> Hauptmenü anzeigen
		{
			wdt_disable(); // Im Menü ist der Watchdog nicht aktiv, da in den Routinen auf Tasteneingaben gewartet werden muss 
			lcd_command(LCD_CLEAR); // LCD löschen
			lcd_backlight(ON);		// Backlight ein
			displayMenu();          // Hauptmenü anzeigen
			while(true)				// Auf Tasteneingaben warten und Menübaum aktualisieren
			{
				if(get_key_press(1<<KEY_UP))		menu(KEY_UP);// Im Menübaum eine Stelle nach oben
				else if(get_key_press(1<<KEY_LEFT)) menu(KEY_LEFT);// Vorheriges Menü anzeigen
				else if(get_key_press(1<<KEY_OK))	menu(KEY_OK);// Untermenü oder Funktion aufrufen
				else if(get_key_press(1<<KEY_RIGHT))menu(KEY_RIGHT);// Untermenü oder Funktion aufrufen
				else if(get_key_press(1<<KEY_DOWN)) menu(KEY_DOWN);// Im Menübaum eine Stelle nach unten
				else if(get_key_press(1<<KEY_ESC))			// Menü verlassen -> Hauptbildschirm anzeigen
				{
					wdt_enable(WDTO_1S);					// Watchdog einschalten
					menu(KEY_ESC);							// Hauptmenü 
					lcd_command(LCD_CLEAR);					// LCD löschen
					lichtaus = 0;							// Counter für Backlight resetten
					break;									// Menüschleife verlassen
				}				 
				displayMenu();								// aktuelles Menü anzeigen
			}
		}
		/*************************************************************************
		* Auf Tasteneingaben in der Hauptanzeige warten, 
		* wird irgendeine Taste gedrückt -> Backlight ein
		*************************************************************************/
		/*************************************************************************
		* Taste RUN gedrückt -> Steuerung zwischen Run und Wartungsmodus umschalten
		*************************************************************************/
		if(get_key_press(1<<KEY_RUN))
		{
			zustandsbit.run ^= 1;	// Steuerung Run oder Wartungs Mode
			lcd_backlight(ON);		// Backlight ein
			lichtaus = 0;			// Counter für Backlight resetten

		}	
		/*************************************************************************
		* Taste F gedrückt -> Pumpe ausschalten für Fütterung 
		*************************************************************************/
		if(get_key_press(1<<KEY_F))
		{
			futterstop_aktivieren(&futterstopp);
			lcd_backlight(ON);	// Backlight ein
			lichtaus = 0;		// Counter für Backlight resetten
		}
		/*************************************************************************
		* Taste ESC gedrückt und Backlight ein -> Alarm löschen
		* Wenn Backlight aus -> Alarm nicht löschen, so kann der Alarm auch im 
		* Dunkeln überprüft werden
		*************************************************************************/
		if(get_key_press(1<<KEY_ESC))
		{
			if(alarm_vektor != 0 && zustandsbit.esc_alarm == 0)
			{
				zustandsbit.esc_alarm =1;
			} 
			
			else if(alarm_vektor != 0  && zustandsbit.esc_alarm == 1)
			{	
				zustandsbit.esc_alarm= 0;
				if(lichtaus < LCDBACKLIGHT_ON_ZEIT) alarm_vektor = 0; // Backlight eingeschaltet? Alarm Vektor löschen

			}
			lcd_backlight(ON);	// Backlight ein
			lichtaus = 0;		// Counter für Backlight resetten
		}	
		/*************************************************************************
		* Falls eine der anderen Tasten gedrückt -> Backlight ein
		*************************************************************************/	
		if(get_key_press(1<<KEY_UP | 1<<KEY_DOWN | 1<<KEY_LEFT | 1<<KEY_RIGHT | 1<<KEY_ESC))
		{
			
			lcd_backlight(ON);	// Backlight ein
			lichtaus = 0;		// Counter für Backlight resetten
		}
		/*************************************************************************
		* Sensorwerte einlesen und Ausgänge ansteuern
		*************************************************************************/
		/*************************************************************************
		* Alle 0.5 sec wird die Zeit eingelesen und die Ausgänge
		* angesteuert
		*************************************************************************/			
		if (messflag_05s) // Wenn 0.5s vorbei, wird durch Timerinterrupt gesteuert
        {
            messflag_05s =0;				// ISR Flag zurücksetzen
            get_ds1307_zeit_datum(&zeit);	// Zeit vom DS1307 holen;
            lampen_schalten(); 				// Lampen Ausgänge ansteuern	
			reset_tageswerte(); // Um Mitternacht Tages Min- und Maxwerte löschen
			if (timer_counter)				// Pumpenstopp aktiv
			{
				zustandsbit.pumpe = 0;		// Zustandsbit für Pumpe ein
			}
			else zustandsbit.pumpe = 1;		// Zustandsbit für Pumpe aus
			ausgaenge_ansteuern();	
			
        }
		/*************************************************************************
		* Alle 1 sec werden die Temperatur und der pH Wert eingelesen 
		* die Zustandsbits entsprechend gesetzt, die aktuellen Werte per UART
		* gesendet und überprüft ob das Backlight ausgeschaltet werden muss
		*************************************************************************/	
        if (messflag_1s) // Wenn 1s vorbei, wird durch Timerinterrupt gesteuert 
        {
            messflag_1s =0;								// ISR Flag zurücksetzen
            temperatur = get_lm76_temperatur_4x(LM76_ADRESSE)+ temperatur_offset; // Temperatur vom LM76 holen und Offset addieren;
			ph_wert = read_phwert(&phwert_referenzen);	// pH Wert über ADC holen 
            temperatuen_schalten();						// Zustandsbit für Temperatur gesteuerte Ausgänge schalten
			//send_to_uart();								// aktuelle Werte über UART senden
			if(lichtaus > LCDBACKLIGHT_ON_ZEIT) lcd_backlight(OFF);	// überprüfen ob Zeit für Backlight in abgelaufen		
			else lichtaus++;							// ansonsten Variable für Backlight inkrementieren
		}        
    }
}
/*************************************************************************
 * timer_init(..) - Timer 0 und Timer 1 des Atmega 644p initialisieren
 * PE:void
 * PA:void
 *************************************************************************/
void timer_init(void)
{
	//Timer 0 für Tastenentprellen und zeitgesteuertes Einlesen der Messwerte 
	TCCR0B |= (1<<CS00)|(1<<CS02);		// Clock/1024 ergibt Timerfrequenz von 15625 Hz
    TIMSK0 |= 1<<TOIE0;					// Timer 0 Interrupt einschalten
	//Timer 1B für PWM Mondlicht
	TCCR1A = (1<<COM1B1) | (1<<WGM10);	// PWM-Mode 1,  8Bit PWM zählt bis 255, Phasenkorrekt
	TCCR1B = (1<<CS10) | (1<<CS11);		// Clock/64 -> + zählen bis 255 ergibt PWM-Frequenz von 490Hz
	
    sei(); //Timer global freigeben
}
/*************************************************************************
 * init_ports(..) - Ports initialisieren -> Datenrichtung bestimmen und Pull-UPs einschalten
 * PA:void
 *************************************************************************/
void init_ports(void)
{
	// O -> Eingang, 1 -> Ausgang 
	// Nach dem Einschalteten Standart alles auf Eingang
    DDRA = 0x0F;		//PA0-PA3 -> NETZ1-NETZ4 aus Ausgang
    DDRD = 0x30;		//PD4, PD5 -> MLPWM und Buzzer auf Ausgang
    DDRC |= (1<<PC6) | (1<<PC7); //PC6, PC7 -> LED RUN und Alarm auf Ausgang

}
/*************************************************************************
 * temperatur_to_string(..) - Temperatur in einen String umwandeln
 * PE:uint32_t aktuelle_temperatur	// Temperatur
 * PE:char * temp_buffer			// Pointer auf Ausgabestring
 * PA:void
 *************************************************************************/
void temperatur_to_string(int16_t aktuelle_temperatur, char* temp_buffer)
{
    int16_t ganze_grad, zehntel_grad;  // Hilfsvariablen
    int32_t string_temperatur;

    string_temperatur = (int32_t) aktuelle_temperatur;
    string_temperatur = string_temperatur *625;			// Bitfolge in Grad umrechnen da 1 Bit 0.0625 Grad entspricht
    ganze_grad = string_temperatur /10000;				// umrechnen in ganze Grad
    zehntel_grad = ((string_temperatur % 10000) /1000);	// umrechnen der zehntel Grad

	/*************************************************************************
	* Routine um Minus Temperaturen richtig auf dem Display anzeigen zu können 
	* falls Temperatur oder Hysterese unter Null
	*************************************************************************/
    if(zehntel_grad < 0)
    {
        zehntel_grad = 0 -zehntel_grad; // Wert positiv machen -> oder auch Minus abschneiden

        if (ganze_grad == 0) // Falls die Vorkommastelle 0 ist -> Minus an Anfang des Strings anhängen, da sprintf das nicht automatisch macht
        {
            sprintf(temp_buffer,"-%i.%iC", ganze_grad, zehntel_grad); // String mit extra Minus Zeichen zusammensetzen
        }
        else sprintf(temp_buffer,"%i.%iC", ganze_grad, zehntel_grad); // Falls ganze Grad auch Minus einfach String zusammensetzen -> Minus wird automatisch von sprintf gesetzt
    }
    else sprintf(temp_buffer,"%i.%iC", ganze_grad, zehntel_grad); // Falls Wert positiv einfach String zusammensetzen
}
/*************************************************************************
 * zeit_to_string(..) - Uhrzeit in String umwandeln
 * PE:struct uhr ausgabe_zeit	// Struct mit Uhrzeit
 * PE:char * temp_buffer		// Pointer auf Ausgabestring
 * PA:void
*************************************************************************/
void zeit_to_string( struct uhr ausgabe_zeit,char* temp_buffer)
{
	// String zusammensetzen
	sprintf(temp_buffer, "%02d:%02d:%02d", ausgabe_zeit.stunde, ausgabe_zeit.minute, ausgabe_zeit.sekunde);	
}
/*************************************************************************
 * datum_to_string(..) - Datum in String umwandeln
 * PE:struct uhr ausgabe_datum	// Struct mit Datum 
 * PE:char * temp_buffer	 // Pointer auf Ausgabestring
 * PA:void
*************************************************************************/
void datum_to_string( struct uhr ausgabe_datum,char* temp_buffer)
{
	// String zusammensetzen
	sprintf(temp_buffer, "%02d.%02d.%02d", ausgabe_datum.tag, ausgabe_datum.monat, ausgabe_datum.jahr);	
}
/*************************************************************************
 * phwert_to_string(..) - PH-Wert in Form von 7000 in Kommazahl 7.0 umwandeln für die LCD Ausgabe
 * PE:uint16_t ph_wert // Aktueller Ph-Wert 7000 entspricht Ph 7.0 
 * PE:char * temp_buffer // Pointer auf Ausgabestring
 * PA:void
 *************************************************************************/
void phwert_to_string( uint16_t ph_wert, char* temp_buffer)
{
	uint16_t ganze_ph; // Hilfsvariable für Zahl vor dem Komma
	uint16_t zentel_ph; // Hilfsvariable für Zahl hinter dem Komma
	
	ph_wert += 50;							// Auf zehntel runden
	ganze_ph = ph_wert / 1000;				// umrechnen in ganze Ph
    zentel_ph = ((ph_wert % 1000) /100);	// umrechnen der zehntel Ph
    sprintf(temp_buffer,"%u.%u ", ganze_ph, zentel_ph); // String für den Ph Wert zusammensetzen
}
/*************************************************************************
 * menue_zeiten_einstellen(..) - Uhr und Schaltzeiten einstellen und speichern
 * PE:struct uhr * zeit_pointer
 * PA:void
 *************************************************************************/
void menue_zeiten_einstellen(struct uhr *zeit_pointer, struct uhr *eeprom_write)
{
    uint8_t display_update =1, cursor_position =0;																// Hilfsvariablen für Menue Navigation
    while(1)																									//State Machine für die Navigation in der Einstellung
    {
        lcd_command(LCD_CURSORON | LCD_BLINKINGON);																// Blinkender Cursor
        lcd_gotolr(3,(2+(cursor_position*3)));																	// Cursor Position auf gewählte Zeit
        if (get_key_short(1<<KEY_RIGHT)) (cursor_position==2) ? (cursor_position = 0) : (cursor_position++);	// nächste Einstellung
        else if (get_key_short(1<<KEY_LEFT)) (cursor_position==0) ? (cursor_position = 2) : (cursor_position--);// vorherige Einstellung
        else if(get_key_press(1<<KEY_ESC)) break;	// Funktion verlassen ohne zu speichern
        else if(get_key_press(1<<KEY_OK)) // Bestätigen und Werte abspeichern
        {
            lcd_command(LCD_CURSOROFF | LCD_BLINKINGOFF);
            lcd_command(LCD_CLEAR);
            if(eeprom_write != NULL)
            {
				cli(); // Interrupts global ausschalten da EEPROM Zugriffe zeitkritisch
                eeprom_write_block (zeit_pointer, eeprom_write, sizeof(struct uhr)); // Schalttemperaturen im EEPROM speichern
				sei();	// Interrupts global einschalten
            }
            else set_ds1307_zeit(zeit_pointer);  // Zeit auf DS1307 schreiben
            lcd_printlc(1,1,(unsigned char*) "Speichere Daten");
            _delay_ms(500);
            lcd_command(LCD_CLEAR); // LCD Anzeige löschen
            break;
        }
        if(display_update)  // Wenn sich die Zeit geändert hat
        {
            display_update =0;  //
			zeit_to_string(*zeit_pointer, anzeigetext1); 	// Zeit in ASCII umwandeln
            lcd_printlr(3,1,(unsigned char*)anzeigetext1);																// Zeit ausgeben
        }
        switch (cursor_position) // Anwahl der welche Zeiteinstellung geändert werden soll -> Stunden, Minuten oder Sekunden
        {
		/*************************************************************************
		* Stunde wird geändert
		*************************************************************************/
        case 0:	 
        {
            if (get_key_press( 1<<KEY_UP) | get_key_rpt(1<< KEY_UP))									// Taste UP gedrückt
            {
                (zeit_pointer->stunde == 23) ? (zeit_pointer->stunde =0) : (zeit_pointer->stunde++);	//Abfrage ob Stundenüberlauf und dann Wert setzen
                display_update = 1;																		// Zeit hat sich geändert
            }
            else if ( get_key_press( 1<<KEY_DOWN ) | get_key_rpt(1<< KEY_DOWN))							// Taste DOWN gedrückt
            {
                (zeit_pointer->stunde == 0) ? (zeit_pointer->stunde =23) : (zeit_pointer->stunde--);	//Abfrage ob Stundenüberlauf und dann Wert setzen
                display_update = 1;																		// Zeit hat sich geändert
            }
            break;
        }
		/*************************************************************************
		* Minuten werden geändert
		*************************************************************************/
        case 1:  
        {
            if (get_key_press( 1<<KEY_UP) | get_key_rpt(1<< KEY_UP))									// Taste UP gedrückt
            {
                (zeit_pointer->minute == 59) ? (zeit_pointer->minute =0) : (zeit_pointer->minute++);	// Abfrage ob Minutenüberlauf und dann Wert setzen
                display_update =1;																		// Zeit hat sich geändert
            }
            else if ( get_key_press( 1<<KEY_DOWN ) | get_key_rpt(1<< KEY_DOWN))							// Taste DOWN gedrückt
            {
                (zeit_pointer->minute== 0) ? (zeit_pointer->minute =59) : (zeit_pointer->minute--);		// Abfrage ob Minutenüberlauf und dann Wert setzen
                display_update = 1;																		// Zeit hat sich geändert
            }
            break;
        }
		/*************************************************************************
		* Sekunden werden geändert
		*************************************************************************/
        case 2: 
        {
            if ( get_key_press( 1<<KEY_UP) | get_key_rpt(1<< KEY_UP))									// Taste UP gedrückt
            {
                (zeit_pointer->sekunde == 59) ? (zeit_pointer->sekunde =0) : (zeit_pointer->sekunde++); // Abfrage ob Sekundenüberlauf und dann Wert setzen
                display_update =1;																		// Zeit hat sich geändert
            }
            if ( get_key_press( 1<<KEY_DOWN ) | get_key_rpt(1<< KEY_DOWN))								// Taste DOWN gedrückt
            {
                (zeit_pointer->sekunde == 0) ? (zeit_pointer->sekunde =59) : (zeit_pointer->sekunde--); // Abfrage ob Sekundenüberlauf und dann Wert setzen
                display_update = 1;																		// Zeit hat sich geändert
            }
            break;
        }
        default :
            break;
        }
    }
    lcd_command(LCD_CLEAR);	// LCD Anzeige löschen
}
/*************************************************************************
 * menue_datum_einstellen(..) - Datum einstellen und speichern
 * PE:struct uhr * datum_pointer
 * PA:void
 *************************************************************************/
void menue_datum_einstellen( struct uhr *datum_pointer)
{
    uint8_t display_update =1, cursor_position =0;

    lcd_command(LCD_CURSORON | LCD_BLINKINGON);

    while(1)																					//State Machine für die Navigation in der Einstellung
    {
        lcd_gotolr(3,(2+(cursor_position*3)));																		// Cursor Position auf gewählte Einstellung
        if (get_key_short(1<<KEY_RIGHT)) (cursor_position==3) ? (cursor_position = 0) : (cursor_position++);		// nächste Einstellung
        else if (get_key_short(1<<KEY_LEFT)) (cursor_position==0) ? (cursor_position = 3) : (cursor_position--);	// vorherige Einstellung
        else if(get_key_press(1<<KEY_ESC)) break;	// Funktion verlassen ohne zu speichern
        else if(get_key_press(1<<KEY_OK)) // Bestätigen und Werte abspeichern
        {
            lcd_command(LCD_CURSOROFF | LCD_BLINKINGOFF);
            lcd_command(LCD_CLEAR);
            set_ds1307_datum(datum_pointer);  // Datum auf DS1307 schreiben
            lcd_printlc(1,1,(unsigned char*) "Speichere Daten");
            _delay_ms(500);
            lcd_command(LCD_CLEAR); // LCD Anzeige löschen
            break;
        }

        if(display_update)  // Wenn sich eine Einstellung geändert hat, Display aktualisieren
        {
            display_update =0;  //Display bei nächsten durchlauf nicht mehr aktualisieren
            lcd_print(strcpy_P (anzeigetext,wochentagname[datum_pointer->wochentag])); // Wochentag aus dem Flash in String umwandeln
            lcd_printlr(3,1,(unsigned char*)anzeigetext); // Wochentag anzeigen
            lcd_print(" "); // Leerstelle
			datum_to_string(*datum_pointer, anzeigetext1); // Datum in String umwandeln
            lcd_printlr(3,4,(unsigned char*)anzeigetext1);																// Datum ausgeben
        }

        switch (cursor_position) // Anwahl der welche Datumseinstellung geändert werden soll -> Wochentag, Tag, Monat, Jahr
        {
		/*************************************************************************
		* Wochentag wird geändert
		*************************************************************************/
        case 0:	
        {
            if (get_key_press( 1<<KEY_UP) | get_key_rpt(1<< KEY_UP))									// Taste UP gedrückt
            {
                (datum_pointer->wochentag == 7) ? (datum_pointer->wochentag =1) : (datum_pointer->wochentag++);	//Abfrage ob Überlauf und dann Wert setzen
                display_update = 1;																		// Datum hat sich geändert
            }
            else if ( get_key_press( 1<<KEY_DOWN ) | get_key_rpt(1<< KEY_DOWN))							// Taste DOWN gedrückt
            {
                (datum_pointer->wochentag == 1) ? (datum_pointer->wochentag =7) : (datum_pointer->wochentag--);	//Abfrage ob Überlauf und dann Wert setzen
                display_update = 1;																		// Datum hat sich geändert
            }
            break;
        }
		/*************************************************************************
		* Tag wird geändert
		*************************************************************************/
        case 1: 
        {
            if (get_key_press( 1<<KEY_UP) | get_key_rpt(1<< KEY_UP))									// Taste UP gedrückt
            {
                (datum_pointer->tag == 31) ? (datum_pointer->tag =1) : (datum_pointer->tag++);	// Abfrage ob Überlauf und dann Wert setzen
                display_update =1;																		// Datum hat sich geändert
            }
            else if ( get_key_press( 1<<KEY_DOWN ) | get_key_rpt(1<< KEY_DOWN))							// Taste DOWN gedrückt
            {
                (datum_pointer->tag== 1) ? (datum_pointer->tag =31) : (datum_pointer->tag--);		// Abfrage ob Überlauf und dann Wert setzen
                display_update = 1;																		// Datum hat sich geändert
            }
            break;
        }
		/*************************************************************************
		* Monat wird geändert
		*************************************************************************/
        case 2: 
        {
            if ( get_key_press( 1<<KEY_UP) | get_key_rpt(1<< KEY_UP))									// Taste UP gedrückt
            {
                (datum_pointer->monat == 12) ? (datum_pointer->monat =1) : (datum_pointer->monat++); // Abfrage ob Überlauf und dann Wert setzen
                display_update =1;																		// Datum hat sich geändert
            }
            if ( get_key_press( 1<<KEY_DOWN ) | get_key_rpt(1<< KEY_DOWN))								// Taste DOWN gedrückt
            {
                (datum_pointer->monat == 1) ? (datum_pointer->monat =12) : (datum_pointer->monat--); // Abfrage ob Überlauf und dann Wert setzen
                display_update = 1;																		// Datum hat sich geändert
            }
            break;
        }
		/*************************************************************************
		* Jahr wird geändert
		*************************************************************************/
        case 3: 
        {
            if ( get_key_press( 1<<KEY_UP) | get_key_rpt(1<< KEY_UP))									// Taste UP gedrückt
            {
                (datum_pointer->jahr == 99) ? (datum_pointer->jahr =0) : (datum_pointer->jahr++); // Abfrage ob Überlauf und dann Wert setzen
                display_update =1;																		// Datum hat sich geändert
            }
            if ( get_key_press( 1<<KEY_DOWN ) | get_key_rpt(1<< KEY_DOWN))								// Taste DOWN gedrückt
            {
                (datum_pointer->jahr == 0) ? (datum_pointer->jahr =99) : (datum_pointer->jahr--); // Abfrage ob Überlauf und dann Wert setzen
                display_update = 1;																		// Datum hat sich geändert
            }
            break;
        }
        default :
            break;
        }
    }
    lcd_command(LCD_CLEAR);	// LCD Anzeige löschen
}
/*************************************************************************
 * menue_temperatur_offset(..) - Temperatur Offset des LM76 einstellen
 * PE:int8_t * offset - Temperatur Versatz in +-0.1 Grad
 * PA:void
 *************************************************************************/
void menue_temperatur_offset( int8_t *offset)
{
    uint8_t offset_alt;            // Hilfsvariable falls Offset nicht gespeichert werden soll
    uint32_t aktuelle_temperatur;  //Hilfsvariable

    offset_alt = *offset;          // Offset zwischenspeichern, falls Einstellungen verworfen werden sollen

    lcd_printlc(1,1,(unsigned char*) "Temperatur Offset:");
    while(1)
    {
        aktuelle_temperatur = get_lm76_temperatur(LM76_ADRESSE);				// Temperatur vom LM76 holen
        temperatur_to_string(aktuelle_temperatur,anzeigetext1);					// Temperatur String erzeugen
        lcd_printlc(2,1,(unsigned char*) "LM76:       ");						// String ausgeben
        lcd_print((unsigned char*) anzeigetext1);								// Temperatur String ausgeben
        lcd_printlc(3,1,(unsigned char*) "Korr.. Temp: ");					    // String ausgeben
        temperatur_to_string((aktuelle_temperatur + *offset),anzeigetext1);		// Korrigierte Temperatur String erzeugen
        lcd_print((unsigned char*) anzeigetext1);								// Korrigierte Temperatur ausgeben
        temperatur_to_string(*offset,anzeigetext1);								// Offset String erzeugen
        lcd_printlc(4,1,(unsigned char*) "Offset:     ");						// String ausgeben
        lcd_print((unsigned char*) anzeigetext1);								// Offset String ausgeben
        lcd_print((unsigned char*) "  ");										// String ausgeben
        if (get_key_press( 1<<KEY_UP) | get_key_rpt(1<< KEY_UP)) *offset+=2;    // Wenn Taste UP gedrückt -> Offsetwert erhöhen, 2 -> da Temperatur auf 0.1 Grad gerundet
        if (get_key_press( 1<<KEY_DOWN) | get_key_rpt(1<< KEY_DOWN)) *offset-=2;// Wenn Taste DOWN gedrückt -> Offsetwert verringern, 2 -> da Temperatur auf 0.1 Grad gerundet
        if (get_key_press( 1<<KEY_OK) | get_key_rpt(1<< KEY_OK))				// Wenn Taste OK gedrückt -> Offset speichern und Menue verlassen
        {
            lcd_command(LCD_CLEAR);
            lcd_printlc(1,1,(unsigned char*) "Speichere Daten");
			cli(); // Interrupts global ausschalten das EEPROM Zugriffe zeitkritisch
            eeprom_write_byte(&temperatur_offset_eeprom, *offset);				// Schreibe Offset ins EEPROM
			sei();	// Interrupts global einschalten
            _delay_ms(500);
            lcd_command(LCD_CLEAR);
            break;     // Menue verlassen
        }
        if (get_key_press( 1<<KEY_ESC) | get_key_rpt(1<< KEY_ESC))				 // Wenn Taste ESC gedrückt -> Alten Offset behalten und Menue verlassen
        {
            *offset = offset_alt; // Neuen Offset verwerfen
            break;     // Menue verlassen
        }
    }
}
/*************************************************************************
 * menue_schalttemperaturen_einstellen(..) -  Menü um die Schalttemperaturen vom Heizer, Lüfter und Alarmschwelle zu ändern
 * PE:unsigned char * text1 // Text der an LCD Position (2,1) angezeigt wird
 * PE:unsigned char * text2 // Text der an LCD Position (3,1) angezeigt wird
 * PE:unsigned char * text3 // Text der an LCD Position (4,1) angezeigt wird
 * PE:schalt_temperaturen * schalt_aktuell // Schalttemperatur die geändert werden soll
 * PE:schalt_temperaturen * eeprom // EEPROM Speicherplatz der Schalttemperatur
 * PA:void
 *************************************************************************/
void menue_schalttemperaturen_einstellen( unsigned char *text1, unsigned char *text2, unsigned char *text3, schalt_werte *schalt_aktuell,schalt_werte *eeprom)
{
    uint8_t display_update =1, cursor_position =2;
    struct schalt_werte alt;
    char buffer[7];
    alt = *schalt_aktuell;
    *buffer = NULL;

    lcd_printlc(2,1,(unsigned char*) text1);  // übergebenen String auf Linie 1 ausgeben
    lcd_printlc(3,1, (unsigned char*) text2); // übergebenen String auf Linie 1 ausgeben
    lcd_printlc(4,1, (unsigned char*) text3); // übergebenen String auf Linie 1 ausgeben
    while(1)
    {

        lcd_gotolr(cursor_position,17);										// Cursor Position auf  Temperatur.max im LCD
        lcd_command(LCD_CURSORON | LCD_BLINKINGON);							// Blinkender Cursor
        if (get_key_short(1<<KEY_RIGHT)) (cursor_position==3) ? (cursor_position = 2) : (cursor_position++);		// Cursor auf entsprechende Einstellung setzen
        else if (get_key_short(1<<KEY_LEFT)) (cursor_position==2) ? (cursor_position = 3) : (cursor_position--);	// Cursor auf entsprechende Einstellung setzen
        else if(get_key_press(1<<KEY_OK)) // Werte abspeichern und Menü verlassen
        {
            lcd_command(LCD_CURSOROFF | LCD_BLINKINGOFF);
            lcd_command(LCD_CLEAR);
            lcd_printlc(1,1,(unsigned char*) "Speichere Daten");
			cli(); // Interrupts global ausschalten das EEPROM Zugriffe zeitkritisch
            eeprom_write_block (schalt_aktuell, eeprom, sizeof(schalt_werte)); // Schalttemperaturen im EEPROM speichern
			sei();	// Interrupts global einschalten
            _delay_ms(500);
            lcd_command(LCD_CLEAR); // LCD Anzeige löschen
            break;
        }
        else if(get_key_press(1<<KEY_ESC)) // Menü verlassen ohne die neuen Werte abzuspeichern
        {
            *schalt_aktuell = alt;  // Neue Werte verwerfen
            break; 	// Funktion verlassen
        }

        if(display_update)  // Wenn sich die Werte geändert haben Display neu schreiben
        {
            display_update =0;  
            temperatur_to_string(schalt_aktuell->max, buffer); // in String konvertieren
            lcd_printlc(2,14, (unsigned char*) buffer); // Schalttemperatur max anzeigen
            temperatur_to_string(schalt_aktuell->min, buffer); // in String konvertieren
            lcd_printlc(3,14, (unsigned char*) buffer); // Schalttemperatur min anzeigen
            // Hysterese -> Betrag zwischen max und min Wert berechnen und in String konvertieren
            (schalt_aktuell->max >= schalt_aktuell->min) ? (temperatur_to_string(schalt_aktuell->max-schalt_aktuell->min, buffer)) :(temperatur_to_string(schalt_aktuell->min-schalt_aktuell->max, buffer));
            lcd_printlc(4,14, (unsigned char*) buffer); // Hysterese anzeigen
            lcd_print(" "); //
        }
		/*************************************************************************
		* Max oder höhere Temperatur einstellen
		*************************************************************************/
        switch (cursor_position)
        {
        case 2:
        {
            if (get_key_press( 1<<KEY_UP) | get_key_rpt(1<< KEY_UP))									// Taste UP gedrückt
            {
                (schalt_aktuell->max == 600) ? (schalt_aktuell->max =0) : (schalt_aktuell->max += 2);	// Abfrage ob Minutenüberlauf und dann Wert setzen
                display_update =1;																		// Zeit hat sich geändert
            }
            else if ( get_key_press( 1<<KEY_DOWN ) | get_key_rpt(1<< KEY_DOWN))							// Taste DOWN gedrückt
            {
                (schalt_aktuell->max== 0) ? (schalt_aktuell->max =600) : (schalt_aktuell->max-= 2);		// Abfrage ob Minutenüberlauf und dann Wert setzen
                display_update = 1;																		// Zeit hat sich geändert
            }
			break;
        }
		/*************************************************************************
		* Min oder niedrige Temperatur einstellen
		*************************************************************************/
        case 3:
        {
            if (get_key_press( 1<<KEY_UP) | get_key_rpt(1<< KEY_UP))									// Taste UP gedrückt
            {
                (schalt_aktuell->min == 600) ? (schalt_aktuell->min =0) : (schalt_aktuell->min += 2);		// Abfrage ob Minutenüberlauf und dann Wert setzen
                display_update =1;																		// Zeit hat sich geändert
            }
            else if ( get_key_press( 1<<KEY_DOWN ) | get_key_rpt(1<< KEY_DOWN))							// Taste DOWN gedrückt
            {
                (schalt_aktuell->min== 0) ? (schalt_aktuell->min =600) : (schalt_aktuell->min -= 2);		// Abfrage ob Minutenüberlauf und dann Wert setzen
                display_update = 1;																		// Zeit hat sich geändert
            }
			break;
        }
		default:
			break;	
        }
    }
}
/*************************************************************************
 * menu_mondlicht_helligkeit_einstellen(..) - Menü um die Helligkeit des Mondlicht LED einzustellen
 * Die PWM Duty Cycle kann von 0-100% einstellt werden
 * PE:uint8_t * helligkeit_pointer // Pointer auf Wert der Helligkeit 0 -100
 * PE:uint8_t * eeprom  // // EEPROM Speicherplatz der Helligkeit
 * PA:void
 *************************************************************************/
void menu_mondlicht_helligkeit_einstellen( uint8_t * helligkeit_pointer, uint8_t *eeprom)
{
	uint8_t display_update =1; // Hilfsvariablen 
	uint8_t helligkeit_alt; 
	helligkeit_alt = *helligkeit_pointer; // Falls der Wert nicht geändert werden soll, aktuelle Helligkeit zwischenspeichern
	
	lcd_printlc(3,1,(unsigned char*)"Helligkeit:   ");
	
	while(true)
	{
	mondlicht_dimmer(*helligkeit_pointer);					// Mondlicht LED einschalten mit aktuellem Helligkeitswert
		
	
	if(display_update)										// Wenn sich die Helligkeit geändert hat
       {
            display_update =0;  //
            sprintf(anzeigetext1, "%03u%%", *helligkeit_pointer);	// Helligkeit in String umwandeln
            lcd_printlr(3,13,(unsigned char*)anzeigetext1);		// Helligkeit ausgeben
        }
        
	 
        if (get_key_press( 1<<KEY_OK) | get_key_rpt(1<< KEY_OK))// Wenn Taste OK gedrückt -> Helligkeit speichern und Menue verlassen
        {
			mondlicht_dimmer(0);							// Mondlicht LED ausschalten
            lcd_command(LCD_CLEAR);
            lcd_printlc(1,1,(unsigned char*) "Speichere Daten");
			cli();
            eeprom_write_byte(eeprom, *helligkeit_pointer);	// Schreibe Helligkeit in EEPROM
			sei();
            _delay_ms(500);
            lcd_command(LCD_CLEAR);
            break;											// Menue verlassen
        }
        if (get_key_press( 1<<KEY_ESC) | get_key_rpt(1<< KEY_ESC))// Wenn Taste ESC gedrückt -> Alte Helligkeit behalten und Menue verlassen
        {
            *helligkeit_pointer = helligkeit_alt;			// Neue Helligkeit verwerfen
			mondlicht_dimmer(0);							// Mondlicht LED ausschalten
            break;											// Menue verlassen
        }
		
		if (get_key_press( 1<<KEY_UP) | get_key_rpt(1<< KEY_UP))// Taste UP gedrückt
        {
            (*helligkeit_pointer == 100) ? (*helligkeit_pointer =0) : (*helligkeit_pointer+=1);	//Abfrage ob Helligkeit 100% und dann Wert setzen
            display_update = 1;																	// Helligkeit hat sich geändert
        }
        else if ( get_key_press( 1<<KEY_DOWN ) | get_key_rpt(1<< KEY_DOWN))						// Taste DOWN gedrückt
        {
            (*helligkeit_pointer == 0) ? (*helligkeit_pointer =100) : (*helligkeit_pointer-=1);	//Abfrage ob Helligkeit 0% und dann Wert setzen
            display_update = 1;																	// Helligkeit hat sich geändert
        }
	}			
}
/*************************************************************************
 * menu_phwert_klaibrieren(..) - Menü um die Sonde für den pH Wert zu kalibrieren
 * Die Sonde wird in die einzelnen Referenz Lösungen getaucht und dann 1 Min gewartet um 
 * einen stabilen Wert zu bekommen, dann wird der ADC Wert ausgelesen. 
 * Um Schluss werden die Funktionen zur Kalibrierung des pH Wertes berechnet
 * PA:void
 *************************************************************************/
void menu_phwert_kalibrieren(void)
{	
	uint16_t adc_ph4, adc_ph7, adc_ph9;  // ADC Werte Zwischenspeicher
	
	lcd_command(LCD_CLEAR);
	lcd_printlc(1,1,(unsigned char*) strcpy_P (anzeigetext,PSTR("pH Sonde kalibrieren")));	// String ausgeben
	lcd_printlc(2,1,(unsigned char*) strcpy_P (anzeigetext,PSTR("OK kalib. starten")));		// String ausgeben
	lcd_printlc(3,1,(unsigned char*) strcpy_P (anzeigetext,PSTR("ESC kalib. abbrechen")));	// String ausgeben
	while(1)
	{
		if(get_key_press(1<<KEY_ESC))  // Funktion verlassen
		{
			lcd_command(LCD_CLEAR);
			 break;	
		}			 
        if (get_key_press( 1<<KEY_OK) | get_key_rpt(1<< KEY_OK))	// Wenn Taste OK gedrückt -> Kalibrierung starten
		{
			/*************************************************************************
			* Zwischen dem Messvorgängen wird immer eine Minute gewartet um des Sonde
			* genügend Zeit zu geben sich dem neuen ph Wert anzupassen
			*************************************************************************/
			timer_counter = WARTEZEIT_PH_KALIBRIEREN;	// Timervariable für Wartezeit
			lcd_printlrc(2,1,(unsigned char*) strcpy_P (anzeigetext,PSTR("Sonde in pH4.0      Loesung tauchen      ")));// String ausgeben
			do											// in Schleife warten bis Zeit abgelaufen
			{
				ultoa(timer_counter-1, anzeigetext1,10);// Timervariable zu ASCII, -1 da sonst Timer == 0 nicht angezeigt wird
				lcd_printlc(4,1,(unsigned char*) anzeigetext1);// String ausgeben	
				lcd_print((unsigned char*) strcpy_P (anzeigetext,PSTR("s warten -> OK  ")));// String ausgeben
			}while (timer_counter);
			
			adc_ph4 = adc_read_channel(PH_ADC_CHANNEL); // ADC Wert bei ph 4.0 auslesen
			get_key_press( 1<<KEY_OK);					// Falls in der Schleife irgendwann mal OK gedrückt wurde -> Taste löschen
			while((!get_key_press( 1<<KEY_OK)));		// Warten bis OK gedrückt
			/************************************************************************/
			timer_counter = WARTEZEIT_PH_KALIBRIEREN;	// Timervariable für Wartezeit 
			lcd_printlrc(2,1,(unsigned char*) strcpy_P (anzeigetext,PSTR("Sonde in pH7.0      Loesung tauchen")));// String ausgeben
			do											// in Schleife warten bis Zeit abgelaufen
			{
				ultoa(timer_counter-1, anzeigetext1,10);// Timervariable zu ASCII, -1 da sonst Timer == 0 nicht angezeigt wird
				lcd_printlc(4,1,(unsigned char*) anzeigetext1);// String ausgeben	
				lcd_print((unsigned char*) strcpy_P (anzeigetext,PSTR("s warten -> OK  ")));// String ausgeben
			}while (timer_counter);
			adc_ph7 = adc_read_channel(PH_ADC_CHANNEL); // ADC Wert bei ph 7.0 auslesen
			get_key_press( 1<<KEY_OK);					// Falls in der Schleife irgendwann mal OK gedrückt wurde -> Taste löschen
			while((!get_key_press( 1<<KEY_OK)));
			/************************************************************************/
			timer_counter = WARTEZEIT_PH_KALIBRIEREN;	// Timervariable für Wartezeit 
			lcd_printlrc(2,1,(unsigned char*) strcpy_P (anzeigetext,PSTR("Sonde in pH9.0      Loesung tauchen")));// String ausgeben
			do											// in Schleife warten bis Zeit abgelaufen
			{
				ultoa(timer_counter-1, anzeigetext1,10); // Timervariable zu ASCII, -1 da sonst Timer == 0 nicht angezeigt wird
				lcd_printlc(4,1,(unsigned char*) anzeigetext1);// String ausgeben	
				lcd_print((unsigned char*) strcpy_P (anzeigetext,PSTR("s warten -> OK  ")));// String ausgeben
			}while (timer_counter);
			adc_ph9 = adc_read_channel(PH_ADC_CHANNEL); // ADC Wert bei ph 7.0 auslesen
			/************************************************************************/
			// Eigentliche Kalibrierung -> Funktionswerte Steigung und Nullversatz berechnen
			phwert_kalibrieren(&adc_ph4, &adc_ph7, &adc_ph9, &phwert_referenzen); 
			get_key_press( 1<<KEY_OK);					// Falls in der Schleife irgendwann mal OK gedrückt wurde -> Taste löschen
			
			lcd_printlrc(2,1,(unsigned char*) strcpy_P (anzeigetext,PSTR("Kalibrierung        abgeschlossen")));// String ausgeben
			lcd_printlr(4,1,(unsigned char*) strcpy_P (anzeigetext,PSTR("ESC druecken     ")));// String ausgeben	
		}
	}			
}
/*************************************************************************
* Da den Funktionspointern im Menü keine Parameter mitgegeben werden können,
* werden über das Menü die "funkt_" Funktionen aufgerufen und so die eigentlichen 
* Menüfunktionen mit Parameterübergabe aufgerufen.
*************************************************************************/

/*************************************************************************
 * funkt_menu_temperatur_offet(..) - Funktionsmenü zur Einstellung des
 * Temperatur Offset des LM 76
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_temperatur_offet(void)
{
    lcd_command(LCD_CLEAR);
	// Eigentliches Menü für die Einstellung des Offsets mit entsprechendem Übergabewert aufrufen
    menue_temperatur_offset(&temperatur_offset);
    lcd_command(LCD_CLEAR);
}
/*************************************************************************
 * funkt_menu_heizer(..) - Funktionsmenü zur Einstellung der Schalttemperaturen
 * des Heizstabes
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_heizer(void)
{
    lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Heizer Einstellungen"))); // Menüstring anzeigen
	// Eigentliches Menü für die Einstellung der Schalttemperaturen mit entsprechenden Texten und Übergabewerten aufrufen
    menue_schalttemperaturen_einstellen(strcpy_P (anzeigetext,PSTR("Heizer ein:")), strcpy_P (anzeigetext1,PSTR("Heizer aus:")), strcpy_P (anzeigetext2,PSTR("Hysterese:")),&heizer, &heizer_eeprom);
    lcd_command(LCD_CLEAR);
}
/*************************************************************************
 * funkt_menu_luefter(..) - Funktionsmenü zur Einstellung der Schalttemperaturen
 * des Lüfters
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_luefter(void)
{
    lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Luefter einstellung"))); // Menüstring anzeigen
	// Eigentliches Menü für die Einstellung der Schalttemperaturen mit entsprechenden Texten und Übergabewerten aufrufen
    menue_schalttemperaturen_einstellen(strcpy_P (anzeigetext,PSTR("Luefter ein:")), (unsigned char*)strcpy_P (anzeigetext1,PSTR("Luefter aus:")), (unsigned char*)strcpy_P (anzeigetext2,PSTR("Hysterese:")),&luefter, &luefter_eeprom);
    lcd_command(LCD_CLEAR);
}
/*************************************************************************
 * funkt_menu_alarm(..) - Funktionsmenü zur Einstellung der maximalen und
 * minimalen Temperatur im Aquarium
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_alarm(void)
{
    lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Temperatur Min/Max:"))); // Menüstring anzeigen
	// Eigentliches Menü für die Einstellung der Temperaturen mit entsprechenden Texten und Übergabewerten aufrufen
    menue_schalttemperaturen_einstellen(strcpy_P (anzeigetext,PSTR("Temp. Max:")), (unsigned char*)strcpy_P (anzeigetext1,PSTR("Temp. Min:")), (unsigned char*)strcpy_P (anzeigetext2,PSTR("Hysterese:")),&alarm, &alarm_eeprom);
    lcd_command(LCD_CLEAR);
}
/*************************************************************************
 * funkt_menu_datum(..) - Funktionsmenü zur Einstellung des aktuellen
 * Datums des DS 1307
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_datum(void)
{
    lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Aktuelles Datum     eingeben")));
    menue_datum_einstellen(&zeit);
}
/*************************************************************************
 * funkt_menu_uhr(..) - Funktionsmenü zur Einstellung der aktuellen 
 * Uhrzeit des DS 1307
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_uhr(void)
{
    lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Aktuelle Uhrzeit    eingeben"))); // Menüstring anzeigen
	// Eigentliches Menü für die Einstellung des Datums mit entsprechenden Übergabewerten aufrufen	
	// hier wird nichts ins EEPROM geschrieben darum EEPROM Adresse auf 0 -> wird im Menü entsprechend abgefragt
    menue_zeiten_einstellen(&zeit,NULL );
}
/*************************************************************************
 * funkt_menu_lampe_ein_am(..) - Funktionsmenü zur Einstellung der 
 * Einschaltzeit für die Lampe
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_lampe_ein_am(void)
{
    lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Zeit AM Einstell.   Lampe einschalten:")));
    menue_zeiten_einstellen(&lampe_on1, &lampe_on_eeprom1 );
}
/*************************************************************************
 * funkt_menu_lampe_aus_am(..) - Funktionsmenü zur Einstellung der 
 * Ausschaltzeit für die Lampe
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_lampe_aus_am(void)
{   
    lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Zeit AM Einstell.   Lampe ausschalten:"))); // Menüstring anzeigen
	// Eigentliches Menü für die Einstellung der Einschaltzeit der Lampe  mit entsprechenden Übergabewerten aufrufen
    menue_zeiten_einstellen(&lampe_off1, &lampe_off_eeprom1 );
}
/*************************************************************************
 * funkt_menu_lampe_ein_am(..) - Funktionsmenü zur Einstellung der 
 * Einschaltzeit für die Lampe
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_lampe_ein_pm(void)
{
    lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Zeit PM Einstell.   Lampe einschalten:")));
    menue_zeiten_einstellen(&lampe_on2, &lampe_on_eeprom2 );
}
/*************************************************************************
 * funkt_menu_lampe_aus_am(..) - Funktionsmenü zur Einstellung der 
 * Ausschaltzeit für die Lampe
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_lampe_aus_pm(void)
{   
    lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Zeit PM Einstell.   Lampe ausschalten:"))); // Menüstring anzeigen
	// Eigentliches Menü für die Einstellung der Einschaltzeit der Lampe  mit entsprechenden Übergabewerten aufrufen
    menue_zeiten_einstellen(&lampe_off2, &lampe_off_eeprom2 );
}
/*************************************************************************
 * funkt_menu_futterstop(..) - Funktionsmenü zur Einstellung der 
 * Zeit wie lange die Pumpe beim Fütterungsstopp ausgeschaltet werden soll
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_futterstop(void)
{
	lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Futter Stopp Dauer  eingeben:"))); // Menüstring anzeigen
	// Eigentliches Menü für die Einstellung der Fütterungsstoppzeit mit entsprechenden Übergabewerten aufrufen
    menue_zeiten_einstellen(&futterstopp, &futterstopp_eeprom );
	
}
/*************************************************************************
 * funkt_menu_mondlicht_ein(..) - Funktionsmenü zur Einstellung der 
 * Einschaltzeit des Mondlichts
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_mondlicht_ein(void)
{
    lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Zeit Einstellungen  Mondlicht einstellen:"))); // Menüstring anzeigen
	// Eigentliches Menü für die Einstellung der Einschaltzeit mit entsprechenden Übergabewerten aufrufen
    menue_zeiten_einstellen(&ml_on, &ml_on_eeprom );
}
/*************************************************************************
 * funkt_menu_mondlicht_aus(..) - Funktionsmenü zur Einstellung
 * der Ausschaltzeit des Mondlichts
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_mondlicht_aus(void)
{
    lcd_command(LCD_CLEAR);
    lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Zeit Einstellungen  Mondlicht ausschalten:"))); // Menüstring anzeigen
	// Eigentliches Menü für die Einstellung der Ausschaltzeit mit entsprechenden Übergabewerten aufrufen
    menue_zeiten_einstellen(&ml_off, &ml_off_eeprom );
    lcd_command(LCD_CLEAR);
}
/*************************************************************************
 * funkt_menu_mondlicht_helligkeit(..) - Funktionsmenü für die Dimmung 
 * des Mondlichts
 * PE:void
 * PA:void
*************************************************************************/
void funkt_menu_mondlicht_helligkeit(void)
{
	lcd_command(LCD_CLEAR);
	lcd_printlc(1,1,(unsigned char*) strcpy_P (anzeigetext,PSTR("Mondlicht Helligkeit")));	// Menüstring anzeigen
	lcd_printlc(2,1,(unsigned char*) strcpy_P (anzeigetext,PSTR("anpassen: ")));			// Menüstring anzeigen
	// Eigentliches Menü für die Einstellung der Dimmung mit entsprechenden Übergabewerten aufrufen
	menu_mondlicht_helligkeit_einstellen(&mondlicht_helligkeit, &mondlicht_helligkeit_eeprom);
	lcd_command(LCD_CLEAR);
}	
/*************************************************************************
 * funkt_menu_phwert_kalibrieren(..) - Funktionsmenü für die Kalibrierung der
 * pH Sonde
 * PE:void
 * PA:void
 *************************************************************************/
void funkt_menu_phwert_kalibrieren(void)
{
	menu_phwert_kalibrieren(); // Eigentliches Menü für die Kalibrierung der pH Sonde aufrufen
	cli(); // Interrupts global aus da EEPROM Zugriffe zeitkritisch
	eeprom_write_block (&phwert_referenzen, &phwert_referenzen_eeprom, sizeof(Ph_reverenz)); // pH Wert Referenzen im EEPROM speichern
	sei(); // Interrupt global wieder freigeben
}
/*************************************************************************
 * funkt_menu_about(..) - Abouttext für Hard und Softwareversionen anzeigen
 * PE:void
 * PA:void
 *************************************************************************/
void funkt_menu_about(void)
{
	lcd_command(LCD_CLEAR);
	lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("uQuarium Diplomarb.")));
	lcd_printlrc(2,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("Stefan Keller")));
	lcd_printlrc(3,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("HW M:V1.0 F:V1.0")));
	lcd_printlrc(4,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("SW:V1.2")));
	
	while (true)
	{
		//if (get_key_press(ALL_KEYS))
		//{
			//lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("     ********   ")));
		//}
		if(get_key_press(ALL_KEYS)) break; // Irgendeine Taste gesrückt-> Menü verlassen
	}
	lcd_command(LCD_CLEAR);
}
/*************************************************************************
 * funkt_menu_max_werte(..) - Menüanzeige der Tages Maximalwerte von Temperatur und pH Wert
 * PE:void
 * PA:void
 *************************************************************************/
void funkt_menu_max_werte(void)
{
	lcd_command(LCD_CLEAR);
	// Tagesmaximum Temperatur und Zeitpunkt in String umwandeln und anzeigen
	lcd_printlrc(1,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("T-Max:")));
	temperatur_to_string(extrem_temperatur.max_wert,anzeigetext1);
    lcd_print((unsigned char*) anzeigetext1);
	zeit_to_string(extrem_temperatur.max_zeitpunkt, anzeigetext1);
    lcd_printlrc(1,13,(unsigned char*)anzeigetext1);
	// Tagesminimum Temperatur und Zeitpunkt in String umwandeln und anzeigen
	lcd_printlrc(2,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("T-Min:")));
	temperatur_to_string(extrem_temperatur.min_wert,anzeigetext1);
    lcd_print((unsigned char*) anzeigetext1);
	zeit_to_string(extrem_temperatur.min_zeitpunkt, anzeigetext1);
	lcd_printlrc(2,13,(unsigned char*)anzeigetext1);
	// Tagesmaximum pH Wert und Zeitpunkt in String umwandeln und anzeigen
	lcd_printlrc(3,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("pHMax:")));
	phwert_to_string(extrem_ph.max_wert, anzeigetext1);
	lcd_print((unsigned char*) anzeigetext1);	
	zeit_to_string(extrem_ph.max_zeitpunkt, anzeigetext1);
	lcd_printlrc(3,13,(unsigned char*)anzeigetext1);
	// Tagesminimum pH Wert und Zeitpunkt in String umwandeln und anzeigen
	lcd_printlrc(4,1,(unsigned char *)strcpy_P (anzeigetext,PSTR("pHMin:")));
	phwert_to_string(extrem_ph.min_wert, anzeigetext1);
	lcd_print((unsigned char*) anzeigetext1);
	zeit_to_string(extrem_ph.min_zeitpunkt, anzeigetext1);
	lcd_printlrc(4,13,(unsigned char*)anzeigetext1);
	
	while (true)
	{
		if(get_key_press(ALL_KEYS)) break;
	}
	lcd_command(LCD_CLEAR);
}
/*************************************************************************
 * vergleiche_temperaturen(..) -  Temperatur mit mit min/max Werten vergleichen und Ergebnis zurückgeben
 * PE:uint32_t * minimum // Minimum Temperatur mit der verglichen wird
 * PE:uint32_t * maximum // Maximum Temperatur mit der verglichen wird
 * PE:uint32_t * aktuelle_temperatur // Aktuelle Temperatur des Wassers mit der verglichen wird
 * PA:uint8_t // Ergebnis zurückgeben 1 -> höher als max 2 -> kleiner als min, 0 -> Ergebnis zwischen den Temperaturen
 *************************************************************************/
uint8_t vergleiche_temperaturen(int16_t *minimum, int16_t *maximum, int16_t *aktuelle_temperatur)
{
    if (*aktuelle_temperatur > *maximum) // Temperatur höher als maximum
    {
        return 1;
    }
    else if ( *aktuelle_temperatur < *minimum) // Temperatur kleiner als minimum
    {
        return 2;
    }
    else return 0;  // Keines von beiden
}
/*************************************************************************
 * vergleiche_zeiten(..) - Vergleiche die Schaltzeiten der Ausgänge mit der aktuellen Zeit und gibt das Ergebnis zurück
 * PE:struct uhr * ein_zeit // Pointer auf struct der Einschaltzeit
 * PE:struct uhr * aus_zeit // Pointer auf struct der Ausschaltzeit
 * PE:struct uhr * aktuelle_zeit // Pointer auf struct der aktuellen Zeit
 * PA:uint8_t // 0 -> Aktuelle Zeit liegt ausserhalb den Schaltzeiten -> aus, 1 -> Aktuelle Zeit liegt zwischen den Schaltzeiten -> ein
 *************************************************************************/
uint8_t vergleiche_zeiten(struct uhr *ein_zeit, struct uhr *aus_zeit, struct uhr *aktuelle_zeit)
{
    uint32_t ein, aus, aktuell; // Hilfsvariablen für Sekunden seit Mitternacht
    // 
	/*************************************************************************
	* Um die Zeiten direkt miteinander vergleichen zu können, 
	* werden die Sekunden seit Mitternacht berechnet
	*************************************************************************/
    ein = ((uint32_t)ein_zeit->stunde * 3600)+((uint32_t)ein_zeit->minute * 60)+((uint32_t)ein_zeit->sekunde); // Einschaltzeit Sekunden seit Mitternacht
    aus = ((uint32_t)aus_zeit->stunde * 3600)+((uint32_t)aus_zeit->minute * 60)+((uint32_t)aus_zeit->sekunde); // Ausschaltzeit Sekunden seit Mitternacht
    aktuell = ((uint32_t)aktuelle_zeit->stunde * 3600)+((uint32_t)aktuelle_zeit->minute * 60)+((uint32_t)aktuelle_zeit->sekunde); // Aktuelle Zeit Sekunden seit Mitternacht

    if ( aus < ein) // Wenn der Ausgang über Mitternacht eingeschaltet werden soll -> Auszeit kleiner als Einzeit
    {
		// So muss die Aktuelle Zeit grösser als ein Einzeit sein ODER kleiner als die Ausschaltzeit, dann ist der Verbraucher eingeschaltet
        return ((aktuell > ein) || (aktuell < aus)); 
    }
    else // Wenn der Ausgang innerhalb eines Tages eingeschaltet werden soll -> Auszeit grösser als Einzeit
    {
		// So muss die Aktuelle Zeit grösser als die Einzeit sein UND kleiner als die Auszeit, dann ist der Verbraucher eingeschaltet
        return ((aktuell > ein) && (aktuell < aus)); 
    }
}
/*************************************************************************
 * lampen_schalten(..) - Zustandsbit für die zeitgesteuerten 
 * Ausgänge setzen
 * PE:void
 * PA:void
*************************************************************************/
void lampen_schalten(void)
{	
	/*************************************************************************
	* Zustandsbit für die Lampe setzen
	*************************************************************************/
	// Ein/Ausschaltzeiten mit aktuelle Uhrzeit vergleichen
    if (vergleiche_zeiten(&lampe_on1,&lampe_off1,&zeit) || vergleiche_zeiten(&lampe_on2,&lampe_off2,&zeit)) 
    {
        zustandsbit.lampe =1; // Lampe ein
    }
    else
    {
        zustandsbit.lampe =0; // Lampe aus
    }
	
	/*************************************************************************
	* Zustandsbit für die Mondlicht setzen
	*************************************************************************/
	// Ein/Ausschaltzeiten mit aktuelle Uhrzeit vergleichen
    if (vergleiche_zeiten(&ml_on,&ml_off,&zeit))
    {
        zustandsbit.mondlicht =1; // Mondlicht ein
    }
    else
    {
        zustandsbit.mondlicht =0; // Mondlicht aus
    }
}
/*************************************************************************
 * temperatuen_schalten(..) - Zustandbits für die temperaturgesteuerten
 * Ausgänge und Alarmzustand setzen
 * PE:void
 * PA:void
 *************************************************************************/
void temperatuen_schalten(void)
{
    uint8_t compare_return; // Hilfsvariable für switch case

    /*************************************************************************
    *  Zustandsbit für Lüfter schalten
    *************************************************************************/
	// Wassertemperatur mit min/max Werten vergleichen
    compare_return = vergleiche_temperaturen(&luefter.min,&luefter.max,&temperatur); 
    switch (compare_return)
    {
    case 1:
        zustandsbit.luefter =1; // 1 -> Wasser zu warm Lüfter einschalten
        break;  
    case 2:
        zustandsbit.luefter =0; // 2 -> Wasser zu kalt Lüfter ausschalten
        break;  
    case 0:
        break;	// 0 ->  Wasser noch unterhalb Lüfter Einschalttemperatur -> nichts machen
    default:
        break;
    }
   /*************************************************************************
   * Zustand für Alarm Vektor schalten
   *************************************************************************/
   // Wassertemperatur mit min/max Werten vergleichen, Ergebnis !=0 -> Alarm Vektor entsprechend setzen
    compare_return = (vergleiche_temperaturen(&alarm.min,&alarm.max,&temperatur)); 
    switch (compare_return)
    {
    case 1: 
        alarm_vektor = 2; // 1 -> Wasser zu warm, Alarm für Temperatur zu hoch
        break;  
    case 2:
        alarm_vektor = 3; // 2 -> Wasser zu kalt, Alarm für Temperatur zu niedrig
        break;   
    case 0:
        break; // 0 ->  Normale Wassertemperatur -> nichts machen
    default:
        break;
    }
    /*************************************************************************
    * Zustandbit für Heizer schalten
    *************************************************************************/
	// Wassertemperatur mit min/max Werten vergleichen
    compare_return = vergleiche_temperaturen(&heizer.max,&heizer.min,&temperatur); 
    switch (compare_return)
    {
    case 1:
        zustandsbit.heizer =0;
        break;  // 1 -> Wasser zu warm Heizer ausschalten
    case 2:
        zustandsbit.heizer =1;
        break;  // 2 -> Wasser zu kalt Heizer einschalten
    case 0:
        break;	// 0 ->  Wasser noch oberhalb Heizer Einschalttemperatur -> nichts machen
    default:
        break;
    }
}
/*************************************************************************
 * ausgaenge_ansteuern(..) - Errechnete Schaltzustände sammeln und 
 * die Hardwareausgänge entsprechen dem Modus der Anlage schalten
 * PE:void
 * PA:void
 *************************************************************************/
void ausgaenge_ansteuern(void)
{
	/*************************************************************************
	* Wenn Steuerung in RUN Mode
	*************************************************************************/
	if(zustandsbit.run)  
	{
		RUNLED_ON;								// LED für Zustand RUN einschalten
		if(zustandsbit.heizer) HEIZER_ON;	// Heizer ein oder ausschalten
		else HEIZER_OFF;
		if(zustandsbit.luefter) LUEFTER_ON;	// Lüfter ein oder ausschalten
		else LUEFTER_OFF;
		if(zustandsbit.lampe) LAMPE_ON;		// Lampe ein oder ausschalten
		else LAMPE_OFF;
		// Mondlicht entsprechend den Dimmwert einschalten
		if(zustandsbit.mondlicht) mondlicht_dimmer(mondlicht_helligkeit);
		else mondlicht_dimmer(0); // Mondlicht ausschalten
		
		if (alarm_vektor) // Wenn ein Alarm anliegt -> Alarmgeber einschalten
		{
			if(zustandsbit.esc_alarm == 1)
			{
				BUZZER_OFF;
			}
			else
			{
				BUZZER_ON;		
			}
			ALARMLED_ON;
		}
		else			// Ansonsten Alarmgeber ausschalten
		{
			ALARMLED_OFF;
			BUZZER_OFF;
		}
		/*************************************************************************
		*  Sensorwerte für die Tages Max und Min Werte abfragen und entsprechend schreiben
		*************************************************************************/
		// Abfrage ober bisherige Max und Min  Temperaturen über/unterschritten 
		switch (vergleiche_temperaturen(&extrem_temperatur.min_wert,&extrem_temperatur.max_wert,&temperatur))
		{
		case 1:	// 1-> Wasser wärmer als bisherige Max Wert
			extrem_temperatur.max_wert = temperatur;	// Neuer Max Wert
			extrem_temperatur.max_zeitpunkt = zeit;		// Zeitpunkt mit speichern
			break; 
		case 2: // 2-> Wasser kälter als bisheriger Min Wert
			extrem_temperatur.min_wert = temperatur;	// Neuer Min Wert
			extrem_temperatur.min_zeitpunkt = zeit;		// Zeitpunkt mit speichern
			break;  
		case 0: // Aktuelle Temperatur liegt zwischen den Extremwerten -> nichts machen
			break;	
		default:
			break;
		}
		// Abfrage ober bisherige Max und Min  ph Werte über/unterschritten 
		switch (vergleiche_temperaturen(&extrem_ph.min_wert,&extrem_ph.max_wert,&ph_wert))
		{
		case 1: // ph höher als bisheriger Max Wert
			extrem_ph.max_wert = ph_wert;	// Neuer Max Wert
			extrem_ph.max_zeitpunkt = zeit;	// Zeitpunkt mit speichern
			break;  
		case 2: // pH niedriger als bisheriger Min Wert
			extrem_ph.min_wert = ph_wert;	// Neuer Min Wert
			extrem_ph.min_zeitpunkt = zeit;	// Zeitpunkt mit speichern
			break;  
		case 0: // Aktueller pH Wert liegt zwischen den Extremwerten -> nichts machen
			break;	
		default:
			break;
		}
	}
	/*************************************************************************
	* Wenn Steuerung im Wartungs Mode, alle Verbraucher ausschalten, 
	* bis auf Licht und Pumpe
	*************************************************************************/	
	else
	{
		RUNLED_OFF;
		HEIZER_OFF;
		LUEFTER_OFF;
		LAMPE_ON;
		ALARMLED_OFF;
		BUZZER_OFF;
		mondlicht_dimmer(0);
		
	}
	/*************************************************************************
	* Pumpe kann in jedem Zustand ab und angeschaltet werden
	*************************************************************************/
	if (zustandsbit.pumpe) PUMPE_ON; // Entsprechend  den Zustand Pumpe an und ausschalten
		else PUMPE_OFF;
}
/*************************************************************************
 * futterstop_aktivieren(..) -  Für Pumpenstopp bei der Fütterung wird ein Counter geladen 
 * welcher über den Timer alle 1 sec dekrementiert wird bis Zeit abgelaufen ist
 * PE:struct uhr * stopp_zeit // Pointer auf Zeit wie lange die Pumpe ausgeschaltet werden soll
 * PA:void
 *************************************************************************/
void futterstop_aktivieren(struct uhr *stopp_zeit)
{	
	// Wenn Futterstopp nicht schon aktiviert -> Zeit in Sekunden umrechnen und in die Timervariable schreiben
	if(!timer_counter) timer_counter = ((uint32_t)stopp_zeit->stunde* 3600)+((uint32_t)stopp_zeit->minute * 60)+((uint32_t)stopp_zeit->sekunde); 
	else timer_counter = 0; // Wenn Pumpen schon gestoppt -> Pumpe wieder vorzeitig einschalten
}
/*************************************************************************
 * read_eeprom_daten(..) - Statische Werte nach einen Reset aus dem EEPROM holen
 * PA:void
 *************************************************************************/
void read_eeprom_daten(void)
{	
	cli();																		// Interrupts global ausschalten
																				// Da Eeprom operationen zeitkritisch
    temperatur_offset  = (uint8_t)eeprom_read_byte (&temperatur_offset_eeprom); // Temperatur Offset holen
	mondlicht_helligkeit  = eeprom_read_byte (&mondlicht_helligkeit_eeprom);	// Mondlicht Helligkeit holen
	eeprom_read_block (&phwert_referenzen, &phwert_referenzen_eeprom, sizeof(struct Ph_reverenz)); // Referenzwerte der PH Werte Berechnung holen
    eeprom_read_block (&lampe_on1, &lampe_on_eeprom1, sizeof(struct uhr));		// Einschaltzeit  der Lampe holen
    eeprom_read_block (&lampe_off1, &lampe_off_eeprom1, sizeof(struct uhr));		// Ausschaltzeit der Lampe holen
	eeprom_read_block (&lampe_on2, &lampe_on_eeprom2, sizeof(struct uhr));		// Einschaltzeit  der Lampe holen
	eeprom_read_block (&lampe_off2, &lampe_off_eeprom2, sizeof(struct uhr));		// Ausschaltzeit der Lampe holen
    eeprom_read_block (&ml_on, &ml_on_eeprom, sizeof(struct uhr));				// Einschaltzeit des ML holen
    eeprom_read_block (&ml_off, &ml_off_eeprom, sizeof(struct uhr));			// Ausschaltzeit des MK holen
    eeprom_read_block (&alarm, &alarm_eeprom, sizeof(schalt_werte));			// Schalttemperatur für den Alarm holen
    eeprom_read_block (&heizer, &heizer_eeprom, sizeof(schalt_werte));			// Schalttemperatur für den Heizer holen
    eeprom_read_block (&luefter, &luefter_eeprom, sizeof(schalt_werte));		// Schalttemperatur für den Lüfter holen
	eeprom_read_block (&futterstopp, &futterstopp_eeprom, sizeof(schalt_werte)); // Schalttemperatur für den Lüfter holen
	sei();																		// Interrupts global wieder freigeben
}
/*************************************************************************
 * zustand_lcd(..) - Zeichen generieren um die Schaltzustände der Ausgänge auf den Display anzuzeigen
 * PE:uint8_t zustand // 1 = Ein , 0 = Aus
 * PA:uint8_t			// Hex Zeichen retournieren Ein=^  Aus=_
 *************************************************************************/
uint8_t zustand_lcd(uint8_t zustand)
{
    if (zustand) // Ausgang eingeschaltet
    {
        return 0x5E; // Display Anzeige ASCII "^"
    }
    else
    {
        return 0x5F; // Display Anzeige ASCII "_"
    }

}
/*************************************************************************
 * send_to_uart(..) - Messwerte über RS 232 an den PC senden
 * PE:void
 * PA:void
 *************************************************************************/
void send_to_uart(void )
{
    //temperatur_to_string(temperatur,anzeigetext1);	// Temperatur zu String
	//uart_puts(anzeigetext1);						// Temperatur über UART senden
    //uart_putc('\r');								// New Line
	//phwert_to_string(ph_wert, anzeigetext1);		// pH Wert zu String		
    //uart_puts(anzeigetext1);						// pH Wert über UART senden
    //uart_putc('\r');								// New Line
	//zeit_to_string(zeit, anzeigetext1);				// Zeit zu String
	//uart_puts(anzeigetext1);						// Zeit über UART senden
    //uart_putc('\r');								// New Line
	//datum_to_string(zeit, anzeigetext1);			// Datum zu string
	//uart_puts(anzeigetext1);						// Datum über UART senden
	//uart_putc('\r');								// New Line
}
/*************************************************************************
 * mondlicht_dimmer(..) - Helligkeit des Mondlichts mittels PWM Timer 1B einstellen
 * PE:uint8_t  helligkeit // Helligkeit in Prozent 0=aus, 50=50%, 100=100%
 * PA:void
 *************************************************************************/
void mondlicht_dimmer(uint8_t helligkeit)
{
	static uint16_t pwm_wert;							// Wert für den compare-match des Timers 0-255
	pwm_wert = (((uint16_t) helligkeit * 255) /100);	// Helligkeit in Prozent in 0-255 für den compare match umrechnen
	OCR1B = pwm_wert;									// Timer schaltet den OC1B Ausgang beim angegebenen pwm_wert um
}
/*************************************************************************
 * reset_tageswerte(..) - Um Mitternacht werden die Tages Min/Max Werte zurückgesetzt
 * PE:void
 * PA:void
 *************************************************************************/
void reset_tageswerte(void)
{
	static uint8_t alter_tag; // Hilfsvariable um festzustellen ob ein neuer Tag angefangen hat
	
	if(alter_tag != zeit.tag) // Feststellen ob das Datum des Tages geändert hat
	{
		extrem_temperatur.max_wert = temperatur;	// Max auf aktuelle Temperatur setzen
		extrem_temperatur.min_wert = temperatur;	// Min auf aktuelle Temperatur setzen
		extrem_temperatur.max_zeitpunkt = zeit;		// Zeitpunkt resetten -> ist immer 00:00:00
		extrem_temperatur.min_zeitpunkt = zeit;		// Zeitpunkt resetten -> ist immer 00:00:00
		extrem_ph.max_wert = ph_wert;				// Max auf aktuellen pH Wert  setzen
		extrem_ph.min_wert = ph_wert;				// Min auf aktuellen pH Wert  setzen
		extrem_ph.max_zeitpunkt = zeit;				// Zeitpunkt resetten -> ist immer 00:00:00
		extrem_ph.min_zeitpunkt = zeit;				// Zeitpunkt resetten -> ist immer 00:00:00
		alter_tag = zeit.tag;						// Datum des neuen Tages merken
	}	
}
/*************************************************************************
 * main_anzeige(..) - Hauptanzeige Welche die aktuellen Informationen 
 * auf den LCD anzeigt
 * PE:void
 * PA:void
 *************************************************************************/
void main_anzeige(void)
{
	/*************************************************************************
	* Messwerte in Strings umwandeln und auf den LCD anzeigen
	*************************************************************************/
    lcd_command(LCD_CURSOROFF | LCD_BLINKINGOFF);	// Kein Cursor in der Hauptanzeige
	zeit_to_string(zeit, anzeigetext1);
    lcd_printlc(1,1,(unsigned char*)anzeigetext1);	// Uhrzeit oben rechts im Display
	lcd_putchar(' ');								// Abstand zum Wochentag
    lcd_print(strcpy_P (anzeigetext,wochentagname[zeit.wochentag])); // Wochentag anzeigen
    temperatur_to_string(temperatur,anzeigetext1);	// Aktuelle Temperatur zu String
    lcd_printlc(2,1,(unsigned char*)"Temp:");		// Temperatur  Linie 2
    lcd_print((unsigned char*) anzeigetext1);		// Temperatur anzeigen
    lcd_printlc(2,11,(unsigned char*)" Max:");		//  Tagesmaximum Temperatur 3 Linie
    temperatur_to_string(extrem_temperatur.max_wert,anzeigetext2);	// Tagesmaximum zu String
    lcd_print((unsigned char*) anzeigetext2);		// Tagesmaximum anzeigen
	//lcd_printlc(3,1,(unsigned char*)"pH:");			// pH Wert Linie 3
	//phwert_to_string(ph_wert, anzeigetext1);		// ph Wert in String
	//lcd_print(anzeigetext1);						// ph Wert anzeigen
    lcd_printlrc(3,11,(unsigned char*)" Min:");		// Tagesminimum Temperatur rechts von pH
    temperatur_to_string(extrem_temperatur.min_wert,anzeigetext2);	// Tagesminimum zu String
    lcd_print((unsigned char*) anzeigetext2);		// Tagesminimum anzeigen	
	/*************************************************************************
	* Zustände der Ausgänge in Linie 4 anzeigen 
	*'^' entspricht eingeschaltet '_' entspricht ausgeschaltet
	*************************************************************************/
    lcd_printlc(4,1,(unsigned char*)"H:");	// Heizer = H
    lcd_putchar(zustand_lcd(( HEIZER_ZUSTAND))); //Zustand abfragen und entsprechendes Zeichen anzeigen
    lcd_print((unsigned char *)" K:");		// Lüfter = Kühler = K, da L schon belegt
    lcd_putchar(zustand_lcd(( LUEFTER_ZUSTAND)));//Zustand abfragen und entsprechendes Zeichen anzeigen
    lcd_print((unsigned char *)" L:");			// Lampe = L
    lcd_putchar(zustand_lcd(( LAMPE_ZUSTAND)));	//Zustand abfragen und entsprechendes Zeichen anzeigen
    lcd_print((unsigned char *)" ML:");			// Mondlicht = ML
    lcd_putchar(zustand_lcd(zustandsbit.mondlicht)); //Zustand abfragen und entsprechendes Zeichen anzeigen
	lcd_print((unsigned char *)" P:");			// Strömungspumpe = P
	lcd_putchar(zustand_lcd(( PUMPE_ZUSTAND)));	//Zustand abfragen und entsprechendes Zeichen anzeigen
	//ultoa(ph_wert, anzeigetext1, 10);
	//lcd_printlc(1,13,anzeigetext1);
	/*************************************************************************
	* Alarmzustand oben rechts im Display anzeigen
	*************************************************************************/
	//Alarmstring aus Array holen und anzeigen
    lcd_printlc(1,13,strcpy_P (anzeigetext,alarm_strings[alarm_vektor])); 
}
