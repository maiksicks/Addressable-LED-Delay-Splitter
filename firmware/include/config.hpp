#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

// =============================================================================
// Configuration
// =============================================================================


// Read configuration from solder jumpers on startup instead of using hardcoded values.
#define READ_CONFIG_FROM_JUMPERS

// Uncomment to set the number of pixels before switching outputs.
// Ignored if READ_CONFIG_FROM_JUMPERS is defined.
// #define PIXEL_DELAY 10

// Uncomment to enable RGBW mode (32 bits per pixel).
// Ignored if READ_CONFIG_FROM_JUMPERS is defined.
// #define RGBW_MODE

// Uncomment to set the number of bits per pixel.
// Ignored if READ_CONFIG_FROM_JUMPERS is defined.
// #define BITS_PER_PIXEL 32

// Uncomment to select output 2 as primary output.
// #define SWAP_OUTPUTS

// Uncomment to passthrough all data from input to primary output.
// The secondary output only receives the pixels after PIXEL_DELAY.
// #define PRIMARY_OUTPUT_PASSTHROUGH

// Uncomment to disable output 1
// #define DISABLE_OUTPUT_1

// Uncomment to disable output 2
// #define DISABLE_OUTPUT_2

// Uncomment to set the duration of the reset timeout (in microseconds)
// #define LED_RESET_TIMEOUT 50UL

// Uncomment to force shorted control pin mode (for testing without jumpers)
// #define FORCE_SHORTED_CONTROL_PIN


// =============================================================================
// DO NOT EDIT BELOW THIS LINE UNLESS YOU KNOW WHAT YOU ARE DOING
// =============================================================================

#define BITS_PER_PIXEL_RGB 24
#define BITS_PER_PIXEL_RGBW 32

#ifndef READ_CONFIG_FROM_JUMPERS
  #ifndef PIXEL_DELAY
    #warning PIXEL_DELAY is not set. Using fallback value of 0 ...
    #define PIXEL_DELAY 0
  #endif

  #ifdef BITS_PER_PIXEL
    #ifdef RGBW_MODE
      #warning BITS_PER_PIXEL and RGBW_MODE are set both. Ignoring RGBW_MODE ...
      #undef RGBW_MODE
    #endif
  #else
    #ifdef RGBW_MODE
      #define BITS_PER_PIXEL BITS_PER_PIXEL_RGBW
    #else
      #define BITS_PER_PIXEL BITS_PER_PIXEL_RGB
    #endif
  #endif
#else
  #define PIXEL_DELAY 0  // Will be overridden by jumper config
  #define BITS_PER_PIXEL 0  // Will be overridden by jumper config
#endif

#define TRUTH_PRIMARY     0b0010
#define TRUTH_SECONDARY   0b1000

#ifdef PRIMARY_OUTPUT_PASSTHROUGH
  #define TRUTH_PRIMARY     0b1010
#endif

#ifdef SWAP_OUTPUTS
  #define TRUTH0_DATA TRUTH_SECONDARY
  #define TRUTH1_DATA TRUTH_PRIMARY
#else
  #define TRUTH0_DATA TRUTH_PRIMARY
  #define TRUTH1_DATA TRUTH_SECONDARY
#endif

#ifdef DISABLE_OUTPUT_1
  #define TRUTH0_DATA 0b0000

  #if !defined(SWAP_OUTPUTS) && defined(PRIMARY_OUTPUT_PASSTHROUGH)
    #warning PRIMARY_OUTPUT_PASSTHROUGH has no effect when primary output (1) is disabled.
  #endif
#endif

#ifdef DISABLE_OUTPUT_2
  #define TRUTH1_DATA 0b0000

  #if defined(SWAP_OUTPUTS) && defined(PRIMARY_OUTPUT_PASSTHROUGH)
    #warning PRIMARY_OUTPUT_PASSTHROUGH has no effect when primary output (2) is disabled.
  #endif
#endif

#ifndef LED_RESET_TIMEOUT
  #define LED_RESET_TIMEOUT 50UL
#endif

#define CPU_CYCLES_FROM_MICROSECONDS(x) ((F_CPU / 1000000UL) * (x))

#endif  // __CONFIG_HPP__
