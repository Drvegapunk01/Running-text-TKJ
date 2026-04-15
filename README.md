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

## WebServer (ESP32)

## DMD32
