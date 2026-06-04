#ifndef TU_GPIO_H_
#define TU_GPIO_H_

#include "TU_options.h"

#define CV1 A3
#define CV2 A6
#define CV3 A5
#define CV4 A4

#define TR1 0
#define TR2 1 // reset

// outputs, tact buttons:
#ifdef _TEMPS_UTILE_REV_0
  #define CLK1 29
  #define CLK2 28
  #define CLK3 10
  #define CLK4 A14
  #define CLK5 2
  #define CLK6 9
  
  #define but_top 5
  #define but_bot 4 
#else
  #define CLK1 5
  #define CLK2 6
  #define CLK3 7
#if defined(__IMXRT1062__)
  // T4.0 has no A14/DAC pad: ch4 is a plain GPIO gate (see t4-port-plan.md
  // "Channel 4 output pin"). Hardware assembled: the ch4 pogo pad rings out to
  // physical pin 26 (not 24 as first planned) — confirmed by continuity check.
  #define CLK4 26
#else
  #define CLK4 A14 // DAC channel; alt = 29
  // Jakplugg module revision: ch4 gate is mirrored as a plain digital gate on
  // pin 29, in parallel with the A14 DAC. An on-board switch selects which
  // physical output reaches the jack. Driven by set_Output4() (TU_outputs.cpp).
  #define CLK4_GATE 29
#endif
  #define CLK5 8
  #define CLK6 2
  
  #define but_top 3
  #define but_bot 12
#endif

// OLED com: 
#ifdef _TEMPS_UTILE_REV_0
  #define OLED_DC 6
  #define OLED_RST 7
  #define OLED_CS 8
#else
  #define OLED_DC 9
  #define OLED_RST 4
  #define OLED_CS 10
#endif

// OLED CS is active low
#define OLED_CS_ACTIVE LOW
#define OLED_CS_INACTIVE HIGH

#if defined(__IMXRT1062__)
// T4.0: the OLED is bit-banged (the panel's SCK pad is wired to physical pin 14,
// which Teensy 4.0 LPSPI4 cannot drive — SCK is only available on pin 13 or 27).
// MOSI/SCK match the K20 routing: DIN=pin 11, SCK=pin 14.
#define OLED_MOSI 11
#define OLED_SCK  14
#endif

#define encR1 15
#define encR2 16
#define butR  13

#define encL1 22
#define encL2 21
#define butL  23

#define TU_GPIO_DEBUG_PIN1 30
// pin 29 was the spare debug pin (DEBUG_PIN2); it is now CLK4_GATE on the
// Jakplugg revision (see CLK4_GATE above), so only one debug pin remains.

#define TU_GPIO_BUTTON_PINMODE INPUT_PULLUP
#define TU_GPIO_TRx_PINMODE INPUT_PULLUP
#define TU_GPIO_ENC_PINMODE INPUT_PULLUP
#define TU_GPIO_OUTPUTS_PINMODE OUTPUT

#endif // TU_GPIO_H_
