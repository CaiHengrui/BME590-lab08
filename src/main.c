#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(mhealth_tymp, LOG_LEVEL_DBG);

#define DEC_ON_TIME_MS 100
#define INC_ON_TIME_MS 100

#define MIN_ON_TIME_MS 100
#define MAX_ON_TIME_MS 2000

#define HEARTBEAT_PERIOD_MS 500


static bool reset_event = 0;
static bool sleep_event = 0;
static bool detect_sleep_event = 0;
static bool freq_up_event = 0;
static bool freq_down_event = 0;
static bool detect_timer = 0;

int LED_ON_TIME_MS = 1000;

//LEDs//
static const struct gpio_dt_spec heartbeat_led = GPIO_DT_SPEC_GET(DT_ALIAS(heartbeat), gpios);
static const struct gpio_dt_spec buzzer_led = GPIO_DT_SPEC_GET(DT_ALIAS(buzzer), gpios);
static const struct gpio_dt_spec ivdrip_led = GPIO_DT_SPEC_GET(DT_ALIAS(ivdrip), gpios);
static const struct gpio_dt_spec alarm_led = GPIO_DT_SPEC_GET(DT_ALIAS(alarm), gpios);
static const struct gpio_dt_spec error_led = GPIO_DT_SPEC_GET(DT_ALIAS(error), gpios);

//buttons//
static const struct gpio_dt_spec sleep = GPIO_DT_SPEC_GET(DT_ALIAS(button0), gpios);
static const struct gpio_dt_spec freq_up = GPIO_DT_SPEC_GET(DT_ALIAS(button1), gpios);
static const struct gpio_dt_spec freq_down = GPIO_DT_SPEC_GET(DT_ALIAS(button2), gpios);
static const struct gpio_dt_spec reset = GPIO_DT_SPEC_GET(DT_ALIAS(button3), gpios);

//Initialize GPIO Callback struct
static struct gpio_callback sleep_cb;
static struct gpio_callback freq_up_cb;
static struct gpio_callback freq_down_cb;
static struct gpio_callback reset_cb;

/* Declarations */
//declare callback function
void sleep_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void freq_up_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void freq_down_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void reset_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

//declare timers
void heartbeat_toggle(struct k_timer *heartbeat_timer);
void buzzer_cycle(struct k_timer *buzzer_timer);
void stop_buzzer_cycle(struct k_timer *buzzer_timer);
void ivdrip_cycle(struct k_timer *ivdrip_timer);
void stop_ivdrip_cycle(struct k_timer *ivdrip_timer);
void alarm_cycle(struct k_timer *alarm_timer);
void stop_alarm_cycle(struct k_timer *alarm_timer);

/* Callbacks */
void sleep_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	sleep_event = 1;
}

void freq_up_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	freq_up_event = 1;
}

void freq_down_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	freq_down_event = 1;
}

void reset_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	reset_event = 1;
}



/* Timers */
K_TIMER_DEFINE(heartbeat_timer, heartbeat_toggle, NULL);
K_TIMER_DEFINE(buzzer_timer, buzzer_cycle, stop_buzzer_cycle);
K_TIMER_DEFINE(ivdrip_timer, ivdrip_cycle, stop_ivdrip_cycle);
K_TIMER_DEFINE(alarm_timer, alarm_cycle, stop_alarm_cycle);

void heartbeat_toggle(struct k_timer *heartbeat_timer){
	    gpio_pin_toggle_dt(&heartbeat_led);
    LOG_DBG("Heartbeat Toggled");

}

void buzzer_cycle(struct k_timer *buzzer_timer){
	    gpio_pin_set_dt(&buzzer_led,1);
		gpio_pin_set_dt(&ivdrip_led,0);
		gpio_pin_set_dt(&alarm_led,0);
    LOG_DBG("buzzer");

}
void stop_buzzer_cycle(struct k_timer *buzzer_timer){
	    gpio_pin_set_dt(&buzzer_led,0);
		gpio_pin_set_dt(&ivdrip_led,0);
		gpio_pin_set_dt(&alarm_led,0);
    LOG_DBG("stop_buzzer");

}

void ivdrip_cycle(struct k_timer *ivdrip_timer){
	    gpio_pin_set_dt(&buzzer_led,0);
		gpio_pin_set_dt(&ivdrip_led,1);
		gpio_pin_set_dt(&alarm_led,0);
    LOG_DBG("ivdrip");

}
void stop_ivdrip_cycle(struct k_timer *ivdrip_timer){
	    gpio_pin_set_dt(&buzzer_led,0);
		gpio_pin_set_dt(&ivdrip_led,0);
		gpio_pin_set_dt(&alarm_led,0);
    LOG_DBG("stop_ivdrip");

}

void alarm_cycle(struct k_timer *alarm_timer){
	    gpio_pin_set_dt(&buzzer_led,0);
		gpio_pin_set_dt(&ivdrip_led,0);
		gpio_pin_set_dt(&alarm_led,1);
    LOG_DBG("alarm");

}
void stop_alarm_cycle(struct k_timer *alarm_timer){
	    gpio_pin_set_dt(&buzzer_led,0);
		gpio_pin_set_dt(&ivdrip_led,0);
		gpio_pin_set_dt(&alarm_led,0);
    LOG_DBG("stop_alarm");

}


void main(void)
{
	
	if (!device_is_ready(heartbeat_led.port)) {
		LOG_ERR("gpio0 interface not ready.");
		return -1;
	}

	if (!device_is_ready(error_led.port)) {
		LOG_ERR("gpio1 interface not ready.");
		return -1;
	}

	int err;
	/* Configure  GPIO pins */
	err = gpio_pin_configure_dt(&heartbeat_led, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Cannot configure heartbeat LED.");
		return err;
	}

	err = gpio_pin_configure_dt(&buzzer_led, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Cannot configure buzzer LED.");
		return err;
	}

	err = gpio_pin_configure_dt(&ivdrip_led, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Cannot configure ivdrip LED.");
		return err;
	}

	err = gpio_pin_configure_dt(&alarm_led, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Cannot configure alarm LED.");
		return err;
	}

	err = gpio_pin_configure_dt(&error_led, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Cannot configure error LED.");
		return err;
	}

	err = gpio_pin_configure_dt(&sleep, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Cannot configure sleep button.");
		return err;
	}
	
	err = gpio_pin_configure_dt(&freq_up, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Cannot configure frequency up button.");
		return err;
	}

	err = gpio_pin_configure_dt(&freq_down, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Cannot configure frequency down button.");
		return err;
	}

	err = gpio_pin_configure_dt(&reset, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Cannot configure reset button.");
		return err;
	}

	/* Setup callbacks *//*callback for limits*/
    err = gpio_pin_interrupt_configure_dt(&sleep, GPIO_INT_EDGE_TO_ACTIVE);	
	if (err < 0) {
		LOG_ERR("Cannot attach callback to sleep button.");		
	}
	gpio_init_callback(&sleep_cb, sleep_callback, BIT(sleep.pin));
	gpio_add_callback(sleep.port, &sleep_cb);

    err = gpio_pin_interrupt_configure_dt(&freq_up, GPIO_INT_EDGE_TO_ACTIVE);
	if (err < 0) {
		LOG_ERR("Cannot attach callback to frequency up button.");		
	}		
	gpio_init_callback(&freq_up_cb, freq_up_callback, BIT(freq_up.pin));
	gpio_add_callback(freq_up.port, &freq_up_cb);

	err = gpio_pin_interrupt_configure_dt(&freq_down, GPIO_INT_EDGE_TO_ACTIVE);	
	if (err < 0) {
		LOG_ERR("Cannot attach callback to frequency down button.");		
	}
	gpio_init_callback(&freq_down_cb, freq_down_callback, BIT(freq_down.pin));
	gpio_add_callback(freq_down.port, &freq_down_cb);

	err = gpio_pin_interrupt_configure_dt(&reset, GPIO_INT_EDGE_TO_ACTIVE);	
	if (err < 0) {
		LOG_ERR("Cannot attach callback to reset button.");		
	}	
	gpio_init_callback(&reset_cb, reset_callback, BIT(reset.pin));
	gpio_add_callback(reset.port, &reset_cb);


/* Start indefinite timers */
	k_timer_start(&heartbeat_timer, K_MSEC(HEARTBEAT_PERIOD_MS), K_MSEC(HEARTBEAT_PERIOD_MS));

	while (1) {
		if (sleep_event== 1){
			printk("sleep!");
			if ( detect_sleep_event == 0){
				detect_sleep_event = 1;
				sleep_event=0;
			}
			else if ( detect_sleep_event == 1)
			{
				detect_sleep_event = 0;
				freq_up_event = 0;
				freq_down_event =0;
				sleep_event=0;
				reset_event = 0;
				detect_timer=0;
			}	
			printk("led %d \n\r",detect_sleep_event);	
		}



		if((LED_ON_TIME_MS >= MIN_ON_TIME_MS) && (LED_ON_TIME_MS <= MAX_ON_TIME_MS)){
			if(detect_sleep_event == 0){
				if(detect_timer == 0){
				k_timer_start(&buzzer_timer, K_MSEC(LED_ON_TIME_MS), K_MSEC(LED_ON_TIME_MS*3));
				k_timer_start(&ivdrip_timer, K_MSEC(LED_ON_TIME_MS*2), K_MSEC(LED_ON_TIME_MS*3));
				k_timer_start(&alarm_timer, K_MSEC(LED_ON_TIME_MS*3), K_MSEC(LED_ON_TIME_MS*3));
				detect_timer = 1;
				printk("led action \n\r");
				}
				
				if (freq_up_event) {
					printk("freq_up!");
					LED_ON_TIME_MS = LED_ON_TIME_MS - DEC_ON_TIME_MS ;
					printk("led %d \n\r",LED_ON_TIME_MS);
					freq_up_event = 0;
					detect_timer = 0;
				}

				else if (freq_down_event) {printk("freq_down!");
					LED_ON_TIME_MS = LED_ON_TIME_MS + INC_ON_TIME_MS ;
					printk("led %d \n\r",LED_ON_TIME_MS);
					freq_down_event = 0;
					detect_timer = 0;
				}
			}
			if(detect_sleep_event == 1){
				k_timer_stop(&buzzer_timer);
				k_timer_stop(&alarm_timer);
				k_timer_stop(&ivdrip_timer);
			}
		}

		if((LED_ON_TIME_MS < MIN_ON_TIME_MS) || (LED_ON_TIME_MS > MAX_ON_TIME_MS)){
			if(detect_sleep_event == 0){
				k_timer_stop(&buzzer_timer);
				k_timer_stop(&alarm_timer);
				k_timer_stop(&ivdrip_timer);
				gpio_pin_set_dt(&buzzer_led,0);
				gpio_pin_set_dt(&ivdrip_led,0);
				gpio_pin_set_dt(&alarm_led,0);
				gpio_pin_set_dt(&error_led,1); 
				printk("error!");
				detect_timer = 1;

				if(reset_event) {printk("reset!");
				LED_ON_TIME_MS = 1000;
				gpio_pin_set_dt(&error_led,0);
				reset_event = 0;
				detect_timer = 0;
				}
			}
		}
	}
}
