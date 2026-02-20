/**
 * WS2812B Signal Splitter for ATTiny406
 *
 * Splits an incoming WS2812B signal into two outputs based on pixel count.
 * - First N pixels go to Output 1 (PA4 / LUT0-OUT)
 * - Remaining pixels go to Output 2 (PA7 / LUT1-OUT)
 */


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "config.hpp"

// =============================================================================
// Global Variables
// =============================================================================

volatile uint16_t targetBits = 0;
volatile bool controlPinShorted = false;


// =============================================================================
// Output Control Functions
// =============================================================================

static inline void enableOutput1() {
  if (controlPinShorted) {
    // Disable invert
    PORTB.PIN0CTRL &= ~PORT_INVEN_bm;
  } else {
    // Set control pin LOW
    PORTB.OUTCLR = PIN0_bm;
  }
}

static inline void enableOutput2() {
  if (controlPinShorted) {
    // Enable invert
    PORTB.PIN0CTRL |= PORT_INVEN_bm;
  } else {
    // Set control pin HIGH
    PORTB.OUTSET = PIN0_bm;
  }
}


// =============================================================================
// Setup Functions
// =============================================================================

/**
 * Configure the main system clock
 */
void setupClock() {
  // No prescaler => 20 MHz
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);
}

/**
 * Disable unused peripherals to save power
 */
void disableUnusedPeripherals() {
  // Disable ADC
  ADC0.CTRLA = 0;

  // Disable AC
  AC0.CTRLA = 0;

  // Disable VREF
  VREF.CTRLA = 0;
  VREF.CTRLB = 0;

  // Disable USART
  USART0.CTRLB = 0;

  // Disable TWI/I2C
  TWI0.MCTRLA = 0;
  TWI0.SCTRLA = 0;

  // Disable SPI
  SPI0.CTRLA = 0;
}

/**
 * Initialize configuration
 *
 * Reads configuration from GPIO pins if READ_CONFIG_FROM_JUMPERS is defined,
 * otherwise uses hardcoded values.
 *
 * Also detects if PB0 is shorted to ground and adjusts control logic accordingly.
 *
 * Delay pixel count (8-bit value):
 *   PA5: Bit 0 (LSB)
 *   PB5: Bit 1
 *   PB3: Bit 2
 *   PB1: Bit 3
 *   PA6: Bit 4
 *   PB4: Bit 5
 *   PB2: Bit 6
 *   PB0: Bit 7 (MSB)
 *
 * RGBW mode:
 *   PA2: RGBW enable (1 = 32 bits/pixel, 0 = 24 bits/pixel)
 *
 * Pins are read with pull-ups enabled:
 *   - Grounded pin = 0
 *   - Floating pin = 1 (pulled high)
 *
 * Pull-ups are disabled after reading.
 */
void init() {

  uint16_t pixel_delay = 0;
  uint8_t bits_per_pixel = 0;

  #ifdef READ_STARTUP_CONFIG

    // Configure pins as inputs with pull-ups enabled
    PORTA.DIRCLR = PIN2_bm | PIN5_bm | PIN6_bm;
    PORTA.PIN2CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN5CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm;

    PORTB.DIRCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm;
    PORTB.PIN0CTRL = PORT_PULLUPEN_bm;
    PORTB.PIN1CTRL = PORT_PULLUPEN_bm;
    PORTB.PIN2CTRL = PORT_PULLUPEN_bm;
    PORTB.PIN3CTRL = PORT_PULLUPEN_bm;
    PORTB.PIN4CTRL = PORT_PULLUPEN_bm;
    PORTB.PIN5CTRL = PORT_PULLUPEN_bm;

    // Small delay to let pull-ups stabilize
    _delay_us(100);

    // Read RGBW mode from PA2
    bool rgbw_mode = (PORTA.IN & PIN2_bm) ? true : false;

    if (rgbw_mode) {
      bits_per_pixel = BITS_PER_PIXEL_RGBW;
    } else {
      bits_per_pixel = BITS_PER_PIXEL_RGB;
    }

    // Read pin states and build the delay pixel value
    // With pull-ups: grounded = 0, floating = 1
    if (PORTA.IN & PIN5_bm) pixel_delay |= (1 << 0);  // Bit 0
    if (PORTB.IN & PIN5_bm) pixel_delay |= (1 << 1);  // Bit 1
    if (PORTB.IN & PIN3_bm) pixel_delay |= (1 << 2);  // Bit 2
    if (PORTB.IN & PIN1_bm) pixel_delay |= (1 << 3);  // Bit 3
    if (PORTA.IN & PIN6_bm) pixel_delay |= (1 << 4);  // Bit 4
    if (PORTB.IN & PIN4_bm) pixel_delay |= (1 << 5);  // Bit 5
    if (PORTB.IN & PIN2_bm) pixel_delay |= (1 << 6);  // Bit 6
    if (PORTB.IN & PIN0_bm) pixel_delay |= (1 << 7);  // Bit 7

    #ifndef FORCE_SHORTED_CONTROL_PIN
      // Check if PB0 is shorted to ground
      if (pixel_delay & (1 << 7)) {
        controlPinShorted = true;
      }
    #endif

    // Disable pull-ups after reading
    PORTA.PIN2CTRL = 0;
    PORTA.PIN5CTRL = 0;
    PORTA.PIN6CTRL = 0;
    PORTB.PIN0CTRL = 0;
    PORTB.PIN1CTRL = 0;
    PORTB.PIN2CTRL = 0;
    PORTB.PIN3CTRL = 0;
    PORTB.PIN4CTRL = 0;
    PORTB.PIN5CTRL = 0;

  #else
    // Use hardcoded configuration values

    #ifndef FORCE_SHORTED_CONTROL_PIN
      // Check if PB0 is shorted to ground by configuring it as input with pull-up and reading its state
      PORTB.DIRCLR = PIN0_bm;
      PORTB.PIN0CTRL = PORT_PULLUPEN_bm;

      _delay_us(100);

      if (!(PORTB.IN & PIN0_bm)) {
        // Pin is shorted to ground => use pin-invert as control signal
        controlPinShorted = true;
      }

      PORTB.PIN0CTRL = 0;
    #endif

    bits_per_pixel = BITS_PER_PIXEL;
    pixel_delay = PIXEL_DELAY;
  #endif

  targetBits = pixel_delay * bits_per_pixel;

  #ifdef FORCE_SHORTED_CONTROL_PIN
    controlPinShorted = true;
  #endif
}

/**
 * Configure GPIO pins
 */
void setupPins() {
  // PA1: Input (WS2812B data input)
  PORTA.DIRCLR = PIN1_bm;

  // PA4 and PA7 direction is controlled by CCL when OUTEN is enabled
  // But we set them as outputs for clarity
  PORTA.DIRSET = PIN4_bm | PIN7_bm;

  // PB0: Control pin for output switching
  if (controlPinShorted) {
    // Use PB0 as input. Toggle invert flag to trigger events.
    PORTB.DIRCLR = PIN0_bm;
  } else {
    // Use PB0 as output. Toggle output to trigger events.
    PORTB.DIRSET = PIN0_bm;
    PORTB.OUTSET = PIN0_bm;
  }
}

/**
 * Configure the Event System
 *
 * Routes PA1 signal to:
 * - Both LUTs via Async Event Channel 0 (for signal buffering)
 * - TCA0 via Sync Event Channel 0 (for hardware bit counting)
 *
 * Routes PB0 signal to:
 * - Both LUTs via Async Event Channel 1 (for output control)
 */
void setupEventSystem() {
  // Async Channel 0: PA1 (data signal) for both LUTs
  EVSYS.ASYNCCH0 = EVSYS_ASYNCCH0_PORTA_PIN1_gc;

  // Async Channel 1: PB0 (control signal) for both LUTs
  EVSYS.ASYNCCH1 = EVSYS_ASYNCCH1_PORTB_PIN0_gc;

  // Route Async Channel 0 to both LUT0 and LUT1 Event 0 inputs
  // ASYNCUSER2 = LUT0 Event 0 input
  // ASYNCUSER3 = LUT1 Event 0 input
  EVSYS.ASYNCUSER2 = EVSYS_ASYNCUSER2_ASYNCCH0_gc;  // LUT0 E0 <- CH0 (data)
  EVSYS.ASYNCUSER3 = EVSYS_ASYNCUSER3_ASYNCCH0_gc;  // LUT1 E0 <- CH0 (data)

  // Route Async Channel 1 to both LUT0 and LUT1 Event 1 inputs
  // ASYNCUSER4 = LUT0 Event 1 input
  // ASYNCUSER5 = LUT1 Event 1 input
  EVSYS.ASYNCUSER4 = EVSYS_ASYNCUSER4_ASYNCCH1_gc;  // LUT0 E1 <- CH1 (control)
  EVSYS.ASYNCUSER5 = EVSYS_ASYNCUSER5_ASYNCCH1_gc;  // LUT1 E1 <- CH1 (control)

  // Sync Channel 0: PA1 (data signal) for TCA0
  EVSYS.SYNCCH0 = EVSYS_SYNCCH0_PORTA_PIN1_gc;

  // Route Sync Channel 0 to TCA0 for event counting
  // SYNCUSER0 = TCA0 Event input
  EVSYS.SYNCUSER0 = EVSYS_SYNCUSER0_SYNCCH0_gc;  // TCA0 <- SYNCCH0 (count rising edges)
}

/**
 * Configure the CCL (Configurable Custom Logic)
 *
 * Two LUTs are configured to control the outputs based on the data and control signals:
 * - LUT0 controls Output 1 (PA4)
 * - LUT1 controls Output 2 (PA7)
 *
 * Both LUTs use the same inputs:
 * - Input 0: Event 0 (data signal from PA1)
 * - Input 1: Event 1 (control signal from PB0)
 * - Input 2: Masked (forced to 0)
 */
void setupCCL() {
  // Disable CCL while configuring
  CCL.CTRLA = 0;

  // LUT0 Configuration (Output 1 - PA4)
  CCL.LUT0CTRLB = CCL_INSEL0_EVENT0_gc | CCL_INSEL1_EVENT1_gc;
  CCL.LUT0CTRLC = CCL_INSEL2_MASK_gc;

  // TRUTH0 table configuration
  CCL.TRUTH0 = TRUTH0_DATA;

  // Enable LUT0 with output to pin PA4
  CCL.LUT0CTRLA = CCL_OUTEN_bm | CCL_ENABLE_bm;

  // LUT1 Configuration (Output 2 - PA7)
  CCL.LUT1CTRLB = CCL_INSEL0_EVENT0_gc | CCL_INSEL1_EVENT1_gc;
  CCL.LUT1CTRLC = CCL_INSEL2_MASK_gc;

  // TRUTH1 table configuration
  CCL.TRUTH1 = TRUTH1_DATA;

  // Enable LUT1 with output to pin PA7
  CCL.LUT1CTRLA = CCL_OUTEN_bm | CCL_ENABLE_bm;

  // Enable the CCL peripheral
  CCL.CTRLA = CCL_ENABLE_bm;
}

/**
 * Configure TCA0 as event counter for bit counting
 *
 * Uses event input as clock source.
 * Generates overflow interrupt when count reaches targetBits, which triggers output switching.
 */
void setupEventCounter() {
  // Disable TCA0 while configuring
  TCA0.SINGLE.CTRLA = 0;

  // Configure for counting on event edges
  TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;

  // Event control: count on positive edge of event, enable event counting
  TCA0.SINGLE.EVCTRL = TCA_SINGLE_EVACT_POSEDGE_gc | TCA_SINGLE_CNTEI_bm;

  // Set period to target bit count (interrupt fires when CNT reaches PER)
  TCA0.SINGLE.PER = targetBits - 2;

  // Enable overflow interrupt (fires when CNT reaches PER)
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;

  // Enable TCA0 - event system provides the clock
  TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;
}

/**
 * Setup pin interrupts
 */
void setupPinInterrupts() {
  // Configure PA1 for rising edge interrupt,
  // used to restart timeout timer on data activity.
  PORTA.PIN1CTRL = PORT_ISC_RISING_gc;
}

/**
 * Configure TCB0 as timeout timer for reset detection
 */
void setupTimeoutTimer() {
  uint16_t timeoutCycles = CPU_CYCLES_FROM_MICROSECONDS(LED_RESET_TIMEOUT);

  // Disable while configuring
  TCB0.CTRLA = 0;

  // Periodic interrupt mode
  TCB0.CTRLB = TCB_CNTMODE_INT_gc;

  // Set compare value (generates interrupt when CNT reaches CCMP)
  TCB0.CCMP = timeoutCycles;

  // Enable capture/compare interrupt
  TCB0.INTCTRL = TCB_CAPT_bm;

  // Enable timer with CLK_PER, no prescaler
  TCB0.CTRLA = TCB_ENABLE_bm;
}


// =============================================================================
// Main Program
// =============================================================================

int main() {
  // Disable unused peripherals first for power saving
  disableUnusedPeripherals();

  // Initialize all peripherals
  setupClock();
  init();
  setupPins();
  setupEventSystem();
  setupCCL();

  if (targetBits < 2) {
    // Technical limitation: Counting less than 2 bits is not possible.
    // Therefor immediately switch to output 2 if targetBits is less than 2.
    enableOutput2();
  } else {
    setupEventCounter();
    setupPinInterrupts();
    setupTimeoutTimer();

    // Enable global interrupts
    sei();
  }

  while (1) {
    // Do nothing
  }
}


// =============================================================================
// Interrupt Service Routines
// =============================================================================

/**
 * Port A Pin Change Interrupt
 *
 * Triggered on rising edge of PA1 (WS2812B data input)
 * Resets the timeout timer to prevent false reset detection during active data
 */
ISR(PORTA_PORT_vect, ISR_NAKED) {
  // Clear the interrupt flag
  PORTA.INTFLAGS = PIN1_bm;

  // Reset timeout timer
  TCB0.CNT = 0;

  // Return from interrupt
  reti();
}

/**
 * TCA0 Overflow Interrupt
 *
 * Triggered when TCA0 event counter reaches targetBits
 * to switch the outputs after the configured pixel delay.
 */
ISR(TCA0_OVF_vect, ISR_NAKED) {
  // Clear the interrupt flag
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;

  // Switch to Output 2
  enableOutput2();

  // Enable timeout timer to switch back to output 1
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;

  // Return from interrupt
  reti();
}

/**
 * TCB0 Capture/Compare Interrupt
 *
 * Triggered when timeout has elapsed without data activity (no rising edges).
 * This indicates the WS2812B reset condition (line held LOW).
 */
ISR(TCB0_INT_vect, ISR_NAKED) {
  // Clear the interrupt flag
  TCB0.INTFLAGS = TCB_CAPT_bm;

  // Stop timeout timer
  TCB0.CTRLA = 0;

  // Verify the line is actually LOW (not just a long HIGH pulse)
  if (!(PORTA.IN & PIN1_bm)) {
    // Reset condition detected

    // Reset TCA0 event counter for new frame
    TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;
    TCA0.SINGLE.CNT = 0;
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;

    // Enable output 1
    enableOutput1();
  }

  // Return from interrupt
  reti();
}
