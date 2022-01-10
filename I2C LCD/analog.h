////////////////////////////////////////////////////////////////////////
//
//
//Analog Digital wandler Atmega8 Port C
//
//
//////////////////////////////////////////////////////////////////////////
#include	<avr/io.h>		// AVR Register und Konstantendefinitionen

#define PH_ADC_CHANNEL 4

typedef struct Ph_reverenz
{
	uint16_t adc_ref_ph4;
	uint16_t adc_ref_ph7;
	uint16_t adc_ref_ph9;
	uint32_t steigung_ph47;
	uint32_t nullversatz_ph47;
	uint32_t steigung_ph79;
	uint32_t nullversatz_ph79;
	
}Ph_reverenz;
Ph_reverenz phwert_referenzen;



uint16_t adc_read_channel(uint8_t mux);
void adc_init(void);
void phwert_kalibrieren(uint16_t *ph4, uint16_t *ph7, uint16_t *ph9, Ph_reverenz *ph_werte);
uint16_t read_phwert(Ph_reverenz *ph_ref_werte);
void phwert_to_string( uint16_t ph_wert, char* temp_buffer);