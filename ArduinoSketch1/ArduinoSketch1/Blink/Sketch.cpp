/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */

/*
* Monophonic MIDI-Based DRSSTC Interrupter
* Daniel Kramnik, June 29, 2012
* Hardware:
*  - Optical transmitter connected to Port D.2 (Arduino Digital Pin 2) through 100 ohm resistor
*  - MIDI input connected to RXD (Arduino RX Pin) through 4N25 optocoupler circuit to isolate micro and prevent ground loops
*/

// MIDI class import - Note: this is not a standard Arduino library, it needs to be downloaded and added to your libraries folder
#include <MIDI.h>
//Beginning of Auto generated function prototypes by Atmel Studio
void setupTimers();
void setupInterrupts();
void HandleNoteOn(byte channel, byte pitch, byte velocity);
int findOnTime(int frequency_input);
ISR(TIMER1_COMPA_vect );
//End of Auto generated function prototypes by Atmel Studio


MIDI_CREATE_DEFAULT_INSTANCE();

// Function prototypes
void setupTimers     (void);
void setupInterrupts (void);
void HandleNoteOn    (byte, byte, byte);
int  findOnTime      (int);

// Global constants
#define PW_TABLE_MULTIPLIER  1.5

// Global primitives
volatile int on_time, current_pitch = 0;

void setup() {
  // Setup optical transmitter on Port D.2 (Digital Pin 2)
  DDRD  |=  (1 << 2);
  PORTD &= ~(1 << 2);    // Start the pin LOW (Note: you should *never* use digitalWrite to set this pin, as that function takes several microseconds
  
  setupTimers();
  setupInterrupts();
  
  // Initiate MIDI communications, listen to all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);    
  
  // Setup MIDI callback functions (add more callback functions here to handle other MIDI events)
  MIDI.setHandleNoteOn(HandleNoteOn);    // Put only the name of the function
}

// Functions for setting up timer and interrupt reigsters
void setupTimers() {
  /*
    Timer1 is used to time the period of a note, with 0.5uS per tick
    When Timer1 reaches the compare value, an interrupt is triggered that generates the output pulse
  */
  
  // Set up 16-bit Timer1 (0.5uS per tick, hardware pins disconnected - interrupt starts disabled)
  TCCR1A = 0x00;                        // CTC mode, hardware pins disconnected
  TCCR1B = (1 << CS11) | (1 << WGM12);  // CTC mode, set prescaler to divide by 8 for 1 / 2MHz or 0.5uS per tick (clkio = 16MHz)
}

void setupInterrupts() {
  sei();    // Enable global interrupts to be safe, nothing else to do here
}

// Main program loop polls for MIDI events
void loop() {
  MIDI.read();
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
        on_time = findOnTime(frequency);                                            // Get a value from the pulsewidth lookup table
        
        OCR1A   = 2 * period;     // Set the compare value
        TCNT1   = 0;              // Reset Timer1
        TIMSK1 |= (1 << OCIE1A);  // Enable the compare interrupt (start playing the note)
    }
  }
  else {
    // Ignore the command if it wasn't on channel 1 - this part may depend on the particular MIDI instrument you use, so modify the code here if you encounter a problem!
  }
}

// Pulsewidth lookup table
int findOnTime(int frequency_input) {
  int on_time = 17;
  if (frequency_input < 1000) {on_time = 17;}
  if (frequency_input < 900)  {on_time = 18;}  
  if (frequency_input < 800)  {on_time = 20;}
  if (frequency_input < 700)  {on_time = 20;}
  if (frequency_input < 600)  {on_time = 23;}
  if (frequency_input < 500)  {on_time = 27;}
  if (frequency_input < 400)  {on_time = 30;}  
  if (frequency_input < 300)  {on_time = 35;}
  if (frequency_input < 200)  {on_time = 40;}
  if (frequency_input < 100)  {on_time = 45;}       
  on_time *= PW_TABLE_MULTIPLIER;
  return on_time;
}

// Interrupt vectors
// Timer1 Compare Interrupt - signals when it's time to start a bang
ISR (TIMER1_COMPA_vect) {
  PORTD |= (1 << 2);           // Set the optical transmit pin high
  delayMicroseconds(on_time);  // Wait
  PORTD &= ~(1 << 2);          // Set the optical transmit pin low
}
