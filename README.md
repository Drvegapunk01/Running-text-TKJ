# Running-text-TKJ
Project ini saya buat sebagai materi pembelajaran di jurusan TKJ dan Teknik elektronika pada SMKN 2 Manokwari


## DESKRIPSI ALAT
Di project ini saya menggunakan Panel P10 dan esp32 sebagai Controler nya, lalu menggunakan power supply 5v 45A untuk menyalakan alat tersebut

## FITUR ALAT
Fitur project ini adalah adalah user bisa memperbaruhi text melalui website hanya dengan menghubungkan ke ESP32 yang di atur sebagai AP 

## SKEMATIK
<img width="894" height="615" alt="connection" src="https://github.com/user-attachments/assets/d35f1841-e8db-4d7a-a492-f5e718bcd66c" />




Note: Ini adalah konfigurasi standar dri panel P10 Ke esp32, pin nya sudah di konfigurasi melalui library sehingga jika ingin memindahkan pinnya harus 
berurusan langsung dengan library








## PENJELASAN CODE 
### WiFI
const char* ap_ssid     = "ESP32-RunningText";
const char* ap_password = "12345678";

IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
- ap_ssid → nama WiFi ESP32
- ap_password → password WiFi
- IPAddress → konfigurasi IP manual

WiFi.softAPConfig(local_IP, gateway, subnet);
WiFi.softAP(ap_ssid, ap_password);

- softAPConfig() → set IP, gateway, subnet
- softAP() → mengaktifkan mode hotspot

### P10
#define DISPLAYS_ACROSS  2
#define DISPLAYS_DOWN    1

DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);
- Menentukan jumlah panel (2 horizontal, 1 vertikal)
void IRAM_ATTR triggerScan() {
  dmd.scanDisplayBySPI();
}
- Fungsi interrupt untuk refresh display
- IRAM_ATTR → agar cepat dieksekusi di interrupt
void initDMDTimer() {
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &triggerScan, true);
  timerAlarmWrite(timer, 2000, true);
  timerAlarmEnable(timer);
}
- timerBegin() → buat timer hardware
- timerAttachInterrupt() → hubungkan ke triggerScan
- timerAlarmWrite() → interval interrupt (≈2ms)
- timerAlarmEnable() → aktifkan timer
- dmd.drawString(xPos, 0, runningText.c_str(), runningText.length(), GRAPHICS_NORMAL);
- xPos → posisi horizontal
- 0 → posisi vertikal
- runningText.c_str() → string ke format C
- length() → panjang teks
- GRAPHICS_NORMAL → mode tampilan


### EEPROMP
### WebServer


