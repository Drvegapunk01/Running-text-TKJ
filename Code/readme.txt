===========================================================================
  DOKUMENTASI BUG & SOLUSI — ESP32 Running Text P10 (DMD32)
  Tanggal   : 2025
  Platform  : ESP32 + 2x Panel P10 + Library DMD32
===========================================================================

---------------------------------------------------------------------------
DESKRIPSI SISTEM
---------------------------------------------------------------------------
Mikrokontroler  : ESP32
Jumlah Panel    : 2x P10 (64x16 pixel total)
Library Display : DMD32
Web Server      : ESP32 WebServer (port 80)
Konektivitas    : WiFi Access Point (AP Mode)
Penyimpanan     : EEPROM 512 bytes
Font            : Arial Black 16
Fungsi Utama    : Running text yang dapat diupdate via browser

---------------------------------------------------------------------------
GEJALA / CIRI MASALAH
---------------------------------------------------------------------------
1. Running text bekerja normal saat pertama kali menyala (teks dari EEPROM
   ditampilkan dan berjalan dengan baik).

2. Ketika teks diupdate melalui web browser:
   - Panel P10 BERKEDIP / FLICKER
   - Panel RESET sebentar (layar mati sesaat)
   - Teks LAMA masih tampil, teks baru TIDAK muncul

3. Browser web LOADING TERUS (tidak selesai / tidak kembali ke halaman).

---------------------------------------------------------------------------
ANALISIS AKAR MASALAH (ROOT CAUSE)
---------------------------------------------------------------------------

MASALAH #1 — Konflik SPI antara Web Handler dan Timer ISR
----------------------------------------------------------
  Lokasi    : fungsi handleUpdateText() dan updateDisplayText()
  Penyebab  : handleUpdateText() memanggil updateDisplayText() secara
              langsung dari dalam konteks web server handler.
              Sementara itu, timer ISR (triggerScan) terus berjalan dan
              memanggil dmd.scanDisplayBySPI() secara paralel.
              Dua operasi SPI berjalan bersamaan menyebabkan konflik bus,
              yang mengakibatkan panel berkedip dan layar reset.
  Dampak    : Panel flicker, display kacau, teks tidak berganti.

MASALAH #2 — Fungsi delay() di dalam Konteks Sensitif Timer
------------------------------------------------------------
  Lokasi    : fungsi updateDisplayText()
  Penyebab  : Terdapat pemanggilan delay(10) dan delay() lain di dalam
              updateDisplayText() yang dipanggil dari web handler.
              delay() memblokir eksekusi dan mengganggu stabilitas
              timer interrupt DMD.
  Dampak    : Memperpanjang konflik SPI, memperparah flicker.

MASALAH #3 — Browser Loading Terus / Tidak Selesai
---------------------------------------------------
  Lokasi    : fungsi handleUpdateText() — mekanisme HTTP response
  Penyebab  : Alur eksekusi di handleUpdateText():
                1. Terima request dari browser
                2. Panggil writeTextToEEPROM() — butuh waktu (delay 100ms)
                3. Panggil updateDisplayText() — konflik SPI, bisa hang
                4. Baru kirim redirect 302 ke browser
              Karena langkah 3 bisa hang atau memakan waktu lama akibat
              konflik SPI, HTTP response tidak pernah terkirim sempurna
              ke browser, sehingga browser menunggu terus.
              Ditambah redirect URL yang panjang (mengandung encoded text)
              menambah beban proses string.
  Dampak    : Browser stuck loading, user mengira sistem tidak merespons.

---------------------------------------------------------------------------
SOLUSI YANG DITERAPKAN
---------------------------------------------------------------------------

SOLUSI #1 — Gunakan Flag untuk Defer Update Display (Utama)
-----------------------------------------------------------
  Prinsip   : Jangan pernah menyentuh DMD dari dalam web handler.
              Web handler hanya SET FLAG dan menyimpan teks baru ke
              variabel sementara. Loop() yang akan memproses update
              di waktu yang aman (di luar konteks ISR).

  Perubahan :
    - Tambah variabel global:
        volatile bool textUpdated = false;
        String pendingText = "";

    - Di handleUpdateText():
        HAPUS pemanggilan updateDisplayText()
        GANTI dengan:
          pendingText = newText;
          textUpdated = true;

    - Di loop():
        Tambahkan blok di AWAL loop (sebelum scrollText()):
          if (textUpdated) {
            textUpdated = false;
            runningText = pendingText;
            xPos = DISPLAYS_ACROSS * 32;
            dmd.clearScreen(true);
            dmd.selectFont(Arial_Black_16);
          }

SOLUSI #2 — Hapus Semua delay() dari Fungsi Display
----------------------------------------------------
  Perubahan :
    - Hapus delay(10), delay(100) dari updateDisplayText()
    - Fungsi updateDisplayText() sendiri dihapus, logikanya
      dipindah ke dalam blok flag di loop()

SOLUSI #3 — Ganti Redirect 302 dengan Response Langsung + fetch() JS
---------------------------------------------------------------------
  Prinsip   : Kirim HTTP response SEGERA setelah menerima request,
              tanpa menunggu proses display selesai.
              Gunakan JavaScript fetch() di sisi browser agar halaman
              tidak reload / tidak masuk kondisi loading.

  Perubahan :
    - Di handleUpdateText():
        GANTI:
          server.sendHeader("Location", redirectUrl);
          server.send(302, "text/plain", "");
        DENGAN:
          server.send(200, "text/plain", "OK");

    - Di HTML (handleRoot()):
        HAPUS tag <form action="/update" method="POST">
        GANTI tombol submit dengan onclick="submitText()"
        TAMBAH fungsi JavaScript:
          function submitText() {
            fetch('/update', { method:'POST',
              body: new URLSearchParams({text}) })
            .then(() => { /* update UI tanpa reload */ })
          }

---------------------------------------------------------------------------
RINGKASAN PERUBAHAN (BEFORE vs AFTER)
---------------------------------------------------------------------------

  Aspek                  | Sebelum (Bermasalah)        | Sesudah (Diperbaiki)
  -----------------------+-----------------------------+-------------------------
  Update display         | Langsung dari web handler   | Via flag di loop()
  delay() di handler     | Ada (delay 10ms, 100ms)     | Dihapus semua
  HTTP response          | Redirect 302 (lambat)       | 200 OK langsung
  Browser behavior       | Form submit + reload        | fetch() tanpa reload
  Konflik SPI            | Ada (ISR vs handler)        | Tidak ada
  Panel flicker          | Ya                          | Tidak
  Teks berganti          | Tidak                       | Ya
  Browser loading terus  | Ya                          | Tidak

---------------------------------------------------------------------------
PELAJARAN / LESSON LEARNED
---------------------------------------------------------------------------

1. JANGAN memanggil fungsi SPI/display dari dalam web server handler
   ketika timer ISR berjalan secara paralel. Selalu gunakan flag dan
   defer eksekusi ke main loop().

2. JANGAN menggunakan delay() di dalam fungsi yang dipanggil dari
   konteks interrupt-sensitive. delay() memblokir CPU dan merusak
   timing ISR.

3. Untuk HTTP response di embedded web server, kirim response SEGERA.
   Proses berat (display, EEPROM) sebaiknya dilakukan SEBELUM atau
   SETELAH response dikirim — bukan di tengah-tengah.

4. Gunakan fetch() di sisi JavaScript browser untuk menghindari full
   page reload saat berkomunikasi dengan embedded web server. Ini
   lebih cepat dan lebih stabil pada koneksi WiFi AP yang terbatas.

5. volatile keyword wajib digunakan untuk variabel flag yang diakses
   bersama antara ISR dan main code agar compiler tidak mengoptimasi
   pembacaan variabel tersebut.

---------------------------------------------------------------------------
FILE TERKAIT
---------------------------------------------------------------------------
  - Sketch Arduino  : esp32_running_text_p10.ino
  - Library         : DMD32, WiFi.h, WebServer.h, EEPROM.h
  - Font file       : fonts/Arial_black_16.h

===========================================================================
  END OF DOCUMENT
===========================================================================
