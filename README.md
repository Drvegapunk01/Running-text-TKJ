# Running Text P10 with ESP32 
# NOTE: KODE INI HANYA AKAN BEKERJA DENGAN ESP32 VERSI 2.0.1 - 2.0.4 dikarenakan library DMD32 yang di gunakan tidak kompetible dengan versi lainnya
Project ini saya buat sebagai materi pembelajaran di jurusan TKJ dan Teknik Elektronika pada SMKN 2 Manokwari

## Deskripsi Alat

Project ini menggunakan:
- Panel P10 sebagai display running text
- ESP32 sebagai controller
- Power Supply 5V 45A untuk menyalakan alat

## Fitur

User dapat mengupdate teks running text melalui website dengan cara menghubungkan ke WiFi ESP32 yang dikonfigurasi sebagai Access Point (AP).

## Skematik

![Skematik Koneksi Panel P10 ke ESP32](https://github.com/user-attachments/assets/d35f1841-e8db-4d7a-a492-f5e718bcd66c)

Catatan: Konfigurasi di atas adalah standar dari panel P10 ke ESP32. Pin sudah dikonfigurasi melalui library, sehingga jika ingin memindahkan pin harus berurusan langsung dengan library.

## Penjelasan Kode

### WiFi Configuration

```cpp
const char* ap_ssid     = "ESP32-RunningText";
const char* ap_password = "12345678";

IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
ap_ssid : nama WiFi ESP32

ap_password : password WiFi

IPAddress : konfigurasi IP manual

cpp
WiFi.softAPConfig(local_IP, gateway, subnet);
WiFi.softAP(ap_ssid, ap_password);
softAPConfig() : set IP, gateway, subnet

softAP() : mengaktifkan mode hotspot

P10 (DMD32 LED Matrix)
cpp
#define DISPLAYS_ACROSS  2
#define DISPLAYS_DOWN    1

DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
DISPLAYS_ACROSS : jumlah panel horizontal

DISPLAYS_DOWN : jumlah panel vertikal

DMD dmd(...) : inisialisasi objek display

cpp
void IRAM_ATTR triggerScan() {
  dmd.scanDisplayBySPI();
}
triggerScan() : fungsi interrupt untuk refresh display

scanDisplayBySPI() : kirim data ke panel P10 via SPI

IRAM_ATTR : memastikan fungsi dijalankan cepat di interrupt

cpp
void initDMDTimer() {
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &triggerScan, true);
  timerAlarmWrite(timer, 2000, true);
  timerAlarmEnable(timer);
}
timerBegin(0, 80, true) : inisialisasi hardware timer (prescaler 80 -> 1 us/tick)

timerAttachInterrupt() : hubungkan timer dengan fungsi triggerScan

timerAlarmWrite(2000, true) : set interval interrupt (2000 us = 2 ms)

timerAlarmEnable() : aktifkan timer

cpp
dmd.drawString(xPos, 0, runningText.c_str(), runningText.length(), GRAPHICS_NORMAL);
xPos : posisi horizontal teks

0 : posisi vertikal (baris atas)

runningText.c_str() : konversi String ke char*

runningText.length() : panjang teks

GRAPHICS_NORMAL : mode tampilan normal
```

## EEPROMP (Electrically Erasable Programmable Read-Only Memory)

## WiFI (ESP32)

## WebServer (ESP32)
