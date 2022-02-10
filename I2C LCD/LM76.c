/************************************************************************
 * Titel :		LM76.c
 * Projekt:		Diplomarbeit Aquariumsteuerung µQuarium
 * Funktion:	Library zur Ansteuerung des LM76 Temperatursensors
 * Autor :		Stefan Keller
 * Lehrgang:	Techniker HF ET 08-11 Klasse A
 * Datum :		18.9.20111
 ************************************************************************/
#include "LM76.h"

/*************************************************************************
 * get_lm76_temperatur(..) - Temperatur Bytes vom LM76 holen und zusammensetzen
 * PE:uint8_t adresse // I2C Adresse LM 76
 * PA:int16_t		  // Temperatur als int Variable, 1 Bit entspricht 0.0625°C
*************************************************************************/
int16_t get_lm76_temperatur(uint8_t adresse) // 16Bit vom I2C Slave einlesen
{
    int8_t oberes_nibble;
    int8_t unteres_nibble;
    int16_t data =0;
    uint8_t ret;

    ret = i2c_start(adresse + I2C_WRITE); // write start auf LM76 schreiben
    if (ret)		// Keine Antwort vom Sensor?
    {
        i2c_stop(); // Bus stoppen
        alarm_vektor = 1; // I2C Alarm auslösen
    }
    else			// Sensor antwortet
    {
        i2c_write(0x00); // Register Pointer auf Temperatur
        i2c_rep_start(adresse + I2C_READ);
        oberes_nibble= i2c_readAck(); // Oberes Temperatur Nibble einlesen
        unteres_nibble = i2c_readNak(); // Unteres Temperatur Nibble einlesen
        i2c_stop();

        data = (unteres_nibble & 0xff) + (oberes_nibble <<8); // Temperatur zusammensetzen
        data >>=3; //untere 3 Status Bits abschneiden
        return data; // Temperatur zurückgeben
    }
    return data;
}

/*************************************************************************
 * get_lm76_temperatur_4x(..) - Temperatur vier mal nacheinander holen, falls 
 * Temperaturen nicht identisch, Alarm auslösen
 * Bei korrekter Funktion des LM76 müssen alle vier Temperaturen gleich sein,
 * da der Sensor nicht genügend Zeit hat um eine neue Temperatur zu berechnen
 * PE:uint8_t adresse	// I2C Adresse LM76
 * PA:int16_t			// Temperatur als int Variable, 1 Bit entspricht 0.0625°C
*************************************************************************/
int16_t get_lm76_temperatur_4x(uint8_t adresse)
{
	int16_t temperatur_array[4]; // Array Temperaturen zwischenspeichern
	uint8_t i;
	static int16_t letzte_ok_temperatur; // Letzte korrekt ausgelesene Temperatur
	uint8_t fehler = 0;					 // Fehlervariable falls temperaturen nicht identisch
	
	for (i=0; i<4; i++)					// Vier mal Temperatur holen
	{
		temperatur_array[i] = get_lm76_temperatur(adresse);
 	}
	
	for(i=0;i<3;i++)					// ausgelesene Temperaturen vergleichen	
	if(temperatur_array[i] != temperatur_array[i+1])
	{
		fehler =1;						// Falls nicht alle identisch Fehler auslösen
	}
	// Falls ein Fehler registriert wurde oder die Wassertemperatur 0°C ist, 0° kann in eine AQ nicht vorkommen
	if(fehler || temperatur_array[0] == 0) 
	{
		alarm_vektor = 1;				// I2C Alarm auslösen
		return letzte_ok_temperatur;	// Letzte korrekte Temperatur returnieren
	}
	 else								// Falls alls Okay
	 {
		letzte_ok_temperatur = temperatur_array[0]; // Temperatur speichern
		return temperatur_array[0];					// Temperatur returnieren
	 }		
}