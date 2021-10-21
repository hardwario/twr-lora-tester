#include <application.h>

#include "at.h"

#define GPS_TIMEOUT_MS (15 * 60 * 1000)
#define TEMPERATURE_MEASUREMENT_PERIOD_MS (1 * 60 * 1000)

// LED instance
twr_led_t led;

// Button instance
twr_button_t button;

twr_button_t button_left;
twr_button_t button_right;

// Lora instance
twr_cmwx1zzabz_t lora;

// Thermometer instance
twr_tmp112_t tmp112;

// GPS
twr_module_gps_time_t gps_time;
twr_module_gps_position_t gps_position;
twr_module_gps_altitude_t gps_altitude;
twr_module_gps_quality_t gps_quality;

twr_led_t gps_led_r;
twr_led_t gps_led_g;

float temperature;

bool lora_send_confimed_message = false;


bool at_send(void);
bool at_status(void);

char str_status[16] = "INIT...";
char str_info[32] = "";
float battery_voltage;
bool gps_present = false;
bool gps_sleep = false;
twr_tick_t gps_tick = 0;

bool reset_flag = false;

int32_t rfq_rssi;
int32_t rfq_snr;
uint32_t frame_counter_up;
uint32_t frame_counter_down;

int lora_received = 0;
int lora_tx_packet_length = 1;

int lora_packet_counter = 0;

twr_tick_t task_tx_period_delay = 0;
twr_scheduler_task_id_t task_tx_period_id;

void set_packet_info(void)
{
    snprintf(str_info, sizeof(str_info),"RSSI%d,SNR%d,C%d,%d", (int)rfq_rssi, (int)rfq_snr, (int)frame_counter_up, (int)frame_counter_down);
}

void clear_packet_info(void)
{
    rfq_rssi = 0;
    rfq_snr = 0;
    frame_counter_down = 0;
    frame_counter_up = 0;
}

void lcd_event_handler(twr_module_lcd_event_t event, void *event_param)
{
    twr_scheduler_plan_now(0);

    if (gps_present)
    {
        gps_tick = twr_tick_get();

        // If sleeping
        if (gps_sleep)
        {
            // Start GPS after any button event
            twr_module_gps_start();
            gps_sleep = false;
            twr_atci_printfln("$GPS: \"START\"");
        }
    }

    switch(event)
    {
        case TWR_MODULE_LCD_EVENT_LEFT_CLICK:
            twr_cmwx1zzabz_link_check(&lora);
        strcpy(str_status, "CHECK...");

        break;

        case TWR_MODULE_LCD_EVENT_LEFT_HOLD:
            twr_cmwx1zzabz_join(&lora);
            strcpy(str_status, "JOIN...");
        break;

        case TWR_MODULE_LCD_EVENT_RIGHT_CLICK:
            at_send();
        break;

        case TWR_MODULE_LCD_EVENT_RIGHT_HOLD:

        break;

        case TWR_MODULE_LCD_EVENT_BOTH_HOLD:
            // Reset must be handled later when the both buttons are release, because after reboot when any button is pressed
            // the BOOT pin is also HIGH, which causes start of bootloader instead of the application
            reset_flag = true;
            strcpy(str_status, "RESET");
        break;

        default:
        break;

    }

}

void lora_ready_params_udpate()
{
    /*twr_cmwx1zzabz_config_band_t band = twr_cmwx1zzabz_get_band(&lora);
    if (band > 1)
    {
        // skip 3 unused bands
        band -= 3;
    }
    strncpy(m_lora_band_str, menu_band.items[band]->text[0], sizeof(m_lora_band_str));

    uint8_t public = twr_cmwx1zzabz_get_nwk_public(&lora);
    strncpy(m_lora_nwk_str, public ? "public" : "private", sizeof(m_lora_nwk_str));

    twr_cmwx1zzabz_config_class_t class = twr_cmwx1zzabz_get_class(&lora);
    strncpy(m_lora_class_str, (class == TWR_CMWX1ZZABZ_CONFIG_CLASS_A) ? "A" : "C", sizeof(m_lora_class_str));

    int datarate = twr_cmwx1zzabz_get_datarate(&lora);
    strcpy(m_lora_datarate_str, menu_datarate.items[datarate]->text[0]);
*/
}



void lora_callback(twr_cmwx1zzabz_t *self, twr_cmwx1zzabz_event_t event, void *event_param)
{
    twr_scheduler_plan_now(0);

    if (event == TWR_CMWX1ZZABZ_EVENT_ERROR)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_BLINK_FAST);
        strcpy(str_status, "ERR");

        strncpy(str_info, twr_cmwx1zzabz_get_fw_version(&lora), sizeof(str_info));
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_SEND_MESSAGE_START)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_ON);
        strcpy(str_status, "SENDING...");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_SEND_MESSAGE_DONE)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_OFF);
        strcpy(str_status, "SENT...");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_MESSAGE_CONFIRMED)
    {
        strcpy(str_status, "ACK");

        // RFQ + Frame counters chain
        twr_cmwx1zzabz_rfq(&lora);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_MESSAGE_NOT_CONFIRMED)
    {
        strcpy(str_status, "NACK");

        // RFQ + Frame counters chain
        twr_cmwx1zzabz_rfq(&lora);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_READY)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_OFF);

       static bool ready_flag = false;

        if (!ready_flag)
        {
            strcpy(str_status, "READY");
            ready_flag = true;
        }

        strncpy(str_info, twr_cmwx1zzabz_get_fw_version(&lora), sizeof(str_info));

        lora_ready_params_udpate();
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_JOIN_SUCCESS)
    {
        twr_atci_printfln("$JOIN_OK");
        strcpy(str_status, "JOIN: OK");

        // RFQ + Frame counters chain
        twr_cmwx1zzabz_rfq(&lora);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_JOIN_ERROR)
    {
        twr_atci_printfln("$JOIN_ERROR");
        strcpy(str_status, "JOIN: ERR");
        clear_packet_info();
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_MESSAGE_RECEIVED)
    {
        /*
        lora_received++;
        char hex[3];
        uint8_t buffer[60];
        uint8_t len = twr_cmwx1zzabz_get_received_message_data(&lora, buffer, sizeof(buffer));
        m_lora_received_str[0] = '\0';
        for (int i = 0; i < len; i++)
        {
            snprintf(hex, sizeof(hex), "%02X", buffer[i]);
            strncat(m_lora_received_str, hex, sizeof(m_lora_received_str) - 1);
        }*/
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_RFQ)
    {
        twr_cmwx1zzabz_get_rfq(&lora, &rfq_rssi, &rfq_snr);

        twr_atci_printfln("$RSSI %d", rfq_rssi);
        twr_atci_printfln("$SNR: %d", rfq_snr);

        set_packet_info();

        twr_cmwx1zzabz_frame_counter(&lora);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_FRAME_COUNTER)
    {
        twr_cmwx1zzabz_get_frame_counter(&lora, &frame_counter_up, &frame_counter_down);
        twr_atci_printfln("$FRAME_COUNTER: %d,%d", frame_counter_up, frame_counter_down);

        set_packet_info();
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_LINK_CHECK_OK)
    {
        uint8_t margin;
        uint8_t gateway_count;

        twr_cmwx1zzabz_get_link_check(&lora, &margin, &gateway_count);

        twr_atci_printfln("$LINK_CHECK: 1");
        twr_atci_printfln("$MARGIN: %d", margin);
        twr_atci_printfln("$GWCOUNT: %d", gateway_count);

        snprintf(str_status, sizeof(str_status), "CHK:%d,%d", (int)margin, (int)gateway_count);

        // RFQ + Frame counters chain
        twr_cmwx1zzabz_rfq(&lora);

    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_LINK_CHECK_NOK)
    {
        twr_atci_printfln("$LINK_CHECK: 0");
        strcpy(str_status, "LNK: NOK");
        clear_packet_info();
    }
}

bool at_send(void)
{
    static uint8_t buffer[230];
    // temp, acc, GPS data
    uint8_t len = 0;

    int16_t temp = (int16_t)(temperature * 10.0f);
    buffer[len++] = temp;
    buffer[len++] = temp >> 8;

    int lat = gps_position.latitude * 1E5;
    buffer[len++] = lat;
    buffer[len++] = lat >> 8;
    buffer[len++] = lat >> 16;
    buffer[len++] = lat >> 24;

    int lon = gps_position.longitude * 1E5;
    buffer[len++] = lon;
    buffer[len++] = lon >> 8;
    buffer[len++] = lon >> 16;
    buffer[len++] = lon >> 24;

    int alt = gps_altitude.altitude;
    buffer[len++] = alt;
    buffer[len++] = alt >> 8;

    uint8_t satellites = gps_quality.satellites_tracked;
    buffer[len++] = satellites;

    twr_cmwx1zzabz_send_message_confirmed(&lora, buffer, len);

    return true;
}

bool at_status(void)
{
    twr_atci_printfln("$STATUS: OK");
    return true;
}

void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    if (event == TWR_TMP112_EVENT_UPDATE)
    {
        twr_tmp112_get_temperature_celsius(self, &temperature);
    }
}

void gps_module_event_handler(twr_module_gps_event_t event, void *event_param)
{
    if (event == TWR_MODULE_GPS_EVENT_START)
    {
        twr_led_set_mode(&gps_led_g, TWR_LED_MODE_ON);
    }
    else if (event == TWR_MODULE_GPS_EVENT_STOP)
    {
        twr_led_set_mode(&gps_led_g, TWR_LED_MODE_OFF);
    }
    else if (event == TWR_MODULE_GPS_EVENT_UPDATE)
    {
        twr_led_pulse(&gps_led_r, 50);

        if (twr_module_gps_get_time(&gps_time))
        {
        }

        if (twr_module_gps_get_position(&gps_position))
        {
        }

        if (twr_module_gps_get_altitude(&gps_altitude))
        {

        }

        if (twr_module_gps_get_quality(&gps_quality))
        {
        }

        //snprintf(m_lora_gps_info_str, sizeof(m_lora_gps_info_str), "%03.1f,%03.1f", gps_position.latitude, gps_position.longitude);

        twr_module_gps_invalidate();
    }
}

void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    (void) event_param;

    if (event == TWR_MODULE_BATTERY_EVENT_UPDATE)
    {
        if (twr_module_battery_get_voltage(&battery_voltage))
        {
            //snprintf(m_battery_str, sizeof(m_battery_str), "%02.1f V", voltage);
            twr_scheduler_plan_now(0);
            twr_atci_printfln("$BATT: %02.1f", battery_voltage);
        }
    }
}

void task_tx_periodic(void *param)
{
    (void) param;

    twr_led_pulse(&led, 500);

    at_send();

    if (task_tx_period_delay)
    {
        twr_scheduler_plan_current_relative(task_tx_period_delay);
    }
}

void application_init(void)
{
    // Initialize logging
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize AT command interface
    at_init(&led, &lora);
    static const twr_atci_command_t commands[] = {
            AT_LORA_COMMANDS,
            {"$SEND", at_send, NULL, NULL, NULL, "Immediately send packet"},
            {"$STATUS", at_status, NULL, NULL, NULL, "Show status"},
            AT_LED_COMMANDS,
            TWR_ATCI_COMMAND_CLAC,
            TWR_ATCI_COMMAND_HELP
    };
    twr_atci_init(commands, TWR_ATCI_COMMANDS_LENGTH(commands));

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_set_mode(&led, TWR_LED_MODE_ON);

    // Initialize battery
    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    twr_module_battery_set_update_interval(5 * 60 * 1000);

    // Initialize thermometer
    twr_tmp112_init(&tmp112, TWR_I2C_I2C0, 0x49);
    twr_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    twr_tmp112_set_update_interval(&tmp112, TEMPERATURE_MEASUREMENT_PERIOD_MS);

    twr_led_init_virtual(&gps_led_r, TWR_MODULE_GPS_LED_RED, twr_module_gps_get_led_driver(), 0);
    twr_led_init_virtual(&gps_led_g, TWR_MODULE_GPS_LED_GREEN, twr_module_gps_get_led_driver(), 0);

    // Init LCD
    twr_module_lcd_init();
    twr_module_lcd_set_font(&twr_font_ubuntu_13);
    twr_module_lcd_update();

    twr_module_lcd_set_event_handler(lcd_event_handler, NULL);
    twr_module_lcd_set_button_hold_time(300);
    twr_module_lcd_set_button_debounce_time(30);

    // Initialize lora module
    twr_cmwx1zzabz_init(&lora, TWR_UART_UART1);
    twr_cmwx1zzabz_set_event_handler(&lora, lora_callback, NULL);
    twr_cmwx1zzabz_set_class(&lora, TWR_CMWX1ZZABZ_CONFIG_CLASS_A);
    twr_cmwx1zzabz_set_debug(&lora, true);

    if (!twr_module_gps_init())
    {
        twr_log_error("APP: GPS Module initialization failed");
        //strncpy(m_lora_gps_info_str, "Not detected",sizeof(m_lora_gps_info_str));
        gps_present = false;
    }
    else
    {
        twr_module_gps_set_event_handler(gps_module_event_handler, NULL);
        twr_module_gps_start();
        gps_present = true;
        twr_atci_printfln("$GPS: \"START\"");
    }

}

void application_task(void)
{
    if (!twr_module_lcd_is_ready())
    {
        twr_scheduler_plan_current_relative(20);
        return;
    }

    twr_system_pll_enable();

    char gps_buffer[30];
    twr_module_lcd_clear();

    twr_module_lcd_set_font(&twr_font_ubuntu_13);

    if (gps_present)
    {
        snprintf(gps_buffer, sizeof(gps_buffer), "%02d-%02d-%02d, %02d:%02d:%02d", (gps_time.year > 2000) ? gps_time.year - 2000 : 0, gps_time.month, gps_time.day, gps_time.hours, gps_time.minutes, gps_time.seconds);
        twr_module_lcd_draw_string(0, 0, gps_buffer, 1);

        snprintf(gps_buffer, sizeof(gps_buffer), "Pos: %03.4f,%03.4f", gps_position.latitude, gps_position.longitude);
        twr_module_lcd_draw_string(0, 13, gps_buffer, 1);

        snprintf(gps_buffer, sizeof(gps_buffer),"Fix: %d Sats: %d", gps_quality.fix_quality, gps_quality.satellites_tracked);
        twr_module_lcd_draw_string(0, 26, gps_buffer, 1);
    }
    else
    {
        twr_module_lcd_draw_string(0, 0, "No GPS Module", 1);
    }

    char str_battery[8];
    snprintf(str_battery, sizeof(str_battery),"%.1fV", battery_voltage);
    twr_module_lcd_draw_string(100, 26, str_battery, 1);

    twr_module_lcd_set_font(&twr_font_ubuntu_24);
    uint32_t x = (128 / 2) - twr_gfx_calc_string_width(twr_module_lcd_get_gfx(), str_status) / 2;

    twr_module_lcd_draw_string(x, 45, str_status, 1);

    twr_module_lcd_set_font(&twr_font_ubuntu_13);

    twr_module_lcd_draw_string(0, 82, str_info, 1);

    // Buttons
    //twr_module_lcd_draw_string(50, 90, "((RESET))", 1);
    twr_module_lcd_draw_string(0, 100, "LNK CHK", 1);
    twr_module_lcd_draw_string(5, 113, "(JOIN)", 1);
    twr_module_lcd_draw_string(88, 100, "SEND", 1);
    //twr_module_lcd_draw_string(85, 113, "(AUTO)", 1);

    twr_module_lcd_update();

    twr_system_pll_disable();

    if (gps_present)
    {
        // If timeout
        if (!gps_sleep && (twr_tick_get() > (gps_tick + GPS_TIMEOUT_MS)))
        {
            twr_module_gps_stop();
            gps_sleep = true;
            twr_atci_printfln("$GPS: \"STOP\"");
        }
    }

    if (reset_flag)
    {
        twr_scheduler_plan_current_relative(100);
        if (twr_gpio_get_input(TWR_GPIO_BUTTON) == 0)
        {
            twr_system_reset();
        }
        return;
    }

    // Faster display redraw when GPS present & active
    if (gps_present && !gps_sleep)
    {
        twr_scheduler_plan_current_relative(1000);
        return;
    }

    // Otherwise update is dependent on button events which
    // triggerst this task automatically
    //twr_scheduler_plan_current_relative(10000);
}
