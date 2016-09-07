#ifndef _NEOPIXEL_H_
#define _NEOPIXEL_H_

#include <stdint.h>

class NeoPixel {
public:
	NeoPixel(uint8_t numPixels, uint8_t pin);
	~NeoPixel();

	void setPixel(uint8_t pixel, uint8_t r, uint8_t g, uint8_t b) {
		// grb
		pixels[pixel * 3 + 0] = g;
		pixels[pixel * 3 + 1] = r;
		pixels[pixel * 3 + 2] = b;
	}

	void show();

private:
	uint8_t *pixels;
	uint8_t numPixels;
	uint8_t pin;
	uint32_t lastTime;
};

#endif // _NEOPIXEL_H_
