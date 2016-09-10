#include <NeoPixel.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(ESP8266)
#include <esp_common.h>
#define PIXEL_PIN	14
#define delay(ms) vTaskDelay(ms / portTICK_RATE_MS)
#define random os_random
#elif defined(__QNX__)
// gpio1[16]
#define PIXEL_PIN	16
#endif

#define NUM_PIXELS	4

NeoPixel pixels(NUM_PIXELS, PIXEL_PIN);

void drawPixels(void * arg) {
	// Wait for the power to settle
	delay(50);

	while (true) {
		uint8_t r = random();
		uint8_t g = random();
		uint8_t b = random();
		for (int i = 0; i < 4; ++i) {
			pixels.setPixel(i, r, g, b);
		}
		pixels.show();

		delay(1000);
	}
}

#if defined(ESP8266)
extern "C" void uart_div_modify(int, int);

extern "C" void user_init() {
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	printf("\n");

	xTaskCreate(drawPixels, (const signed char * ) "pixels", 256, NULL,
			tskIDLE_PRIORITY, NULL);
}
#elif defined(__QNX__)
int main(int argc, char **argv) {
	drawPixels(NULL);
}
#endif
