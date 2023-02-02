#define DOUT 8
#define DIN 7
#define LOAD 4
#define CLK 2

void setup (void) {
    Serial.begin(115200);                   //Starts Serial Communication at Baud Rate 115200 

    pinMode(DOUT, OUTPUT);                    //Sets pin 7 as Output
    pinMode(LOAD, OUTPUT);                    //Sets pin 7 as Output
    pinMode(CLK, OUTPUT);                    //Sets pin 7 as Output
    pinMode(DIN, INPUT);                    //Sets pin 7 as Output
}

void set_val(byte val, byte *ret) {
  switch(val) {
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

void loop(void) {
  // boolean send_array[20];
  boolean send_array[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int del = 5;
  int timebetween = 100;


  for(byte a = 0x0; a <= 0xF; a++) {
    for(int j = 0; j < 9; j++) {
      Serial.println(millis());
      byte ret[2];
      set_val(a, ret);

      Serial.println(ret[0]);
      Serial.println(ret[1]);

      for(int i = 1; i < 4; i++) {
        send_array[i - 1] = bitRead(ret[0], 3 - i);
        Serial.print(bitRead(ret[0], 3 - i));
      }
      Serial.println("DONE");
      for(int i = 3; i < 7; i++) {
        send_array[i] = bitRead(ret[1], 3 - (i - 3));
        Serial.print(bitRead(ret[1], 3 - (i - 3)));
      }
      Serial.println("DONE2");

      for(int i = 8; i < 17; i++) {
        send_array[i] = 0;
      }
      for(int i = 8; i < 8 + j; i++) {
        send_array[i] = 1;
      }

      for(int i = 0; i < 20; i++) {
        Serial.print(send_array[i]);
      }
      Serial.println();

      digitalWrite(CLK, LOW);
      delay(del);

      for(int i = 19; i >= 0; i--) {
        digitalWrite(DOUT, send_array[i]);
        digitalWrite(CLK, HIGH);
        delay(del);
        digitalWrite(CLK, LOW);
      }

      digitalWrite(LOAD, HIGH);
      delay(del);
      digitalWrite(LOAD, LOW);

      delay(timebetween);
    }
  }
  delay(timebetween);
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




