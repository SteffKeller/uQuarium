/************************************************************************
 * Titel :		LM76.c
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Library zur Ansteuerung des LM76 Temperatursensors
 * Autor :		Stefan Keller
 * Lehrgang:	Techniker HF ET 08-11 Klasse A
 * Datum :		18.9.20111
 ************************************************************************/
#include "hih6131.h"



/*************************************************************************
 * get_hih6131_data(..) -
 * PE:uint8_t adresse
 * PE:uint16_t * huminity
 * PE:int16_t * temperatur
 * PA:int16_t
*************************************************************************/
void get_hih6131_data( uint8_t adresse, uint16_t *huminity_local, int16_t *temperatur_local) // 16Bit vom I2C Slave einlesen
{
    int32_t high_temp, low_temp;
    int32_t high_hum, low_hum;
    uint8_t ret;

    ret = i2c_start(adresse + I2C_WRITE); // write start auf LM76 schreiben
    if (ret)		// Keine Antwort vom Sensor?
    {
        i2c_stop(); // Bus stoppen
        alarm_vektor = 1; // I2C Alarm auslösen
		*huminity_local = 0;
		*temperatur_local = 0;
    }
    else			// Sensor antwortet
    {
		_delay_ms(40);
		i2c_start(adresse + I2C_READ);
        high_hum= i2c_readAck(); // Oberes Temperatur Nibble einlesen
        low_hum = i2c_readAck(); // Unteres Temperatur Nibble einlesen
		high_temp = i2c_readAck(); // Oberes Temperatur Nibble einlesen
		low_temp = i2c_readNak(); // Unteres Temperatur Nibble einlesen
        i2c_stop();

        *huminity_local =((((high_hum & 0x3f) << 8) | (low_hum))*100)/16383;
		*temperatur_local = (((((high_temp << 8) | low_temp)>>2)*165)/16383)-40; 

    }
  
}

