/************************************************************************
 * Titel :		DS1307.c
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Library zur Ansteuerung des DS1307 RTC ICs
 * Autor :		Stefan Keller
 * Lehrgang:	Techniker HF ET 08-11 Klasse A
 * Datum :		4.9.20111
 ************************************************************************/
#include "DS1307.h"

/*************************************************************************
 * ds_1307_init(..) - Initialisiert den DS1307 und aktiviert den Oszillator
 * PE:void
 * PA:void
*************************************************************************/
void ds_1307_init(void)
{
    uint8_t ret,			// Returnwert für Fehlererkennung
			tmp;			// temporäre Variable
    ret = i2c_start(DS1307_ADRESSE + I2C_WRITE); // write start auf DS1307 schreiben
    if (ret)				// Wenn I2C Fehler aufgetreten
    {
        i2c_stop();			// Bus stoppen
        alarm_vektor = 1;	// Alarm I2C Bus Fehler
    }
    else
    {
        i2c_write(0x00);	// Register Pointer auf Sekunden
        i2c_rep_start(DS1307_ADRESSE + I2C_READ);	// Restart mit write senden
        tmp = i2c_readNak();						// Sekunden Register auslesen
        i2c_stop();									// Bus stoppen
        tmp &= ~0x80;								// Bit 7 (CH-BIT) löschen -> aktiviert den RTC Oszillator
        ret = i2c_start(DS1307_ADRESSE + I2C_WRITE);// Restart senden
        i2c_write(0x00);							// Pointer Register auf Sekunden
        i2c_write(tmp);								// Sekunden mit gelöschtem CH-Bit zurückschreiben
        i2c_stop();									// Bus stoppen
    }
}
/************************************************************************
 * get_ds1307_zeit_datum(..) - Aktuelle Zeit und Datum vom DS1307 einlesen
 * PE:Pointer auf struct vom Typ uhr
 * PA:--
************************************************************************/
void get_ds1307_zeit_datum(uhr *zeit_pointer)   // DS1307 Datum setzen  Pointer auf Zeit struct wird übergeben
{
    uint8_t ret;
    uhr get_zeit;			// Hilfsstruct um BCD Code vom DS1307 zu holen und dann die Zeit und Dezimale umzurechnen
    ret = i2c_start(DS1307_ADRESSE + I2C_WRITE); // write start auf DS1307 schreiben
    if (ret)				// Wenn I2C Fehler aufgetreten
    {
        i2c_stop();			// Bus stoppen
        alarm_vektor = 1;	// Alarm I2C Bus Fehler
    }
    else
    {
        i2c_write(0x00);							// Register Pointer auf Sekunde
        i2c_rep_start(DS1307_ADRESSE + I2C_READ);	// Restart mit write senden
        get_zeit.sekunde = i2c_readAck();			// Sekunden einlesen
        get_zeit.minute = i2c_readAck();			// Minuten einlesen
        get_zeit.stunde = i2c_readAck();			// Stunden einlesen
        get_zeit.wochentag = i2c_readAck();			// Wochentag einlesen
        get_zeit.tag = i2c_readAck();				// Tag einlesen
        get_zeit.monat = i2c_readAck();				// Monat einlesen
        get_zeit.jahr = i2c_readNak();				// Jahr einlesen

        zeit_pointer->sekunde = (get_zeit.sekunde & 0x0F) + ((get_zeit.sekunde >> 4) * 10); // BCD-Code ins Dezimale umrechnen
        zeit_pointer->minute = (get_zeit.minute & 0x0F) + ((get_zeit.minute >> 4) * 10);
        zeit_pointer->stunde = (get_zeit.stunde & 0x0F) + ((get_zeit.stunde >> 4)* 10);
        zeit_pointer->tag = (get_zeit.tag & 0x0F) + ((get_zeit.tag >> 4) * 10);
        zeit_pointer->wochentag = get_zeit.wochentag;										// Wochentag kann direkt übernommen werden
        zeit_pointer->monat = (get_zeit.monat & 0x0F) + ((get_zeit.monat >> 4) * 10);
        zeit_pointer->jahr = (get_zeit.jahr & 0x0F) + ((get_zeit.jahr >> 4) * 10);
    }
}
/************************************************************************
 * set_ds1307_zeit(..) - Uhrzeit auf DS1307 schreiben
 * PE:Pointer auf struct uhr
 * PA:--
************************************************************************/
void set_ds1307_zeit(uhr *zeit_pointer)   
{
    uint8_t  ret;
    ret = i2c_start(DS1307_ADRESSE + I2C_WRITE); // write start auf DS1307 schreiben
    if (ret)				// Wenn I2C Fehler aufgetreten	
    {
        i2c_stop();			// Bus stoppen
        alarm_vektor = 1;	// Alarm I2C Bus Fehler
    }
    else
    {
        i2c_write(0x00);															// Register Pointer auf Sekunden
        i2c_write(0x00 |(zeit_pointer->sekunde % 10) | ((zeit_pointer->sekunde / 10) << 4)); // Timer einschalten Bit7 ->0 Sekunden in BCD umrechnen und auf DS1307 senden
        i2c_write((zeit_pointer->minute % 10) | ((zeit_pointer->minute / 10) << 4));// Minuten in BCD umrechnen und an DS 1307 senden
        i2c_write((zeit_pointer->stunde % 10) | ((zeit_pointer->stunde / 10) << 4));// Stunden in BCD umrechnen und an DS 1307 senden
        i2c_stop();																	// Bus stoppen
    }
}
/************************************************************************
 * set_ds1307_datum(..) - Datum auf den DS1307 schreiben
 * PE:Pointer auf struct uhr
 * PA:--
************************************************************************/
void set_ds1307_datum(uhr *datum_pointer) // Datum auf DS1307 schreiben
{
    uint8_t ret;
    ret = i2c_start(DS1307_ADRESSE + I2C_WRITE); // write start auf DS1307 schreiben
    if (ret)	// Wenn I2C Fehler aufgetreten
    {
        i2c_stop(); // Bus stoppen
        alarm_vektor = 1; // Alarm I2C Bus Fehler
    }
    else
    {
        i2c_write(0x03); //Pointer auf Wochentag Register
        i2c_write(datum_pointer->wochentag);									// Wochentag schreiben
        i2c_write((datum_pointer->tag % 10) | (datum_pointer->tag / 10) << 4);	// BCD-Code in BCD für DS1307 umrechnen und schreiben
        i2c_write((datum_pointer->monat % 10) | ((datum_pointer->monat / 10) << 4));
        i2c_write((datum_pointer->jahr % 10) | ((datum_pointer->jahr / 10) << 4));
        i2c_stop();																// Buss stoppen

    }

}
/***************************************************************************************************************************************
Beispiel: Auf DS1307 schreiben und wieder lesen
	int main() {
		i2c_init();
		lcd_home();
		ds_1307_init();
		set_zeit.sekunde = 0;
		set_zeit.minute = 26;
		set_zeit.stunde = 12;
		set_zeit.tag = 5;
		set_zeit.wochentag =7;
		set_zeit.monat = 6;
		set_zeit.jahr = 11;
		set_ds1307_zeit(&set_zeit.stunde, &set_zeit.minute, &set_zeit.sekunde);
		set_ds1307_datum(&set_zeit.tag,&set_zeit.wochentag,&set_zeit.monat,&set_zeit.jahr);
		while (1) {
			wdt_reset();
			get_ds1307_zeit_datum(&zeit);
			sprintf(buffer, "%02d:%02d:%02d", zeit.stunde, zeit.minute, zeit.sekunde);
			sprintf(buffer2, "%02d.%02d.%02d", zeit.tag, zeit.monat, zeit.jahr);
			lcd_goto(1, 0);
			lcd_writeText(buffer);
			lcd_writeText(wochentagname[zeit.wochentag]);
			lcd_writeText(" ");
			lcd_goto(2, 0);
			lcd_writeText(buffer2);
			if(error_flag) error();
			}
	}
*********************************************************************************************************************/