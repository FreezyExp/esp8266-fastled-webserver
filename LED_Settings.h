#ifndef _LED_SETTINGS_h
#define _LED_SETTINGS_h


//#define FASTLED_ALLOW_INTERRUPTS 1
#define INTERRUPT_THRESHOLD 1
#define FASTLED_INTERRUPT_RETRY_COUNT 1
#define FASTLED_ALLOW_INTERRUPTS 1
#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_ESP8266_DMA

#define DATA_PIN      D7
#define LED_TYPE      WS2813
#define LED_CORRECTION TypicalLEDStrip
#define COLOR_ORDER   BRG
#define NUM_LEDS      150

#define MILLI_AMPS         15000 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND  60  // here you can control the speed. With the Access Point / Web Server the animations run a bit slower.

#endif
