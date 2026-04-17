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

# Penjelasan Library yang digunakan 



## EEPROMP (Electrically Erasable Programmable Read-Only Memory)
Pada Arduino khususnya di ESP32, EEPROM digunakan untuk menyimpan data yang tetap tersimpan walaupun perangkat dimatikan (non-volatile). Namun penting dipahami, pada ESP32 EEPROM bukan memori fisik terpisah, melainkan emulasi di dalam flash memory. Karena itu, cara kerjanya sedikit berbeda dibanding EEPROM asli, terutama harus menggunakan proses commit agar data benar-benar tersimpan.

## Fungsi dan Parameter EEPROM
### EEPROM.begin(size)
Fungsi ini digunakan untuk menginisialisasi EEPROM sebelum digunakan. Parameter size menentukan jumlah memori (dalam byte) yang ingin dialokasikan, misalnya 512 atau 1024. Fungsi ini wajib dipanggil di awal (biasanya di setup()), karena tanpa ini semua fungsi EEPROM tidak akan bekerja.

### EEPROM.read(address)
Fungsi ini digunakan untuk membaca 1 byte data dari EEPROM. Parameter address adalah alamat memori (dimulai dari 0). Nilai yang dikembalikan berupa angka 0–255 (tipe byte). Cocok untuk data sederhana seperti karakter atau nilai kecil.

### EEPROM.write(address, value)
Fungsi ini digunakan untuk menulis 1 byte data ke EEPROM. Parameter address adalah lokasi penyimpanan, dan value adalah data yang ingin disimpan (0–255). Fungsi ini hanya menulis ke buffer, belum menyimpan permanen.

### EEPROM.get(address, variable)
Fungsi ini digunakan untuk membaca data dengan tipe lebih kompleks (misalnya int, float, atau struct). Parameter address adalah lokasi awal data, dan variable adalah variabel tujuan. Fungsi ini akan langsung mengisi variabel tersebut sesuai tipe datanya.

### EEPROM.put(address, data)
Fungsi ini digunakan untuk menyimpan data kompleks ke EEPROM. Parameter address adalah lokasi awal, dan data adalah variabel yang ingin disimpan. Kelebihannya dibanding write adalah lebih efisien karena hanya menulis jika data berubah.

### EEPROM.commit()
Fungsi ini digunakan untuk menyimpan semua perubahan ke flash memory secara permanen. Tidak memiliki parameter. Ini adalah fungsi paling penting di ESP32—tanpa ini, data yang ditulis tidak akan benar-benar tersimpan.

### EEPROM.end()
Fungsi ini digunakan untuk mengakhiri penggunaan EEPROM dan membebaskan resource. Tidak memiliki parameter. Biasanya digunakan pada sistem yang lebih kompleks, tapi sering tidak wajib di project sederhana.

### EEPROM.length()
Fungsi ini digunakan untuk mengetahui ukuran EEPROM yang telah dialokasikan. Tidak memiliki parameter dan mengembalikan jumlah byte sesuai dengan nilai saat begin() dipanggil.

## WiFI (ESP32)

# Arduino WiFi Library

Source:
**Author:** Arduino `<info@arduino.cc>`  
**Repository:** [Arduino WiFi Reference](https://www.arduino.cc/en/Reference/WiFi)

## Pengertian Library WiFi Arduino

Library WiFi Arduino adalah sebuah kumpulan kode (library) yang memungkinkan board Arduino untuk terhubung ke jaringan internet melalui WiFi Shield. Dengan library ini, board Arduino dapat berperan sebagai server yang menerima koneksi masuk maupun sebagai client yang melakukan koneksi keluar ke server lain. Library ini mendukung pengiriman dan penerimaan paket UDP, serta dapat mengelola DNS (Domain Name System).

## Fungsi dan Tujuan

Library ini dibuat untuk memberikan kemampuan konektivitas nirkabel pada proyek-proyek Arduino. Dengan menggunakan WiFi Shield dan library ini, Anda dapat membuat berbagai aplikasi seperti:

- Mengirim data sensor ke server atau cloud
- Mengontrol perangkat dari jarak jauh melalui web
- Membuat web server sederhana yang dapat diakses melalui jaringan lokal
- Mengambil data dari API (Application Programming Interface) internet
- Berkomunikasi antar perangkat melalui jaringan WiFi

## Hal yang Perlu Diperhatikan Sebelum Menggunakan

### Kompatibilitas Shield
Library ini dirancang khusus untuk **Arduino WiFi Shield**. Jika menggunakan shield lain, kompatibilitas tidak dijamin.

### Firmware Update
Firmware pada WiFi shield perlu diperbarui jika menggunakan Arduino IDE versi 1.0.5 ke atas. Pastikan untuk mengikuti panduan update yang tersedia agar shield berfungsi dengan baik.

### Enkripsi yang Didukung
Library ini mendukung jenis enkripsi berikut:
- **WEP** (Wired Equivalent Privacy) - dengan key index dan hexadecimal key
- **WPA2 Personal** - menggunakan password berupa string

**Tidak mendukung:** WPA2 Enterprise

### Keterbatasan Jaringan
Jika SSID (Service Set Identifier) atau nama jaringan tidak dipublikasikan (hidden SSID), maka shield **tidak dapat** terhubung ke jaringan tersebut.

### Konfigurasi Pin

Arduino berkomunikasi dengan WiFi Shield melalui bus SPI (Serial Peripheral Interface):

| Board | Pin SPI (MOSI, MISO, SCK) | Pin SS (Slave Select) | Pin Khusus |
|-------|---------------------------|----------------------|------------|
| Arduino Uno | 11, 12, 13 | 10 | Pin 7 untuk handshake (jangan digunakan) |
| Arduino Mega | 50, 51, 52 | 10 | Pin 53 (SS hardware) harus dijaga sebagai OUTPUT, pin 7 untuk handshake |

## Contoh Program Sederhana

Berikut adalah contoh program minimal untuk menghubungkan Arduino ke jaringan WiFi:

```cpp
#include <WiFi.h>

// Masukkan SSID dan password jaringan Anda
char ssid[] = "namaJaringanAnda";
char pass[] = "passwordJaringanAnda";

void setup() {
  Serial.begin(9600);
  
  // Mulai koneksi ke jaringan
  WiFi.begin(ssid, pass);
  
  // Tunggu hingga terhubung
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Menghubungkan ke WiFi...");
  }
  
  Serial.println("Terhubung ke WiFi!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Kode Anda di sini
}

## WebServer (ESP32)

## DMD32
