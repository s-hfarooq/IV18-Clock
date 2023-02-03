/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/gpio.h"

static const char *TAG = "ESP-CLOCK";

#define DOUT 19
#define DIN 7
#define LOAD 4
#define CLK 2
#define TESTPIN 18

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

static void obtain_time(void);

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv)
{
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

void set_val(unsigned char val, unsigned char* ret) {
  switch (val) {
    case 0x0:
      ret[0] = 0x7;
      ret[1] = 0xE;
      break;
    case 0x1:
      ret[0] = 0x3;
      ret[1] = 0x0;
      break;
    case 0x2:
      ret[0] = 0x6;
      ret[1] = 0xD;
      break;
    case 0x3:
      ret[0] = 0x7;
      ret[1] = 0x9;
      break;
    case 0x4:
      ret[0] = 0x3;
      ret[1] = 0x3;
      break;
    case 0x5:
      ret[0] = 0x5;
      ret[1] = 0xB;
      break;
    case 0x6:
      ret[0] = 0x5;
      ret[1] = 0xF;
      break;
    case 0x7:
      ret[0] = 0x7;
      ret[1] = 0x0;
      break;
    case 0x8:
      ret[0] = 0x7;
      ret[1] = 0xF;
      break;
    case 0x9:
      ret[0] = 0x7;
      ret[1] = 0xB;
      break;
    case 0xA:
      ret[0] = 0x7;
      ret[1] = 0x7;
      break;
    case 0xB:
      ret[0] = 0x1;
      ret[1] = 0xF;
      break;
    case 0xC:
      ret[0] = 0x4;
      ret[1] = 0xE;
      break;
    case 0xD:
      ret[0] = 0x3;
      ret[1] = 0xD;
      break;
    case 0xE:
      ret[0] = 0x4;
      ret[1] = 0xF;
      break;
    case 0xF:
      ret[0] = 0x4;
      ret[1] = 0x7;
      break;
    default:
      ret[0] = 0x0;
      ret[1] = 0x0;
      break;
  }
}

void tube_init(void) {
    gpio_reset_pin(DOUT);
    gpio_set_direction(DOUT, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LOAD);
    gpio_set_direction(LOAD, GPIO_MODE_OUTPUT);
    gpio_reset_pin(CLK);
    gpio_set_direction(CLK, GPIO_MODE_OUTPUT);
    
    gpio_reset_pin(TESTPIN);
    gpio_set_direction(TESTPIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TESTPIN, 1);
}

bool bitRead(unsigned char val, int idx) {
    val >>= idx;
    val &= 0x1;

    return val != 0;
}

void set_tube_time(void) {
    int del = 0.01;

    time_t now;
    struct tm timeinfo;
    time(&now);

    setenv("TZ", "PST+8", 1);
    tzset();

    localtime_r(&now, &timeinfo);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

    //                            A  B  C  D  E  F  G  *  1  2  3  4  5  6  7  8  9  x  x  x
    bool send_array[8][20] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 0
                                 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 1
                                 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 2
                                 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 3
                                 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 4
                                 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 5
                                 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 6
                                 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // 7
                                };

    unsigned char vals[8] = {0, 0, timeinfo.tm_hour / 10, timeinfo.tm_hour % 10, timeinfo.tm_min / 10, timeinfo.tm_min % 10, timeinfo.tm_sec / 10, timeinfo.tm_sec % 10};

    //  Thu Feb  2 21:52:49 2023

    for(int i = 7; i >= 0; i--) {
        unsigned char ret[2];
        set_val(vals[i], ret);

        for(int j = 1; j < 4; j++)
            send_array[i][j - 1] = bitRead(ret[0], 3 - j);
        for(int j = 3; j < 7; j++)
            send_array[i][j] = bitRead(ret[1], 3 - (j - 3));

        send_array[i][15 - i] = 1;
    }

    // Dots
    send_array[3][7] = 1;
    send_array[5][7] = 1;

    for (int j = 0; j < 8; j++) {
        for (int i = 19; i >= 0; i--) {
            gpio_set_level(DOUT, send_array[j][i]);
            gpio_set_level(CLK, 1);
            vTaskDelay(del / portTICK_PERIOD_MS);
            gpio_set_level(CLK, 0);
        }

        gpio_set_level(LOAD, 1);
        vTaskDelay(del / portTICK_PERIOD_MS);
        gpio_set_level(LOAD, 0);

        vTaskDelay(del / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    ++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }

    char strftime_buf[64];

    // Set timezone to Eastern Standard Time and print local time
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in New York is: %s", strftime_buf);

    // Set timezone to China Standard Time
    setenv("TZ", "PST+8", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Seattle is: %s", strftime_buf);

    tube_init();
    set_tube_time();

    if (sntp_get_sync_mode() == SNTP_SYNC_MODE_SMOOTH) {
        struct timeval outdelta;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_IN_PROGRESS) {
            adjtime(NULL, &outdelta);
            ESP_LOGI(TAG, "Waiting for adjusting time ... outdelta = %jd sec: %li ms: %li us",
                        (intmax_t)outdelta.tv_sec,
                        outdelta.tv_usec/1000,
                        outdelta.tv_usec%1000);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
    }

    const int deep_sleep_sec = 10;
    ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
    esp_deep_sleep(1000000LL * deep_sleep_sec);
}

static void print_servers(void) {
    ESP_LOGI(TAG, "List of configured NTP servers:");

    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i){
        if (esp_sntp_getservername(i)){
            ESP_LOGI(TAG, "server %d: %s", i, esp_sntp_getservername(i));
        } else {
            // we have either IPv4 or IPv6 address, let's print it
            char buff[INET6_ADDRSTRLEN];
            ip_addr_t const *ip = esp_sntp_getserver(i);
            if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
                ESP_LOGI(TAG, "server %d: %s", i, buff);
        }
    }
}

static void obtain_time(void) {
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default() );

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());


    ESP_LOGI(TAG, "Initializing and starting SNTP");
#if CONFIG_LWIP_SNTP_MAX_SERVERS > 1
    /* This demonstrates configuring more than one server
     */
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2,
                               ESP_SNTP_SERVER_LIST(CONFIG_SNTP_TIME_SERVER, "pool.ntp.org" ) );
#else
    /*
     * This is the basic default config with one server and starting the service
     */
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);
#endif
    config.sync_cb = time_sync_notification_cb;     // Note: This is only needed if we want
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    config.smooth_sync = true;
#endif

    esp_netif_sntp_init(&config);


    print_servers();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_ERROR_CHECK( example_disconnect() );
    esp_netif_sntp_deinit();
}
