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

#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"
#include <pthread.h>

// Variables to fetch weather info
#define WEB_SERVER "api.open-meteo.com"
#define WEB_PORT "80"
#define WEB_PATH "/v1/forecast?latitude=47.68&longitude=-122.21&current_weather=true"

static const char *TAG = "ESP-CLOCK";

// Pins used for MAX6921
#define DOUT 19
#define LOAD 4
#define CLK 2

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

#define TO_F(c) ((c * 9 / 5) + 32)

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

enum states {clock_24, clock_12, temp_room, temp_loc} curr_state = clock_12;
static volatile char tube_vals[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static volatile bool tube_dots[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static pthread_mutex_t tube_vals_lock;
static pthread_mutex_t tube_dots_lock;

static void obtain_time(void);

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv) {
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif

void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

// Determine which bits of 7 segment display to set high based on number trying to display
void segment_value_conversion(char val, unsigned char* ret) {
  switch (val) {
    case '0':
      ret[0] = 0x7;
      ret[1] = 0xE;
      break;
    case '1':
      ret[0] = 0x3;
      ret[1] = 0x0;
      break;
    case '2':
      ret[0] = 0x6;
      ret[1] = 0xD;
      break;
    case '3':
      ret[0] = 0x7;
      ret[1] = 0x9;
      break;
    case '4':
      ret[0] = 0x3;
      ret[1] = 0x3;
      break;
    case '5':
      ret[0] = 0x5;
      ret[1] = 0xB;
      break;
    case '6':
      ret[0] = 0x5;
      ret[1] = 0xF;
      break;
    case '7':
      ret[0] = 0x7;
      ret[1] = 0x0;
      break;
    case '8':
      ret[0] = 0x7;
      ret[1] = 0xF;
      break;
    case '9':
      ret[0] = 0x7;
      ret[1] = 0xB;
      break;
    case 'A':
      ret[0] = 0x7;
      ret[1] = 0x7;
      break;
    case 'B':
      ret[0] = 0x1; 
      ret[1] = 0xF; 
      break;
    case 'c':
      ret[0] = 0x4;
      ret[1] = 0xE;
      break;
    case 'C':
      ret[0] = 0x4;
      ret[1] = 0xE;
      break;
    case 'D':
      ret[0] = 0x3;
      ret[1] = 0xD;
      break;
    case 'E':
      ret[0] = 0x4;
      ret[1] = 0xF;
      break;
    case 'F':
      ret[0] = 0x4;
      ret[1] = 0x7;
      break;
    case 'G':
      ret[0] = 0x5;
      ret[1] = 0xE;
      break;
    case 'H':
      ret[0] = 0x3;
      ret[1] = 0x7;
      break;
    case 'h':
      ret[0] = 0x1;
      ret[1] = 0x7;
      break;
    case 'I':
      ret[0] = 0x0;
      ret[1] = 0x6;
      break;
    case 'J':
      ret[0] = 0x3;
      ret[1] = 0x8;
      break;
    case 'L':
      ret[0] = 0x0;
      ret[1] = 0xE;
      break;
    case 'N':
      ret[0] = 0x7;
      ret[1] = 0x6;
      break;
    case 'n':
      ret[0] = 0x1;
      ret[1] = 0x5;
      break;
    case 'o':
      ret[0] = 0x1;
      ret[1] = 0xD;
      break;
    case 'P':
      ret[0] = 0x6;
      ret[1] = 0x7;
      break;
    case 'r':
      ret[0] = 0x0;
      ret[1] = 0x5;
      break;
    case 't':
      ret[0] = 0x0;
      ret[1] = 0xF;
      break;
    case 'U':
      ret[0] = 0x3;
      ret[1] = 0xE;
      break;
    case 'u':
      ret[0] = 0x1;
      ret[1] = 0xC;
      break;
    case 'y':
      ret[0] = 0x3;
      ret[1] = 0xB;
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

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());
}

bool bitRead(unsigned char val, int idx) {
    val >>= idx;
    val &= 0x1;

    return val != 0;
}

static const char *REQUEST = "GET " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";

static void http_get_task(void *pvParameters) {
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    while(1) {
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.
           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        char full_arr[600];
        bzero(full_arr, sizeof(full_arr));
        int fai = 0;
        
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                full_arr[fai] = recv_buf[i];
                fai++;
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        char *tmp_char_arr;
        tmp_char_arr = strstr(full_arr, "temperature");
        int start_idx = tmp_char_arr - full_arr + 13;
        int end_idx = start_idx;
        char tmp_char = full_arr[end_idx];
        while(isdigit(tmp_char) || tmp_char == '.') {
            end_idx++;
            tmp_char = full_arr[end_idx];
        }

        char tmp_str[10];
        bzero(tmp_str, sizeof(tmp_str));
        int count = 0;
        while(count < end_idx - start_idx) {
            tmp_str[count] = full_arr[start_idx + count];
            count++;
        }

        float temp_c = (float)atof(tmp_str);
        float temp_f = TO_F(temp_c);
        ESP_LOGI(TAG, "TEMP IS: %s", tmp_str);
        ESP_LOGI(TAG, "TEMP IS: %f (%f)", temp_c, temp_f);

        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

void set_tube() {
    while(1) {
        int del = 1;

        //                         A  B  C  D  E  F  G  *  1  2  3  4  5  6  7  8  9  x  x  x
        bool send_array[8][20] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 0
                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 1
                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 2
                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 3
                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 4
                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 5
                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 6
                                  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // 7
                                 };

        // Convert int value to segment values
        for(int i = 7; i >= 0; i--) {
            unsigned char ret[2];
            segment_value_conversion(tube_vals[i], ret);

            for(int j = 1; j < 4; j++)
                send_array[i][j - 1] = bitRead(ret[0], 3 - j);
            for(int j = 3; j < 7; j++)
                send_array[i][j] = bitRead(ret[1], 3 - (j - 3));

            send_array[i][15 - i] = 1;
        }

        // Dots
        for(int i = 0; i < 8; i++)
            send_array[i][7] = tube_dots[i];

        // Send to MAX6921
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

    vTaskDelete(NULL);
}

void set_time(bool is_24) {
    time_t now;
    struct tm timeinfo;
    time(&now);

    setenv("TZ", "PST+8", 1);
    tzset();

    localtime_r(&now, &timeinfo);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

    // TODO - if is_24=false set 9th digit dot
    int hour = timeinfo.tm_hour;
    bool is_pm = false;
    if(!is_24) {
        is_pm = (hour >= 12);
        hour %= 12;

        if(hour == 0)
            hour = 12;
    }

    pthread_mutex_lock(&tube_vals_lock);
    tube_vals[0] = is_24 ? NULL : (is_pm ? 'P' : 'A');
    tube_vals[1] = NULL;
    tube_vals[2] = (hour / 10) + '0';
    tube_vals[3] = (hour % 10) + '0';
    tube_vals[4] = (timeinfo.tm_min / 10) + '0';
    tube_vals[5] = (timeinfo.tm_min % 10) + '0';
    tube_vals[6] = (timeinfo.tm_sec / 10) + '0';
    tube_vals[7] = (timeinfo.tm_sec % 10) + '0';
    pthread_mutex_unlock(&tube_vals_lock);
    pthread_mutex_lock(&tube_dots_lock);
    tube_dots[0] = 0;
    tube_dots[1] = 0;
    tube_dots[2] = 0;
    tube_dots[3] = 1;
    tube_dots[4] = 0;
    tube_dots[5] = 1;
    tube_dots[6] = 0;
    tube_dots[7] = 0;
    pthread_mutex_unlock(&tube_dots_lock);
}

void set_temp_loc() {

}

void set_temp_room() {

}

void set_error() {

}

void dispatcher() {
    while(1) {
        switch(curr_state) {
            case clock_24:
                set_time(true);
                break;
            case clock_12:
                set_time(false);
                break;
            case temp_room:
                set_temp_room();
                break;
            case temp_loc:
                set_temp_loc();
                break;
            default:
                set_error();
                break;
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
        // TODO: check if button pressed - if it is then move to next state
    }

    vTaskDelete(NULL);
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
    xTaskCreate(dispatcher, "dispatcher", 8 * 1024,
                        NULL, 10, NULL);
    xTaskCreatePinnedToCore(set_tube, "set_tube", 2 * 1024,
                        NULL, 1, NULL, 1);



    // http_get_task(NULL);

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
