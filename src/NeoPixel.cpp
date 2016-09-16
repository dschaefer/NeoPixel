#include "NeoPixel.h"
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

#ifdef ESP8266
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

static cycles_t getClockCycles(void) __attribute__((always_inline));
static inline cycles_t getClockCycles(void) {
	cycles_t ccount;
	__asm__ __volatile__("rsr %0,ccount":"=a" (ccount));
	return ccount;
}

void usleep(int usecs) {
	cycles_t start = getClockCycles();
	while (getClockCycles() - start < usecs);
}

#define InterruptDisable taskDISABLE_INTERRUPTS
#define InterruptEnable taskENABLE_INTERRUPTS
#elif defined(__QNX__)
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <sys/mman.h>
#define IRAM_ATTR
#define HW_WRITE32(reg, value) *((volatile uint32_t *) (reg)) = value;
#define HW_READ32(reg) (*((volatile uint32_t *)(reg)))

static cycles_t getClockCycles(void) __attribute__((always_inline));
static inline cycles_t getClockCycles(void) {
	cycles_t ccount;
	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(ccount));
	return ccount;
}

#endif

NeoPixel::NeoPixel(uint32_t _numPixels, uint32_t _pin) {
	numPixels = _numPixels;
	pin = _pin;

	pixels = (uint8_t *) malloc(numPixels * 3);
	for (int i = 0; i < _numPixels; ++i) {
		setPixel(i, 0, 0, 0);
	}

	// set up pin and timers
#if defined(ESP8266)
	PIN_FUNC_SELECT(pinMap[pin].pin, pinMap[pin].func);
	GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, BIT(pin));
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, BIT(pin));

	cycles_t cpuFreq = system_get_cpu_freq() * 1000000;
#elif defined(__QNX__)
	// registers for gpio1
	gpio = (uint8_t *) mmap_device_memory(NULL, 0x1000, PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, 0x4804c000);

	// set for output
	uint32_t oe = HW_READ32(gpio + 0x134);
	oe &= ~(1 << pin);
	HW_WRITE32(gpio + 0x134, oe);

	// set output to zero
	HW_WRITE32(gpio + 0x190, 1 << pin);

	// Enable the cycle clock
	ThreadCtl(_NTO_TCTL_IO_PRIV, NULL);
	asm volatile ("mcr p15, 0, %0, c9, c14, 0\n\t" :: "r"(1));
	asm volatile ("mcr p15, 0, %0, c9, c12, 0\n\t" :: "r"(0x11));
	asm volatile ("mcr p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));
	asm volatile ("mcr p15, 0, %0, c9, c12, 3\t\n" :: "r"(0x8000000f));

	cycles_t cpuFreq = 1000000000; // 1GHz
#endif

	hitime0 = cpuFreq / 2500000;		// 0.4us
	hitime1 = cpuFreq / 1250000;		// 0.8us
	cycleTime = cpuFreq / 800000;	// 1.25us

	fprintf(stderr, "cpuFreq = %d\n", cpuFreq);
	fprintf(stderr, "cycleTime = %d\n", cycleTime);
	fprintf(stderr, "numPixels = %d\n", numPixels);
	fprintf(stderr, "pin = %d\n", pin);
}

NeoPixel::~NeoPixel() {
	free(pixels);
#if defined(__QNX__)
	munmap_device_memory(gpio, 0x1000);
#endif
}

void IRAM_ATTR NeoPixel::show() {
	uint8_t *end = pixels + numPixels * 3;

	// Wait for previous command to complete
	// TODO wait for wrap around too if necessary
	usleep(100);

	InterruptDisable();

	cycles_t start = getClockCycles();
	for (uint8_t *p = pixels; p != end; p++) {
		for (uint8_t mask = 0x80; mask; mask >>= 1) {
#if defined(ESP8266)
			GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1 << pin);
#elif defined(__QNX__)
			HW_WRITE32(gpio + 0x194, 1 << pin);
#endif
			cycles_t hitime = (*p & mask) ? hitime1 : hitime0;
			while (getClockCycles() - start < hitime);
#if defined(ESP8266)
			GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1 << pin);
#elif defined(__QNX__)
			HW_WRITE32(gpio + 0x190, 1 << pin);
#endif
			while (getClockCycles() - start < cycleTime);
			start += cycleTime;
		}
	}

	InterruptEnable();
}
