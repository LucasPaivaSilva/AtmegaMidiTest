/*
 * Atmega328_TC_Interrupter.c
 *
 * Created: 5/15/2019 11:11:43 AM
 * Author : Lucas Paiva da Silva 
 */ 

#include "defs.h"

float PW_mult = 1.0;
float PW_mult_limit = 2.0;
int	  Pw_mult_to_display = 1;
unsigned char PW_multStr[4];

int FixedFreq = 220;
int FixedFrqLimit = 500;
unsigned char FixedFreqStr[4];

volatile int ON_TIME, current_pitch = 0;

int debouncePB3 = 0;
int debouncePB4 = 0;
int debouncePB5 = 0;
int PB3Flag = 0;
int PB4Flag = 1;
int PB5Flag = 0;


int StateSelection = 0;


//Menu Variables
unsigned char MenuChar[] = {0x20, 'M', 'I', 'D', 'I', 0x20, 0x20, 0x20, 'F', 'i', 'x', 'e', 'd', 0x20, 0x20, 0x20,
						    0x20, 'T', 'e', 's', 't', 0x20, 0x20, 0x20, 'S' , 'e' , 't', 't', 'i', 'n', 'g', 's'};
unsigned char MenuSelectionBar[] = {0, 7, 16, 23};
int MenuSelectionPosition = 3;

//MIDI Variables
unsigned char MIDIChar[] = {'E', '5', '-', '4', '4', '0', 'H', 'z', 0x20, 0x20, 'P', 'W', ':', '1', '.', '7',
							0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
unsigned char MIDISelectionBar[] = {0, 7, 16, 23};
	
//Fixed Variables
unsigned char FixedChar[] = {0x20, '4', '4', '0', 'H', 'z', 0x20, 0x20, 0x20,  0x20, 'P', 'W', ':', '1', '.', '2',
							0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 , 0x20 , 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
unsigned char FixedSelectionBar[] = {0, 8};

//Test
unsigned char NoneChar[] = {0x20, 'B', '1', ':',  0x20, '2', '0', '0', 'H', 'z', '@', '5', '0', '0', 'm', 'S',
							0x20, 'B', '2', ':',  0x20, '1', '0', '0', 'H', 'z', '@', '5', '0', '0', 'm', 'S',};
unsigned char NoneSelectionBar[] = {0, 7, 16, 23};
	
//Settings
unsigned char SettingsChar[] = {0x20, 'P' , 'W' , '_' , 'l' , 'i' , 'm' , 'i' , 't', ':', 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
								0x20, 'V', 'e', 'r', 's', 'i', 'o', 'n',0x20, '0', '.', '3', '0', 0x20, 0x20, 0x20};
unsigned char SettingsSelectionBar[] = {0, 7, 16, 23};

void InitMessage();
void ChangePWLimit(int operation);
void RefreshDisplay(unsigned char DisplayChar[]);
void ModifyDisplay(unsigned char DisplayChar[], unsigned char DisplaySelectionBar[]);
void ConvertBars(unsigned char DisplayChar[], float PW, float PWMax);
void ChangeFixedFreq(int operation, unsigned char DisplayChar[]);
void ChangePW(int operation, unsigned char DisplayChar[]);
int GetOnTime(int freq);
void HandleNoteOn(byte channel, byte pitch, byte velocity);
ISR(PCINT0_vect);
ISR(TIMER1_COMPA_vect);
ISR(PCINT2_vect);




int main(void)
{
    
	DDRC = 0xFF; 
	DDRB  = 0x01;	
	PORTB = 0b111110;
	PORTD = 0x00;
	
	//interrupção dos bots
	PCICR = (1<<PCIE0) | (1<<PCIE2);
	PCMSK0 = (1<<PCINT3) | (1<<PCINT4) | (1<<PCINT5);
	PCMSK2 = (1<<PCINT16);
	
	//Timer
	TCCR1A = 0x00;                        
	TCCR1B = (1 << CS11) | (1 << WGM12); 
	
// 	OCR1A   = 12300;
// 	TCNT1   = 0;             
// 	TIMSK1 |= (1 << OCIE1A);  
	
		
	sei();
	
	MIDI.begin(MIDI_CHANNEL_OMNI);  
	MIDI.setHandleNoteOn(HandleNoteOn); 
	
	inic_LCD_4bits();
	InitMessage();
	ModifyDisplay(MenuChar, MenuSelectionBar);
	ChangeFixedFreq(2, FixedChar);								//Atualiza o valor de FixedChar com a frequencia(sem alterar a mesma)
	ChangePW(2, MIDIChar);										//Atualiza o valor de MIDIChar p PW(sem alterar o mesmo)
    while (1) 
    {
		if (1)
		{
			switch (StateSelection)
			{
				case 0:
				ModifyDisplay(MenuChar, MenuSelectionBar);
				break;

				case 1:
				ModifyDisplay(MIDIChar, MIDISelectionBar);
				break;
				
				case 2:
				ModifyDisplay(FixedChar, FixedSelectionBar);
				break;
				
				case 3:
				ModifyDisplay(NoneChar, NoneSelectionBar);
				break;
				
				case 4:
				ModifyDisplay(SettingsChar, SettingsSelectionBar);
				break;
			}
			_delay_ms(10);
			debouncePB3 = 0;
			debouncePB4 = 0;
			debouncePB5 = 0;
		}
    }
}

void ConvertBars(unsigned char DisplayChar[], float V, float Vmax)
{
	int Nbars = 0, x = 0;
	for (x=0; x<16; x++)
	{
		DisplayChar[16+x] = 0x20;
	}
	Nbars = (16*V)/Vmax;
	for (x=0; x<Nbars; x++)
	{
		DisplayChar[16+x] = 0xFF;
	}
		
}


void InitMessage()
{
	cmd_LCD(0x80, 0);
	escreve_LCD("Paiva's TC");
	cmd_LCD(0xC0, 0);
	escreve_LCD("328P Interrupter");
	_delay_ms(300);
	cmd_LCD(1, 0);
	cmd_LCD(0x80, 0);
	escreve_LCD("Version 0.1");
	_delay_ms(100);
	cmd_LCD(1, 0);
};

void ChangePW(int operation, unsigned char DisplayChar[])
{
	if ((operation == 1)&&(PW_mult<PW_mult_limit)){PW_mult = PW_mult + 0.1;}
	if ((operation == 0)&&(PW_mult>0.1)){PW_mult = PW_mult - 0.1;}
	if (PW_mult>=PW_mult_limit){PW_mult = PW_mult_limit;}
	Pw_mult_to_display = (PW_mult * 10);
	ident_num(Pw_mult_to_display, PW_multStr);
	DisplayChar[13] = PW_multStr[1];
	DisplayChar[14] = '.';
	DisplayChar[15] = PW_multStr[0];
}

void ChangeFixedFreq(int operation, unsigned char DisplayChar[])
{
	if ((operation == 1)&&(FixedFreq<FixedFrqLimit)){FixedFreq = FixedFreq + 10;}
	if ((operation == 0)&&(FixedFreq>0)){FixedFreq = FixedFreq - 10;}
	ident_num(FixedFreq, FixedFreqStr);
	DisplayChar[1] = FixedFreqStr[2];
	DisplayChar[2] = FixedFreqStr[1];
	DisplayChar[3] = FixedFreqStr[0];
}


void ChangePWLimit(int operation)
{
	if ((operation == 1)&&(PW_mult_limit<4)){PW_mult_limit = PW_mult_limit + 0.1;}
	if ((operation == 0)&&(PW_mult_limit>0.5)){PW_mult_limit = PW_mult_limit - 0.1;}
		
	if (PW_mult>=PW_mult_limit)
	{
		PW_mult = PW_mult_limit;
		ChangePW(2, MIDIChar);
	}
	
	Pw_mult_to_display = (PW_mult_limit * 10);
	ident_num(Pw_mult_to_display, PW_multStr);
	SettingsChar[11] = PW_multStr[1];
	SettingsChar[12] = '.';
	SettingsChar[13] = PW_multStr[0];
}

void RefreshDisplay(unsigned char DisplayChar[])
{
	int x;
	cmd_LCD(1, 0);
	cmd_LCD(0x80, 0);
	for (x=0; x<16;x++)
	{
		cmd_LCD(DisplayChar[x], 1);
	}
	cmd_LCD(0xC0, 0);
	for (x=16; x<32;x++)
	{
		cmd_LCD(DisplayChar[x], 1);
	}
}

void ModifyDisplay(unsigned char DisplayChar[], unsigned char DisplaySelectionBar[])
{
	if ((PB4Flag == 1))
	{
		PB4Flag = 0;
		switch (StateSelection)
		{
			case 0:
			DisplayChar[DisplaySelectionBar[MenuSelectionPosition]] = 0x20;
			MenuSelectionPosition++;
			if (MenuSelectionPosition == 4){MenuSelectionPosition = 0;}
			DisplayChar[DisplaySelectionBar[MenuSelectionPosition]] = '>';
			RefreshDisplay(DisplayChar);
			break;
			
			case 1:
			ChangePW(0, MIDIChar);
			ConvertBars(MIDIChar, PW_mult, PW_mult_limit);
			RefreshDisplay(MIDIChar);
			break;
			
			case 2:
			ChangeFixedFreq(0, FixedChar);
			ConvertBars(FixedChar, FixedFreq, FixedFrqLimit);
			RefreshDisplay(FixedChar);
			break;
			
			case 3:
			
			break;

			case 4:
			ChangePWLimit(0);
			RefreshDisplay(SettingsChar);
			break;
		}
	}
	
	if (PB5Flag == 1)
	{
		PB5Flag = 0;
		switch (StateSelection)
		{
			case 0:
			if (MenuSelectionPosition == 0){StateSelection = 1; ConvertBars(MIDIChar, PW_mult, PW_mult_limit); RefreshDisplay(MIDIChar);}
			if (MenuSelectionPosition == 1){StateSelection = 2; ConvertBars(FixedChar, FixedFreq, FixedFrqLimit); ChangePW(2, FixedChar); RefreshDisplay(FixedChar);}
			if (MenuSelectionPosition == 2){StateSelection = 3; RefreshDisplay(NoneChar);}
			if (MenuSelectionPosition == 3){StateSelection = 4; ChangePWLimit(2); RefreshDisplay(SettingsChar);}
			break;
			
			case 1:
			ChangePW(1, MIDIChar);
			ConvertBars(MIDIChar, PW_mult, PW_mult_limit);
			RefreshDisplay(MIDIChar);
			break;
			
			case 2:
			ChangeFixedFreq(1, FixedChar);
			ConvertBars(FixedChar, FixedFreq, FixedFrqLimit);
			RefreshDisplay(FixedChar);
			break;
			
			case 3:
			
			break;

			case 4:
			ChangePWLimit(1);
			RefreshDisplay(SettingsChar);
			break;
		}
	}
	
	if (PB3Flag == 1)
	{
		PB3Flag = 0;
		switch (StateSelection)
		{
			case 0:
			break;

			case 1:
			StateSelection = 0; RefreshDisplay(MenuChar);
			break;
			
			case 2:
			StateSelection = 0; RefreshDisplay(MenuChar);
			break;
			
			case 3:
			StateSelection = 0; RefreshDisplay(MenuChar);
			break;
			
			case 4:
			StateSelection = 0; RefreshDisplay(MenuChar);
			break;
			
		}
	}
}

int GetOnTime(int freq)
{
	int on_time = 10;
	if (freq < 700)  {on_time = 20;}
	if (freq < 600)  {on_time = 23;}
	if (freq < 500)  {on_time = 27;}
	if (freq < 400)  {on_time = 30;}
	if (freq < 300)  {on_time = 35;}
	if (freq < 200)  {on_time = 40;}
	if (freq < 100)  {on_time = 45;}
	on_time = on_time * PW_mult;
	return on_time;
}

// MIDI callback functions
void HandleNoteOn(byte channel, byte pitch, byte velocity) {
	if (channel == 1) {
		if (velocity == 0) {  // Note off
			if (pitch == current_pitch) {
				TIMSK1 &= ~(1 << OCIE1A);    // Disable Timer1's overflow interrupt to halt the output
			}
		}
		else {  // Note on
			TIMSK1 &= ~(1 << OCIE1A);    // Disable Timer1's overflow interrupt to halt the output
			
			current_pitch = pitch;  // Store the pitch that will be played
			
			int frequency = (int) (220.0 * pow(pow(2.0, 1.0/12.0), pitch - 57) + 0.5);  // Decypher the pitch number
			int period = 1000000 / frequency;                                           // Perform period and calculation
			ON_TIME = GetOnTime(frequency);                                            // Get a value from the pulsewidth lookup table
			
			OCR1A   = 2 * period;     // Set the compare value
			TCNT1   = 0;              // Reset Timer1
			TIMSK1 |= (1 << OCIE1A);  // Enable the compare interrupt (start playing the note)
		}
	}
	else {
		// Ignore the command if it wasn't on channel 1 - this part may depend on the particular MIDI instrument you use, so modify the code here if you encounter a problem!
	}
}

ISR(PCINT0_vect) //interrupção do TC1
{
	if ((!tst_bit(PINB, PB4))&&(debouncePB4==0))
	{
		
		PB4Flag = 1;
		debouncePB4 = 1;
	}
	else if ((!tst_bit(PINB, PB5))&&(debouncePB5==0))
	{
		PB5Flag = 1;
		debouncePB5 = 1;
	}
	else if ((!tst_bit(PINB, PB3))&&(debouncePB3==0))
	{
		PB3Flag = 1;
		debouncePB3 = 1;
	}
}

ISR(PCINT2_vect) //interrupção do TC1
{
	MIDI.read();
}

ISR(TIMER1_COMPA_vect)
{
	int x;
	set_bit(PORTB, PB0);  
	for (x=0;x<ON_TIME;x++)
	{
		_delay_us(1);
	}
	clr_bit(PORTB, PB0); 
	         
}



