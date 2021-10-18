#include <application.h>

#include "m2.h"
#include "at.h"

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

bool sleep_active = false;
bool sleep_gps = false;
bool sleep_class_c = false;

bool at_send(void);
bool at_status(void);

void sleep();
void wakeup();

void menu_main_callback(Menu *menu, MenuItem *item);
void menu_data_callback(Menu *menu, MenuItem *item);
void menu_period_callback(Menu *menu, MenuItem *item);
void menu_band_callback(Menu *menu, MenuItem *item);
void menu_datarate_callback(Menu *menu, MenuItem *item);

int msTick = 10;

char m_lora_send_str[16] = "...";
char m_lora_mode_str[8] = "ABP";
char m_lora_join_str[16] = "";
char m_lora_band_str[16] = "";
char m_lora_nwk_str[16] = "";
char m_lora_class_str[16] = "A";
char m_lora_received_str[16] = "";
char m_lora_datarate_str[16] = "";
char m_lora_tx_data_str[16] = "1B";
char m_lora_gps_info_str[16] = "??";
char m_lora_tx_period_str[16] = "off";
char m_battery_str[16] = "";

int lora_port = 2;
int lora_received = 0;
int lora_tx_packet_length = 1;
int app_state = 0;

int lora_packet_counter = 0;

twr_tick_t task_tx_period_delay = 0;
twr_scheduler_task_id_t task_tx_period_id;

Menu menu_tx_data;
Menu menu_tx_period;
Menu menu_band;
Menu menu_datarate;

// Items
MenuItem m_item_send = {{"Send"}, NULL, MENU_PARAMETER_IS_STRING, (void*)m_lora_send_str};
MenuItem m_item_tx_data = {{"TX Data"}, &menu_tx_data,  MENU_PARAMETER_IS_STRING, (void*)m_lora_tx_data_str};
MenuItem m_item_tx_period = {{"TX Period"}, &menu_tx_period,  MENU_PARAMETER_IS_STRING, (void*)&m_lora_tx_period_str};
MenuItem m_item_lora_mode = {{"Mode"}, NULL,  MENU_PARAMETER_IS_STRING, (void*)m_lora_mode_str};
MenuItem m_item_join = {{"Join"}, NULL,  MENU_PARAMETER_IS_STRING, (void*)m_lora_join_str};
MenuItem m_item_confirmed_chk = {{"Confirmation"}, NULL,  MENU_ITEM_IS_CHECKBOX , 0};
MenuItem m_item_port = {{"Port"}, NULL,  MENU_PARAMETER_IS_NUMBER , (void*)&lora_port};
MenuItem m_item_band = {{"Band"}, &menu_band,  MENU_PARAMETER_IS_STRING , (void*)&m_lora_band_str};
MenuItem m_item_datarate = {{"Datarate"}, &menu_datarate,  MENU_PARAMETER_IS_STRING , (void*)&m_lora_datarate_str};
MenuItem m_item_nwk = {{"Network"}, NULL,  MENU_PARAMETER_IS_STRING, (void*)&m_lora_nwk_str};
MenuItem m_item_class = {{"Class"}, NULL,  MENU_PARAMETER_IS_STRING, (void*)&m_lora_class_str};
MenuItem m_item_received = {{"Received"}, NULL,  MENU_PARAMETER_IS_NUMBER, (void*)&lora_received};
MenuItem m_item_rx_data = {{"RX"}, NULL, MENU_PARAMETER_IS_STRING, (void*)&m_lora_received_str};
MenuItem m_item_gps_info = {{"GPS Info"}, NULL, MENU_PARAMETER_IS_STRING, (void*)&m_lora_gps_info_str};
MenuItem m_item_battery = {{"Battery"}, NULL, MENU_PARAMETER_IS_STRING, (void*)&m_battery_str};
MenuItem m_item_sleep = {{"Sleep"}, NULL, NULL, NULL};

Menu menu_main = {
    {"HARDWARIO LoRa v1.0"},
    .items = {&m_item_send, &m_item_tx_data, &m_item_tx_period, &m_item_gps_info, &m_item_lora_mode, &m_item_join, &m_item_confirmed_chk, &m_item_port, &m_item_band, &m_item_datarate, &m_item_nwk, &m_item_class, &m_item_received, &m_item_rx_data, &m_item_battery, &m_item_sleep, 0},
    .refresh = 200,
    .callback = menu_main_callback
};

MenuItem m_band_as923 = {{"AS923"}, 0, 0, (void*)0}; // 0
MenuItem m_band_au915 = {{"AU915"}, 0, 0, (void*)1}; // 1
MenuItem m_band_eu868 = {{"EU868"}, 0, 0, (void*)5}; // 5
MenuItem m_band_kr920 = {{"KR920"}, 0, 0, (void*)6}; // 6
MenuItem m_band_in865 = {{"IN865"}, 0, 0, (void*)7}; // 7
MenuItem m_band_us915 = {{"US915"}, 0, 0, (void*)8}; // 8

Menu menu_band = {
    {"Band"},
    .items = {&m_band_as923, &m_band_au915, &m_band_eu868, &m_band_kr920, &m_band_in865, &m_band_us915, 0},
    .refresh = 200,
    .callback = menu_band_callback
};

MenuItem m_datarate_0 = {{"SF12/125kHz"}, 0, 0, (void*)0};
MenuItem m_datarate_1 = {{"SF11/125kHz"}, 0, 0, (void*)1};
MenuItem m_datarate_2 = {{"SF10/125kHz"}, 0, 0, (void*)2};
MenuItem m_datarate_3 = {{"SF9/125kHz"}, 0, 0, (void*)3};
MenuItem m_datarate_4 = {{"SF8/125kHz"}, 0, 0, (void*)4};
MenuItem m_datarate_5 = {{"SF7/125kHz"}, 0, 0, (void*)5};
MenuItem m_datarate_6 = {{"SF7/250kHz"}, 0, 0, (void*)6};
MenuItem m_datarate_7 = {{"FSK 50kbit"}, 0, 0, (void*)7};

Menu menu_datarate = {
    {"Datarate"},
    .items = {&m_datarate_0, &m_datarate_1, &m_datarate_2, &m_datarate_3, &m_datarate_4, &m_datarate_5, &m_datarate_6, &m_datarate_7, 0},
    .refresh = 200,
    .callback = menu_datarate_callback
};

MenuItem m_data_0 = {{"1 B"}, 0, 0, (void*)1};
MenuItem m_data_1 = {{"25 B"}, 0, 0, (void*)25};
MenuItem m_data_2 = {{"50 B"}, 0, 0, (void*)50};
MenuItem m_data_3 = {{"123 B"}, 0, 0, (void*)123};
MenuItem m_data_4 = {{"230 B"}, 0, 0, (void*)230};
MenuItem m_data_5 = {{"GPS,Temp"}, 0, 0, (void*)250};

Menu menu_tx_data = {
    {"TX data"},
    .items = {&m_data_0, &m_data_1, &m_data_2, &m_data_3, &m_data_4, &m_data_5, 0},
    .refresh = 200,
    .callback = menu_data_callback
};

MenuItem m_periodic_0 = {{"off"}, 0, 0, (void*)0};
MenuItem m_periodic_1 = {{"5 s"}, 0, 0, (void*)(5 * 1000)};
MenuItem m_periodic_2 = {{"10 s"}, 0, 0,(void*)(10 * 1000)};
MenuItem m_periodic_3 = {{"15 s"}, 0, 0, (void*)(15 * 1000)};
MenuItem m_periodic_4 = {{"30 s"}, 0, 0, (void*)(30 * 1000)};
MenuItem m_periodic_5 = {{"1 min"}, 0, 0, (void*)(60 * 1000)};
MenuItem m_periodic_6 = {{"5 min"}, 0, 0, (void*)(5 * 60 * 1000)};
MenuItem m_periodic_7 = {{"10 min"}, 0, 0, (void*)(10 * 60 * 1000)};
MenuItem m_periodic_8 = {{"20 min"}, 0, 0, (void*)(20 * 60 * 1000)};
MenuItem m_periodic_9 = {{"60 min"}, 0, 0, (void*)(60 * 60 * 1000)};

Menu menu_tx_period = {
    {"TX period"},
    .items = {&m_periodic_0, &m_periodic_1, &m_periodic_2, &m_periodic_3, &m_periodic_4, &m_periodic_5, &m_periodic_6, &m_periodic_7, &m_periodic_8, &m_periodic_9, 0},
    .refresh = 200,
    .callback = menu_period_callback
};

void lcdBufferString(char *str, int x, int y)
{
    twr_module_lcd_draw_string(x, y, str, 1);
}

void lcdBufferNumber(int number, int x, int y)
{
    char str[16];
    snprintf(str, sizeof(str), "%d", number);
    twr_module_lcd_draw_string(x, y, str, 1);
}

void menu_main_callback(Menu *menu, MenuItem *item)
{
    if (item == &m_item_send)
    {
        at_send();
    }

    else if (item == &m_item_lora_mode)
    {
        if (twr_cmwx1zzabz_get_mode(&lora) == TWR_CMWX1ZZABZ_CONFIG_MODE_ABP)
        {
            twr_cmwx1zzabz_set_mode(&lora, TWR_CMWX1ZZABZ_CONFIG_MODE_OTAA);
            strcpy(m_lora_mode_str, "OTAA");
        }
        else
        {
            twr_cmwx1zzabz_set_mode(&lora, TWR_CMWX1ZZABZ_CONFIG_MODE_ABP);
            strcpy(m_lora_mode_str, "ABP");
        }
    }

    else if (item == &m_item_join)
    {
        twr_cmwx1zzabz_join(&lora);
        strcpy(m_lora_join_str, "Joining...");
    }

    else if (item == &m_item_confirmed_chk)
    {
        lora_send_confimed_message = (item->flags & MENU_ITEM_IS_CHECKED) ? true : false;
    }

    else if (item == &m_item_port)
    {
        lora_port++;
        if (lora_port == 5)
        {
            lora_port = 0;
        }
        twr_cmwx1zzabz_set_port(&lora, lora_port);
    }

    else if (item == &m_item_confirmed_chk)
    {
        lora_send_confimed_message = (item->flags & MENU_ITEM_IS_CHECKED) ? true : false;
    }

    else if (item == &m_item_nwk)
    {
        uint8_t public = twr_cmwx1zzabz_get_nwk_public(&lora);
        twr_cmwx1zzabz_set_nwk_public(&lora, public ? 0 : 1);
    }

    else if (item == &m_item_class)
    {
        twr_cmwx1zzabz_config_class_t class = twr_cmwx1zzabz_get_class(&lora);
        twr_cmwx1zzabz_set_class(&lora, (class == TWR_CMWX1ZZABZ_CONFIG_CLASS_A) ? TWR_CMWX1ZZABZ_CONFIG_CLASS_C : TWR_CMWX1ZZABZ_CONFIG_CLASS_A);

        class = twr_cmwx1zzabz_get_class(&lora);
        sleep_class_c = class == TWR_CMWX1ZZABZ_CONFIG_CLASS_C;
    }

    else if (item == &m_item_received)
    {
        lora_received = 0;
    }

    else if (item == &m_item_gps_info)
    {
        app_state = 1;
    }

    else if (item == &m_item_sleep)
    {
        sleep();
    }
}

void menu_data_callback(Menu *menu, MenuItem *item)
{
    lora_tx_packet_length = (int)item->parameter;
    strncpy(m_lora_tx_data_str, item->text[0], sizeof(m_lora_tx_data_str));
}

void menu_period_callback(Menu *menu, MenuItem *item)
{
    task_tx_period_delay = (int)item->parameter;
    strncpy(m_lora_tx_period_str, item->text[0], sizeof(m_lora_tx_period_str));
    twr_scheduler_plan_relative(task_tx_period_id, task_tx_period_delay ? task_tx_period_delay : TWR_TICK_INFINITY);
}

void menu_band_callback(Menu *menu, MenuItem *item)
{
    int band = (int)item->parameter;
    strncpy(m_lora_band_str, item->text[0], sizeof(m_lora_band_str));
    twr_cmwx1zzabz_set_band(&lora, band);
}

void menu_datarate_callback(Menu *menu, MenuItem *item)
{
    int datarate = (int)item->parameter;
    strncpy(m_lora_datarate_str, item->text[0], sizeof(m_lora_datarate_str));
    twr_cmwx1zzabz_set_datarate(&lora, datarate);
}

void sleep()
{
    twr_cmwx1zzabz_set_class(&lora, TWR_CMWX1ZZABZ_CONFIG_CLASS_A);
    twr_module_gps_stop();
    twr_module_lcd_clear();
    twr_module_lcd_update();
    sleep_active = true;
    // Disable applicatin_task
    twr_scheduler_plan_absolute(0, TWR_TICK_INFINITY);
}

void wakeup()
{
    if (sleep_class_c)
    {
        twr_cmwx1zzabz_set_class(&lora, TWR_CMWX1ZZABZ_CONFIG_CLASS_C);
    }
    if (sleep_gps)
    {
        twr_module_gps_start();
    }
    app_state = 0;
    sleep_active = false;
    twr_scheduler_plan_now(0);
}

void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    (void) self;

    if (sleep_active)
    {
        if (self == &button_left && event == TWR_BUTTON_EVENT_HOLD)
        {
            wakeup();
        }
        else
        {
            return;
        }
    }

    twr_scheduler_plan_now(0);

    // Any event returns from GPS screen to menu
    if (app_state == 1 && (event == TWR_BUTTON_EVENT_CLICK || event == TWR_BUTTON_EVENT_HOLD))
    {
        app_state = 0;
        return;
    }

    if (app_state == 0)
    {
        if (self == &button_left && event == TWR_BUTTON_EVENT_CLICK)
        {
            menu2_event(&menu_main, BTN_UP);
        }
        if (self == &button_left && event == TWR_BUTTON_EVENT_HOLD)
        {
            menu2_event(&menu_main, BTN_LEFT);
        }
        else if (self == &button_right && event == TWR_BUTTON_EVENT_CLICK)
        {
            menu2_event(&menu_main, BTN_DOWN);
        }
        else if (self == &button_right && event == TWR_BUTTON_EVENT_HOLD)
        {
            menu2_event(&menu_main, BTN_ENTER);
        }
    }


}

void lora_ready_params_udpate()
{
    twr_cmwx1zzabz_config_band_t band = twr_cmwx1zzabz_get_band(&lora);
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

}



void lora_callback(twr_cmwx1zzabz_t *self, twr_cmwx1zzabz_event_t event, void *event_param)
{
    if (event == TWR_CMWX1ZZABZ_EVENT_ERROR)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_BLINK_FAST);
        strcpy(m_lora_send_str, "ERR");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_SEND_MESSAGE_START)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_ON);
        strcpy(m_lora_send_str, "SENDING..");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_SEND_MESSAGE_DONE)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_OFF);
        strcpy(m_lora_send_str, "SENT");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_MESSAGE_CONFIRMED)
    {
        strcpy(m_lora_send_str, "ACK");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_MESSAGE_NOT_CONFIRMED)
    {
        strcpy(m_lora_send_str, "NACK");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_READY)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_OFF);

        static bool ready_flag = false;

        if (!ready_flag)
        {
            strncpy(m_lora_send_str, "READY", sizeof(m_lora_send_str));
            ready_flag = true;
        }

        lora_ready_params_udpate();
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_JOIN_SUCCESS)
    {
        twr_atci_printf("$JOIN_OK");
        strcpy(m_lora_join_str, "JOINED");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_JOIN_ERROR)
    {
        twr_atci_printf("$JOIN_ERROR");
        strcpy(m_lora_join_str, "ERROR");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_MESSAGE_RECEIVED)
    {
        lora_received++;
        char hex[3];
        uint8_t buffer[60];
        uint8_t len = twr_cmwx1zzabz_get_received_message_data(&lora, buffer, sizeof(buffer));
        m_lora_received_str[0] = '\0';
        for (int i = 0; i < len; i++)
        {
            snprintf(hex, sizeof(hex), "%02X", buffer[i]);
            strncat(m_lora_received_str, hex, sizeof(m_lora_received_str) - 1);
        }
    }
}

bool at_send(void)
{
    static uint8_t buffer[230];

    int len = lora_tx_packet_length;

    if (len > 0 && len <= 230)
    {
        for (int i = 0; i < len; i++)
        {
            buffer[i] = lora_packet_counter;
        }
        lora_packet_counter++;
    }
    else if (len == 250)
    {
        // temp, acc, GPS data
        len = 0;

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
    }


    if (lora_send_confimed_message)
    {
        twr_cmwx1zzabz_send_message_confirmed(&lora, buffer, len);
    }
    else
    {
        twr_cmwx1zzabz_send_message(&lora, buffer, len);
    }


    return true;
}

bool at_status(void)
{
    twr_atci_printf("$STATUS: OK");
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

        snprintf(m_lora_gps_info_str, sizeof(m_lora_gps_info_str), "%03.1f,%03.1f", gps_position.latitude, gps_position.longitude);

        twr_module_gps_invalidate();
    }
}

void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    (void) event_param;

    float voltage;

    if (event == TWR_MODULE_BATTERY_EVENT_UPDATE)
    {
        if (twr_module_battery_get_voltage(&voltage))
        {
            snprintf(m_battery_str, sizeof(m_battery_str), "%02.1f V", voltage);
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

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_set_mode(&led, TWR_LED_MODE_ON);

    // Initialize button
    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, false);
    twr_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize battery
    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    twr_module_battery_set_update_interval(5 * 60 * 1000);

    // Initialize thermometer
    twr_tmp112_init(&tmp112, TWR_I2C_I2C0, 0x49);
    twr_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    twr_tmp112_set_update_interval(&tmp112, 10000);

    if (!twr_module_gps_init())
    {
        twr_log_error("APP: GPS Module initialization failed");
        strncpy(m_lora_gps_info_str, "Not detected",sizeof(m_lora_gps_info_str));
    }
    else
    {
        twr_module_gps_set_event_handler(gps_module_event_handler, NULL);
        twr_module_gps_start();
        sleep_gps = true;
    }

    twr_led_init_virtual(&gps_led_r, TWR_MODULE_GPS_LED_RED, twr_module_gps_get_led_driver(), 0);
    twr_led_init_virtual(&gps_led_g, TWR_MODULE_GPS_LED_GREEN, twr_module_gps_get_led_driver(), 0);

    // Init LCD
    twr_module_lcd_init();
    twr_module_lcd_set_font(&twr_font_ubuntu_13);
    twr_module_lcd_update();

    // Initialize lora module
    twr_cmwx1zzabz_init(&lora, TWR_UART_UART1);
    twr_cmwx1zzabz_set_event_handler(&lora, lora_callback, NULL);
    twr_cmwx1zzabz_set_class(&lora, TWR_CMWX1ZZABZ_CONFIG_CLASS_A);

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

    const twr_button_driver_t* lcdButtonDriver =  twr_module_lcd_get_button_driver();
    twr_button_init_virtual(&button_left, 0, lcdButtonDriver, 0);
    twr_button_init_virtual(&button_right, 1, lcdButtonDriver, 0);

    twr_button_set_event_handler(&button_left, button_event_handler, (int*)0);
    twr_button_set_event_handler(&button_right, button_event_handler, (int*)1);

    twr_button_set_hold_time(&button_left, 300);
    twr_button_set_hold_time(&button_right, 300);

    twr_button_set_debounce_time(&button_left, 30);
    twr_button_set_debounce_time(&button_left, 30);

    task_tx_period_id = twr_scheduler_register(task_tx_periodic, 0, TWR_TICK_INFINITY);

    menu2_init(&menu_main);
    menu2_init(&menu_tx_data);
    menu2_init(&menu_tx_period);
    menu2_init(&menu_band);
    menu2_init(&menu_datarate);
}

void application_task(void)
{
    if (!twr_module_lcd_is_ready())
    {
        twr_scheduler_plan_current_relative(20);
        return;
    }

    if(sleep_active)
    {
        return;
    }

    twr_system_pll_enable();

    switch (app_state)
    {
        case 0:
        {
            menu2_draw(&menu_main);
            break;
        }

        case 1:
        {
            char gps_buffer[30];
            twr_module_lcd_clear();

            snprintf(gps_buffer, sizeof(gps_buffer), "Date: %04d-%02d-%02d", gps_time.year, gps_time.month, gps_time.day);
            twr_module_lcd_draw_string(0, 0, gps_buffer, 1);

            snprintf(gps_buffer, sizeof(gps_buffer), "Time: %02d:%02d:%02d", gps_time.hours, gps_time.minutes, gps_time.seconds);
            twr_module_lcd_draw_string(0, 13, gps_buffer, 1);

            snprintf(gps_buffer, sizeof(gps_buffer), "Lat: %03.5f", gps_position.latitude);
            twr_module_lcd_draw_string(0, 26, gps_buffer, 1);

            snprintf(gps_buffer, sizeof(gps_buffer),"Lon: %03.5f", gps_position.longitude);
            twr_module_lcd_draw_string(0, 39, gps_buffer, 1);

            snprintf(gps_buffer, sizeof(gps_buffer),"Fix quality: %d", gps_quality.fix_quality);
            twr_module_lcd_draw_string(0, 52, gps_buffer, 1);

            snprintf(gps_buffer, sizeof(gps_buffer),"Satellites: %d", gps_quality.satellites_tracked);
            twr_module_lcd_draw_string(0, 65, gps_buffer, 1);

            twr_module_lcd_update();
            break;
        }

        default:
        {
            break;
        }
    }

    twr_system_pll_disable();

    twr_scheduler_plan_current_relative(200);

}
