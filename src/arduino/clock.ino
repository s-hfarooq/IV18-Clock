#include <WiFi.h>
#include "time.h"

#define DOUT 19
#define DIN 7
#define LOAD 4
#define CLK 2

const char* ssid = "SSID";
const char* password = "PASSWORD";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -8 * 60 * 60;
const int daylightOffset_sec = 3600;

void setup(void) {
  Serial.begin(115200);  //Starts Serial Communication at Baud Rate 115200

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  pinMode(DOUT, OUTPUT);  //Sets pin 7 as Output
  pinMode(LOAD, OUTPUT);  //Sets pin 7 as Output
  pinMode(CLK, OUTPUT);   //Sets pin 7 as Output
  pinMode(DIN, INPUT);    //Sets pin 7 as Output
}

void set_val(byte val, byte* ret) {
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

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  // Serial.print("Day of week: ");
  // Serial.println(&timeinfo, "%A");
  // Serial.print("Month: ");
  // Serial.println(&timeinfo, "%B");
  // Serial.print("Day of Month: ");
  // Serial.println(&timeinfo, "%d");
  // Serial.print("Year: ");
  // Serial.println(&timeinfo, "%Y");
  // Serial.print("Hour: ");
  // Serial.println(&timeinfo, "%H");
  // Serial.print("Hour (12 hour format): ");
  // Serial.println(&timeinfo, "%I");
  // Serial.print("Minute: ");
  // Serial.println(&timeinfo, "%M");
  // Serial.print("Second: ");
  // Serial.println(&timeinfo, "%S");

  // Serial.println("Time variables");
  // char timeHour[3];
  // strftime(timeHour, 3, "%H", &timeinfo);
  // Serial.println(timeHour);
  // char timeWeekDay[10];
  // strftime(timeWeekDay, 10, "%A", &timeinfo);
  // Serial.println(timeWeekDay);
  // Serial.println();


  Serial.println(timeinfo.tm_sec);
  Serial.println(timeinfo.tm_min);
  Serial.println(timeinfo.tm_hour);

  byte sec[] = {timeinfo.tm_sec / 10, timeinfo.tm_sec % 10};
  byte min[] = {timeinfo.tm_min / 10, timeinfo.tm_min % 10};
  byte hour[] = {timeinfo.tm_hour / 10, timeinfo.tm_hour % 10};
}

void loop(void) {
  // boolean send_array[20];
  //                             A  B  C  D  E  F  G  *  1  2  3  4  5  6  7  8  9  x x  x
  // boolean send_array[8][20] = {{ 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 0
  //                              { 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 1
  //                              { 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // 2
  //                              { 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, // 3
  //                              { 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, // 4
  //                              { 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 }, // 5
  //                              { 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 }, // 6
  //                              { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 }  // 7
  //                             };
    boolean send_array[8][20] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 0
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 1
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 2
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 3
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 4
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 5
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 6
                               {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // 7
                              };
  int del = 0.01;
  int timebetween = 0.01;

  // printLocalTime();
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  byte vals[] = {0, 0, timeinfo.tm_hour / 10, timeinfo.tm_hour % 10, timeinfo.tm_min / 10, timeinfo.tm_min % 10, timeinfo.tm_sec / 10, timeinfo.tm_sec % 10};

  for(int i = 7; i >= 0; i--) {
      byte ret[2];
      set_val(vals[i], ret);

      for(int j = 1; j < 4; j++) {
        send_array[i][j - 1] = bitRead(ret[0], 3 - j);
      }
      for(int j = 3; j < 7; j++) {
        send_array[i][j] = bitRead(ret[1], 3 - (j - 3));
      }

      send_array[i][15 - i] = 1;
  }

  // Dots
  send_array[3][7] = 1;
  send_array[5][7] = 1;

  for (int j = 0; j < 8; j++) {
    for (int i = 19; i >= 0; i--) {
      digitalWrite(DOUT, send_array[j][i]);
      digitalWrite(CLK, HIGH);
      delay(del);
      digitalWrite(CLK, LOW);
    }

    digitalWrite(LOAD, HIGH);
    delay(del);
    digitalWrite(LOAD, LOW);

    delay(del);
  }
}

// A = 0
// B = 1
// C = 2
// D = 3
// E = 4
// F = 5
// G = 6
// DEC = 7

// 1 = 8
// 2 = 9
// 3 = 10
// 4 = 11
// 5 = 12
// 6 = 13
// 7 = 14
// 8 = 15
// 9 = 16
