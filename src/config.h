// Configuration

// Display Configuration
#ifdef NAYAD_V1
#define I2C_SDA 02
#define I2C_SCL 04
#elif defined(NAYAD_V1R2) || defined(T_BEAM_V10) || defined(T_BEAM_LORA_32) || defined(T_BEAM_V12)
#define I2C_SDA SDA
#define I2C_SCL SCL
#else
#warning "I2C_SDA and I2C_SCL not defined"
#define I2C_SDA 0
#define I2C_SCL 0
#endif

// Display Configuration
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_ADDRESS 0x3C

#if defined(T_BEAM_LORA_32)
#define DISPLAY_SDA 4
#define DISPLAY_SCL 15
#define DISPLAY_RST 16
#elif defined(NAYAD_V1) || defined(NAYAD_V1R2) || defined(T_BEAM_V10) || defined(T_BEAM_V12)
#define DISPLAY_SDA I2C_SDA
#define DISPLAY_SCL I2C_SCL
#define DISPLAY_RST -1
#else
#warning "DISPLAY_SDA and DISPLAY_SCL not defined"
#define DISPLAY_SDA 0
#define DISPLAY_SCL 0
#define DISPLAY_RST -1
#endif

// Led configuration
#if defined(NAYAD_V1)
#define LED 4
#define LED_ON      LOW
#define LED_OFF     HIGH
#elif defined(T_BEAM_LORA_32)
#define LED 2
#define LED_ON      HIGH
#define LED_OFF     LOW
#elif defined(T_BEAM_V10) || defined(T_BEAM_V12)
#define LED 4
#define LED_ON      LOW
#define LED_OFF     HIGH
#elif defined(NAYAD_V1R2)
#define LED 13
#define LED_ON      HIGH
#define LED_OFF     LOW
#else
#warning "LED not defined"
#define LED 255U
#define LED_ON      HIGH
#define LED_OFF     LOW
#endif
