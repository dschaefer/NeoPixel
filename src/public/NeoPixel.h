#ifndef _NEOPIXEL_H_
#define _NEOPIXEL_H_

#include <stdint.h>
#include <GPIO.h>

#if defined(ESP8266)
typedef uint32_t cycles_t;
#elif defined(__QNX__)
typedef uint32_t cycles_t;
#endif

class NeoPixel {
public:
	NeoPixel(uint32_t numPixels, uint8_t pin);
	~NeoPixel();

	void setPixel(uint32_t pixel, uint8_t r, uint8_t g, uint8_t b) {
		// grb
		pixels[pixel * 3 + 0] = g;
		pixels[pixel * 3 + 1] = r;
		pixels[pixel * 3 + 2] = b;
	}

	void show();

	uint32_t getNumPixels() { return numPixels; }

private:
	GPIO pin;
	uint8_t *pixels;
	uint32_t numPixels;
	cycles_t hitime0, hitime1, cycleTime;
};

#endif // _NEOPIXEL_H_
