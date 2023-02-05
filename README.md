# IV18-Clock

### Features
* 12/24hr clock (time fetched through sntp)
* Weather (fetched from api.open-meteo.com)
* Weather (in room based on sensor data)
* Switch through modes using button


### Parts
MAX6921, ESP32, IV-18 tube

### Pinout
| MAX6921     | Connection    |
| ----------- | ------------- |
| VCC         | 5v            |
| DIN         | ESP Pin D19   |
| OUT0        | IV-18 A       |
| OUT1        | IV-18 B       |
| OUT2        | IV-18 C       |
| OUT3        | IV-18 D       |
| OUT4        | IV-18 E       |
| OUT5        | IV-18 F       |
| OUT6        | IV-18 G       |
| OUT7        | IV-18 Decimal |
| OUT8        | IV-18 1       |
| OUT9        | IV-18 2       |
| LOAD        | ESP Pin D4    |
| CLK         | ESP Pin D2    |
| BLANK       | ESP Pin D19   |
| GND         | GND           |
| OUT10       | IV-18 3       |
| OUT11       | IV-18 4       |
| OUT12       | IV-18 5       |
| OUT13       | IV-18 6       |
| OUT14       | IV-18 7       |
| OUT15       | IV-18 8       |
| OUT16       | IV-18 9       |
| OUT17       | No connect    |
| OUT18       | No connect    |
| OUT19       | No connect    |
| DOUT        | No connect    |
| VBB         | 30v           |


https://github.com/Anacron-mb/esp32-DHT11
