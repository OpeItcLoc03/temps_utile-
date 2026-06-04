
#include <Arduino.h>  // pinMode/digitalWriteFast/OUTPUT (was pulled in via spififo.h on K20)
#if defined(__MK20DX256__)
#include <spififo.h>  // Teensy 3.x only
#endif
#include "TU_outputs.h"
#include "TU_gpio.h"

namespace TU {

/*static*/
void OUTPUTS::Init() {

  pinMode(CLK1, TU_GPIO_OUTPUTS_PINMODE);
  pinMode(CLK2, TU_GPIO_OUTPUTS_PINMODE);
  pinMode(CLK3, TU_GPIO_OUTPUTS_PINMODE);
#if defined(__MK20DX256__)
  // CLK4 == DAC (A14) on K20 — driven via set_Output4(), no pinMode
#ifdef CLK4_GATE
  // Jakplugg rev: ch4 gate is also mirrored as a digital gate on CLK4_GATE.
  pinMode(CLK4_GATE, TU_GPIO_OUTPUTS_PINMODE);
  digitalWriteFast(CLK4_GATE, LOW);
#endif
#else
  // T4.0: ch4 is a plain GPIO gate (no DAC peripheral)
  pinMode(CLK4, TU_GPIO_OUTPUTS_PINMODE);
#endif
  pinMode(CLK5, TU_GPIO_OUTPUTS_PINMODE);
  pinMode(CLK6, TU_GPIO_OUTPUTS_PINMODE);

  history_tail_ = 0;
  memset(history_, 0, sizeof(uint16_t) * kHistoryDepth * NUM_CHANNELS);
}

void OUTPUTS::SPI_Init() {
#if defined(__IMXRT1062__)
  // T4.0: the OLED is bit-banged (SH1106_128x64_Driver::SPI_send), because the
  // panel's SCK pad is wired to physical pin 14 and Teensy 4.0 LPSPI4 can only
  // drive SCK on pin 13 or 27. Just set the two clocking pins as outputs with
  // SCK idle low (SPI_MODE0); CS/DC/RST are handled by the display driver Init.
  pinMode(OLED_MOSI, OUTPUT);
  pinMode(OLED_SCK, OUTPUT);
  digitalWriteFast(OLED_SCK, LOW);
  return;
#else
  uint32_t ctar0, ctar1;

  SIM_SCGC6 |= SIM_SCGC6_SPI0;
  CORE_PIN11_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2);
  CORE_PIN14_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2);
  
  ctar0 = SPI_CTAR_DBR; // default
  #if   F_BUS == 60000000
      ctar0 = (SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) | SPI_CTAR_DBR); //(60 / 2) * ((1+1)/2) = 30 MHz
  #elif F_BUS == 48000000
      ctar0 = (SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) | SPI_CTAR_DBR); //(48 / 2) * ((1+1)/2) = 24 MHz          
  #endif
  ctar1 = ctar0;
  ctar0 |= SPI_CTAR_FMSZ(7);
  ctar1 |= SPI_CTAR_FMSZ(15);
  SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_PCSIS(0x1F);
  SPI0_MCR |= SPI_MCR_CLR_RXF | SPI_MCR_CLR_TXF;

  // update ctars
  uint32_t mcr = SPI0_MCR;
  if (mcr & SPI_MCR_MDIS) {
    SPI0_CTAR0 = ctar0;
    SPI0_CTAR1 = ctar1;
  } else {
    SPI0_MCR = mcr | SPI_MCR_MDIS | SPI_MCR_HALT;
    SPI0_CTAR0 = ctar0;
    SPI0_CTAR1 = ctar1;
    SPI0_MCR = mcr;
  }
#endif // __IMXRT1062__
}


/*static*/
uint32_t OUTPUTS::values_[CLOCK_CHANNEL_LAST];
/*static*/
uint32_t OUTPUTS::states_[CLOCK_CHANNEL_LAST];
/*static*/
uint16_t OUTPUTS::history_[NUM_CHANNELS][OUTPUTS::kHistoryDepth];
/*static*/
volatile size_t OUTPUTS::history_tail_;

}; // namespace TU

const uint16_t _MAX_VALUE = 4095;

#if defined(__MK20DX256__)
// ch4's DAC (A14) feeds a bipolar CV op-amp stage (U12), so DAC code 0 maps to
// the op-amp's most-negative rail, not 0 V. Stock firmware idled the gate at the
// calibrated 0 V code so it swung 0 V -> +V (unipolar); the T4.0 port dropped
// that calibration (ch4 is a plain GPIO gate on T4). For the K20 build, idle the
// gate at the nominal 0 V DAC code instead of 0 to keep the A14 output unipolar.
// Value = index 2 ("0 V") of the stock default calibration table for this model
// variant (TU_calibration.ino kCalibrationDefaults); nominal, not per-board.
#ifdef MODEL_2TT
static const uint16_t CH4_DAC_ZERO_CODE = 1809;
#elif defined(MOD_OFFSET)
static const uint16_t CH4_DAC_ZERO_CODE = 1725;
#else
static const uint16_t CH4_DAC_ZERO_CODE = 2236;
#endif
#endif

void set_Output1(uint8_t data) {
  digitalWriteFast(CLK1, data);
}

void set_Output2(uint8_t data) {
  digitalWriteFast(CLK2, data);
}

void set_Output3(uint8_t data) {
  digitalWriteFast(CLK3, data);
}

void set_Output4(uint16_t data) {

  uint16_t _data = data > _MAX_VALUE ? _MAX_VALUE : data;
  #if defined(__MK20DX256__)
    SIM_SCGC2 |= SIM_SCGC2_DAC0;
    DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS; // 3.3V VDDA = DACREF_2
    // ch4 gate: drive the DAC between the 0 V code (LOW) and fullscale (HIGH) so
    // the bipolar CV op-amp swings 0 V -> +V instead of -V -> +V. values_[ch4]
    // stays a clean binary (0 / 4095), so the pin-29 and T4 gates are unaffected.
    *(int16_t *)&(DAC0_DAT0L) = _data ? _MAX_VALUE : CH4_DAC_ZERO_CODE;
    #ifdef CLK4_GATE
      // Jakplugg rev: mirror the gate as a plain digital output, in parallel
      // with the DAC. ch4 is binary (OFF=0 / ON=4095), so any non-zero = HIGH —
      // same threshold the T4 gate path uses. A board switch picks A14 or this.
      digitalWriteFast(CLK4_GATE, _data ? HIGH : LOW);
    #endif
  #elif defined(__IMXRT1062__)
    // T4.0: no DAC — ch4 is a gate. Treat any non-zero DAC value as HIGH.
    // (CLOCKMODE::DAC itself is pruned in step 4; this keeps ch4 sane meanwhile.)
    digitalWriteFast(CLK4, _data ? HIGH : LOW);
  #endif
}

void set_Output5(uint8_t data) {
  digitalWriteFast(CLK5, data);
}

void set_Output6(uint8_t data) {
  digitalWriteFast(CLK6, data);
}

