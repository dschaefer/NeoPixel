#include "NeoPixel.h"
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

#ifdef ESP8266
#include <esp_common.h>
#include <GPIO.h>

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
#elif defined(__QNX__)
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <sys/mman.h>
#include <sys/procmgr.h>
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

NeoPixel::NeoPixel(uint32_t _numPixels, uint8_t _pin)
: numPixels(_numPixels)
, pin(_pin) {
	pin.pinMode(GPIO::Output);
	pin.digitalWrite(GPIO::Low);

	pixels = (uint8_t *) malloc(numPixels * 3);
	for (int i = 0; i < _numPixels; ++i) {
		setPixel(i, 0, 0, 0);
	}

	// set timers
	cycles_t cpuFreq;
#if defined(ESP8266)
	cpuFreq = system_get_cpu_freq() * 1000000;
#elif defined(__QNXNTO__)
	// Enable the cycle clock
	procmgr_ability(0, PROCMGR_AID_IO);
	ThreadCtl(_NTO_TCTL_IO_PRIV, NULL);
	asm volatile ("mcr p15, 0, %0, c9, c14, 0\n\t" :: "r"(1));
	asm volatile ("mcr p15, 0, %0, c9, c12, 0\n\t" :: "r"(0x11));
	asm volatile ("mcr p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));
	asm volatile ("mcr p15, 0, %0, c9, c12, 3\t\n" :: "r"(0x8000000f));

	cpuFreq = 1000000000; // 1GHz
#endif

	hitime0 = cpuFreq / 2500000;		// 0.4us
	hitime1 = cpuFreq / 1250000;		// 0.8us
	cycleTime = cpuFreq / 800000;	// 1.25us
}

NeoPixel::~NeoPixel() {
	free(pixels);
}

void IRAM_ATTR NeoPixel::show() {
	uint8_t *end = pixels + numPixels * 3;

	// Wait for previous command to complete
	// TODO wait for wrap around too if necessary
	pin.digitalWrite(GPIO::Low);
	usleep(200);

#if defined(ESP8266)
	taskDISABLE_INTERRUPTS();
#elif defined(__QNXNTO__)
	intrspin_t spinner;
	InterruptLock(&spinner);
#endif

	cycles_t start = getClockCycles();
	for (uint8_t *p = pixels; p != end; p++) {
		for (uint8_t mask = 0x80; mask; mask >>= 1) {
			pin.digitalWrite(GPIO::High);
			cycles_t hitime = (*p & mask) ? hitime1 : hitime0;
			while (getClockCycles() - start < hitime);
			pin.digitalWrite(GPIO::Low);
			while (getClockCycles() - start < cycleTime);
			start += cycleTime;
		}
	}

#if defined(ESP8266)
	taskENABLE_INTERRUPTS();
#elif defined(__QNXNTO__)
	InterruptUnlock(&spinner);
#endif

}
