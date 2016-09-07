#include "NeoPixel.h"
#include <malloc.h>
#include <esp_common.h>
#include <freertos/FreeRTOS.h>

static struct {
	volatile uint32_t pin;
	uint32_t func;
} pinMap[16] = {
		{ PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0 },	// 0
		{ PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1 },	// 1
		{ PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2 },	// 2
		{ PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3 },	// 3
		{ PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4 },	// 4
		{ PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5 },	// 5
		{ PERIPHS_IO_MUX_SD_CLK_U, FUNC_GPIO6 },	// 6
		{ PERIPHS_IO_MUX_SD_DATA0_U, FUNC_GPIO7 },	// 7
		{ PERIPHS_IO_MUX_SD_DATA1_U, FUNC_GPIO8 },	// 8
		{ PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9 },	// 9
		{ PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10 },	// 10
		{ PERIPHS_IO_MUX_SD_CMD_U, FUNC_GPIO11 },	// 11
		{ PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12 },	// 12
		{ PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13 },	// 13
		{ PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14 },	// 14
		{ PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15 },	// 15
};

#define CYCLES_T0H  (configCPU_CLOCK_HZ / 2500000) // 0.4us
#define CYCLES_T1H  (configCPU_CLOCK_HZ / 1250000) // 0.8us
#define CYCLES      (configCPU_CLOCK_HZ /  800000) // 1.25us per bit
#define RET			(configCPU_CLOCK_HZ /   20000) // 50us

static uint32_t getCycleCount(void) __attribute__((always_inline));
static inline uint32_t getCycleCount(void) {
	uint32_t ccount;
	__asm__ __volatile__("rsr %0,ccount":"=a" (ccount));
	return ccount;
}

NeoPixel::NeoPixel(uint8_t _numPixels, uint8_t _pin) {
	numPixels = _numPixels;
	pin = _pin;

	pixels = (uint8_t *) malloc(numPixels * 3);
	for (int i = 0; i < _numPixels; ++i) {
		setPixel(i, 0, 0, 0);
	}

	// set up pin
	PIN_FUNC_SELECT(pinMap[pin].pin, pinMap[pin].func);
	GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, BIT(pin));
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, BIT(pin));
	uint32_t start = getCycleCount();
	while (getCycleCount() - start < RET);
	lastTime = getCycleCount();
}

NeoPixel::~NeoPixel() {
	free(pixels);
}

void IRAM_ATTR NeoPixel::show() {
	uint32_t pinMask = BIT(pin);
	uint8_t *end = pixels + numPixels * 3;

	// Wait for previous command to complete
	// TODO wait for wrap around too if necessary
	while (getCycleCount() - lastTime < RET);

	taskDISABLE_INTERRUPTS();

	uint32_t start = getCycleCount();
	for (uint8_t *p = pixels; p != end; p++) {
		for (uint8_t mask = 0x80; mask; mask >>= 1) {
			GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinMask);
			uint32_t hitime = (*p & mask) ? CYCLES_T1H : CYCLES_T0H;
			while (getCycleCount() - start < hitime);
			GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinMask);
			while (getCycleCount() - start < CYCLES);
			start += CYCLES;
		}
	}

	taskENABLE_INTERRUPTS();

	lastTime = getCycleCount();
}
