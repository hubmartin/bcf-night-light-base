
#include <application.h>
#include <bc_module_encoder.h>

#define COUNT 144

// Instance of PCB LED and BUTTON
bc_led_t led;
bc_button_t button;

// LED strip stuff
bc_led_strip_t led_strip;
static uint32_t _dma_buffer[COUNT * 4 * 2]; // count * type * 2

const bc_led_strip_buffer_t _led_strip_buffer =
{
    .type = BC_LED_STRIP_TYPE_RGBW,
    .count = COUNT,
    .buffer = _dma_buffer
};

bool led_strip_on = false;
uint8_t led_strip_brightness = 64;

void led_update(bool led_strip_on, uint8_t led_strip_brightness)
{

	// This is for RGB LED strip;
  //uint32_t color = led_strip_brightness << 16 | led_strip_brightness << 8 | led_strip_brightness << 0;

	// This is for RGBW LED strip
  uint32_t color = led_strip_brightness;

  // Turn off when led strip is off
  if(!led_strip_on)
  {
    color = 0x00;
  } else if(color == 0)
  {
		// Minimal turn on brightness
    color = 0x02;
  }

	// Update LED strip
  bc_led_strip_fill(&led_strip, color);
  bc_led_strip_write(&led_strip);

}

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *param)
{
    (void) self;
    (void) param;
		// Turn off/on the LED strip
    if (event == BC_BUTTON_EVENT_CLICK)
    {
      bc_led_pulse(&led, 100);
      led_strip_on = !led_strip_on;
      led_update(led_strip_on, led_strip_brightness);
    }

		// Do the pairing stuff
    if (event == BC_BUTTON_EVENT_HOLD)
    {
        bc_radio_enrollment_start();
        bc_led_set_mode(&led, BC_LED_MODE_BLINK_FAST);
    }
}

// On incoming BUTTON message from REMOTE
void bc_radio_on_push_button(uint16_t *event_count)
{
    (void) event_count;
    bc_led_pulse(&led, 100);
    led_strip_on = !led_strip_on;
    led_update(led_strip_on, led_strip_brightness);
}

// Blink LED when REMOTE is paired (ATTACH) or unpaired (DETACH)
void radio_event_handler(bc_radio_event_t event, void *event_param)
{
    (void) event_param;

    if (event == BC_RADIO_EVENT_ATTACH || event == BC_RADIO_EVENT_DETACH)
    {
        bc_led_pulse(&led, 1000);
        bc_led_set_mode(&led, BC_LED_MODE_OFF);
    }
}

void encoder_event_handler(bc_module_encoder_event_t event, void *param)
{
  (void) param;

    if(event == BC_MODULE_ENCODER_EVENT_ROTATION)
    {
        int32_t check = led_strip_brightness + (bc_module_encoder_get_increment() * 8);
        if(check > 255)
        {
            check = 255;
        }
        else if(check < 0)
        {
            check = 0;
        }

        led_strip_brightness = check;
        led_update(led_strip_on, led_strip_brightness);
    }
}


void application_init(void)
{
	// This module is not powered from battery so disable sleep
	bc_scheduler_disable_sleep();

	// Init Radio
	bc_radio_init();
	bc_radio_set_event_handler(radio_event_handler, NULL);
	//bc_radio_listen();

	// PCB LED
	bc_led_init(&led, BC_GPIO_LED, false, false);

	// PCB BUTTON
	bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
	bc_button_set_hold_time(&button, 1000);
	bc_button_set_event_handler(&button, button_event_handler, NULL);

	// Encoder
	bc_module_encoder_init();
	bc_module_encoder_set_event_handler(encoder_event_handler, NULL);

	// LED strip
	bc_module_power_init();
	bc_led_strip_init(&led_strip, bc_module_power_get_led_strip_driver(), &_led_strip_buffer);

	bc_led_strip_fill(&led_strip, 0x00000000);
	bc_led_strip_write(&led_strip);
}
