#include <stdint.h>
// #include <stdio.h>
// #include "log.h"
// #include "bits.h"
// 
// #include "pico/stdlib.h"

typedef uint32_t u32;

#define LED_PIN 2

//
// Purpose that base metal boot in 48MHz (48,000,000 tick in one second) without
// clock configuration
//
#define CLK_SPEED 48000000
#define HALF_SECOND_DELAY (u32)(CLK_SPEED / 2)
#define ONE_QUARTER_SECOND_DELAY (u32)(CLK_SPEED / 4)
#define ONE_EIGHTH_SECOND_DELAY (u32)(CLK_SPEED / 8)

//
// ./src/rp2_common/hardware_base/include/hardware/address_mapped.h
//
// typedef volatile uint32_t reg_u32;
typedef volatile u32 reg_u32;

// //
// // Atomic Register Access
// //
// #define ATOMIC_NORMAL_READ_WRITE_OFFSET 0x0000
// #define ATOMIC_XOR_ON_WRITE_OFFSET		0x1000
// #define ATOMIC_SET_ON_WRITE_OFFSET		0x2000
// #define ATOMIC_CLEAR_ON_WRITE_OFFSET	0x3000

//
// Page 177 -> 2.14.3
//
// Reset Registers
//
#define RESET_BASE_ADDR 0x4000c000
#define RESET_CONTROL_ADDR (RESET_BASE_ADDR + 0x00)
#define RESET_WATCH_DOG_SELCECT_ADDR (RESET_BASE_ADDR + 0x04)
#define RESET_DONE_ADDR (RESET_BASE_ADDR + 0x08)

//
// Page 243 -> 2.19.6.1 IO - User Bank
//
// GPIO Registers
//
#define GPIO_BASE_ADDR 0x40014000
#define GPIO_0_CONTROL_ADDR (GPIO_BASE_ADDR + 0x004)
#define GPIO_2_CONTROL_ADDR (GPIO_BASE_ADDR + 0x014)

//
// Page 42 -> 2.3.17
//
// SIO GPIO Registers
//
#define SIO_BASE_ADDR 0xd0000000
#define SIO_GPIO_OUT_ENABLE_ADDR (SIO_BASE_ADDR + 0x20)
#define SIO_GPIO_OUT_SET_ADDR (SIO_BASE_ADDR + 0x14)
#define SIO_GPIO_OUT_CLEAR_ADDR (SIO_BASE_ADDR + 0x18)

//
// Simulate `delay` function
//
void simulate_delay(void) {
    volatile uint32_t _ = 0;
    for (uint32_t i = 0; i < ONE_EIGHTH_SECOND_DELAY; i++) {
        _ = i;
    }
}

///
///
///
void enable_gpio_and_wait_for_it_stable(void) {
    //
    // Page 178:
    //
    // Reset control register: Set the bit5 to `0` to enable GPIO (iobank0)
    // functionality
    //
    // If your project links to `pico_stdlib` (in `CMakeLists.txt`), then somehow SDK
    // enables all functionalities by default. This can be confirmed by the following
	// steps:
	//
	// - If you print `*reset_control_reg` register value, it will be 0x00
	//
	// - If you read the `Reset done resgiter` value, it should be:
	//   `0000 0001 1111 1111 1111 1111 1111 1111` (bit0~bit24)
    //   It means all peripherals are ready to be used!!!
    //
    // To avoid that happens, set the `Reset control register` value to the following:
    //
    // bit24 ~ bit0: 0000 0001 1111 1111 1111 1111 1101 1111 -> 0x01 FF FF DF
	// bit23 ~ bit0: 0000 0000 1100 0011 0000 1100 0101 1001 -> 0x00 C3 0C 59
    //
    // Only bit5 is 0 to enable GPIO and disable all the other peripherals.
    //
    reg_u32 *reset_control_reg = (reg_u32 *)RESET_CONTROL_ADDR;

    // printf("\n>>> Reset control register value:");
    // PRINT_BITS(*reset_control_reg);

	//
	// DO NOT DO THIS!!!
	// As I don't konw why this will halt the MCU if try to set the value to I wanted???
	//
    // *reset_control_reg = 0x01FFFFDF;
    // *reset_control_reg = 0x00C30C59;

    // printf("\n>>> Reset control register value after reset GPIO:");
    // PRINT_BITS(*reset_control_reg);

    //
    // Reset done register:
    //
    // When resetting GPIO functionality has been done, then the bit5 in this
    // register will become `1`
    //

    // printf("\n>>> Waiting for GPIO reset done......");
    reg_u32 *reset_done_reg = (reg_u32 *)RESET_DONE_ADDR;

    // printf("\n>>> Rest done register value:");
    // PRINT_BITS(*reset_done_reg);
    while (!(*reset_done_reg & (1 << 5))) {
        //
    }
    // printf("\n>>> GPIO rest is done.");

    //
    // GPIO_X (pin) control register:
    //
    // Page 236 -> 2.19.2 Function Select:
    //
    // Each GPIO has multiple functionalites, you need to select the function you
    // needed before using it.
    //
    // Page 246(bottom)~247(top) -> IO_BANK0: GPIO0_CTRL ... CPIO29_CTRL
    // Registers:
    //
    // Set `0x5` (0000 0101) to bit0~bit4 to select the `SIO` function
    //

    // For example: select SIO function for the GPIO_0
    // reg_u32 *gpio_0_control_reg = (reg_u32*)GPIO_0_CONTROL_ADDR;
    // *gpio_0_control_reg = 0x05;

    // For example: select SIO function for the GPIO_2
    reg_u32 *gpio_2_control_reg = (reg_u32 *)GPIO_2_CONTROL_ADDR;
    *gpio_2_control_reg = 0x05;

    //
    // Page 46 -> 2.3.17, SIO: GPIO_OE Register
    //
    // SIO GPIO out enable register: Set the bit{LED_PIN} to 1 to enable
    // GPIO_{LED_PIN} output mode
    //
    reg_u32 *gpio_out_enable_reg = (reg_u32 *)SIO_GPIO_OUT_ENABLE_ADDR;
    *gpio_out_enable_reg |= (1 << LED_PIN);
}

//
//
//
void blinking_led_loop(void) {
    reg_u32 *gpio_out_set_reg = (reg_u32 *)SIO_GPIO_OUT_SET_ADDR;
    reg_u32 *gpio_out_clear_reg = (reg_u32 *)SIO_GPIO_OUT_CLEAR_ADDR;

    while (1) {
        //
        // Page 46 -> SIO: GPIO_OUT_SET Register
        //
        // SIO GPIO out set register: set bit{LED_PIN} to 1 for setting
        // GPIO_{LED_PIN} to high
        //
        *gpio_out_set_reg |= (1 << LED_PIN);
        simulate_delay();

        //
        // Page 46 -> SIO: GPIO_OUT_CLR Register
        //
        // SIO GPIO clear set register: set bit{LED_PIN} to 1 for setting
        // GPIO_{LED_PIN} to low
        //
        *gpio_out_clear_reg |= (1 << LED_PIN);
        simulate_delay();
    }
}

//
//
//
int main(void) {
    // stdio_init_all();
    // printf("\n>>> [ blinking-led-register ]\n");

    enable_gpio_and_wait_for_it_stable();
    blinking_led_loop();

    return 0;
}
