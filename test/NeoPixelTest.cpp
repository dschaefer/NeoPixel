#include <NeoPixel.h>

#include <esp_common.h>

typedef uint32_t uint32;

extern "C" void uart_div_modify(int, int);

#define NUM_PIXELS	4
#define PIXEL_PIN	14

NeoPixel pixels(NUM_PIXELS, PIXEL_PIN);

void drawPixels(void * arg) {
	// Wait for the power to settle
	vTaskDelay(50 / portTICK_RATE_MS);

	while (true) {
		uint8_t r = os_random();
		uint8_t g = os_random();
		uint8_t b = os_random();
		for (int i = 0; i < 4; ++i) {
			pixels.setPixel(i, r, g, b);
		}
		pixels.show();

		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

extern "C" void user_init() {
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	printf("\n");

	xTaskCreate(drawPixels, (const signed char * ) "pixels", 256, NULL,
			tskIDLE_PRIORITY, NULL);
}
