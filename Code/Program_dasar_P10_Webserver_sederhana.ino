/* ===============================================================
   PROGRAM RUNNING TEXT P10 DENGAN ESP32
   ===============================================================
   FUNGSI UMUM:
   - ESP32 membuat WiFi sendiri (Access Point) dengan nama "ESP32-RunningText"
   - Kita bisa connect ke WiFi itu dari HP/Laptop
   - Buka browser ketik 192.168.1.1 => muncul halaman web
   - Kita ketik teks apapun, klik simpan => teks berjalan di panel P10
   - Teks tersimpan otomatis meskipun listrik mati
   =============================================================== */

#include <WiFi.h>          // Library untuk koneksi WiFi (mode AP)
#include <WebServer.h>     // Library untuk membuat web server
#include <EEPROM.h>        /* EEPROM = memori internal ESP32 yang tidak hilang saat mati listrik
                              Ibaratnya seperti "flashdisk mini" di dalam chip.
                              Digunakan untuk menyimpan teks agar saat listrik nyala lagi,
                              teks terakhir tetap muncul. */
#include <DMD32.h>         // Library untuk mengontrol modul P10 (LED running text)
#include "fonts/Arial_black_16.h"  // File font untuk teks di P10
#include "esp_task_wdt.h"  // Library untuk watchdog timer (pengawas sistem, dimatikan biar tidak reset)

// ========== KONFIGURASI WiFi ==========
/* ACCESS POINT (AP): ESP32 bertindak seperti router WiFi mini.
   Kita connect ke WiFi ini dari HP, tanpa perlu internet.
   Nama WiFi: ESP32-RunningText
   Password: 12345678 */
const char* ap_ssid     = "ESP32-RunningText";
const char* ap_password = "12345678";

/* Alamat IP: Setiap perangkat di jaringan punya alamat unik.
   192.168.1.1 adalah alamat ESP32 itu sendiri.
   Ketik ini di browser untuk mengakses halaman setting. */
IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Ukuran memori EEPROM (512 byte) dan batas panjang teks (200 karakter)
#define EEPROM_SIZE      512
#define MAX_TEXT_LENGTH  200

// Konfigurasi jumlah modul P10: 2 modul ke samping, 1 modul ke bawah
#define DISPLAYS_ACROSS  2
#define DISPLAYS_DOWN    1

// Membuat objek DMD untuk mengontrol panel P10
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

// Membuat objek web server di port 80 (port standar HTTP)
WebServer server(80);

// Variabel untuk timer (pemicu scan display)
hw_timer_t *timer = NULL;

// Variabel untuk teks yang berjalan
String runningText = "";
int    xPos        = DISPLAYS_ACROSS * 32;  // Posisi X awal teks (lebar total = 2x32 = 64 pixel)

// Variabel untuk mengatur waktu scroll dan proses web
unsigned long lastScrollTime = 0;
unsigned long lastWebTime    = 0;

// Flag dan variabel untuk update teks secara aman (dari interrupt)
volatile bool pendingUpdate = false;
String        pendingText   = "";

// =====================================================================
// TIMER - untuk refresh display P10 (scan display)
// =====================================================================
/* TIMER: Ibarat jam alarm internal yang memanggil fungsi secara rutin.
   Di sini dipanggil setiap 2ms untuk menyegarkan tampilan P10.
   Tanpa timer ini, layar akan berkedip atau mati. */
void IRAM_ATTR triggerScan() {
  dmd.scanDisplayBySPI();  // Perintah refresh layar P10
}

void timerPause() {
  timerAlarmDisable(timer);  // Hentikan timer sementara (saat simpan data)
}

void timerResume() {
  timerAlarmEnable(timer);   // Hidupkan timer lagi
}

void initDMDTimer() {
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &triggerScan, true);
  timerAlarmWrite(timer, 2000, true);  // 2000 mikrodetik = 2ms
  timerAlarmEnable(timer);
}

// =====================================================================
// WATCHDOG - dimatikan biar tidak reset sendiri
// =====================================================================
/* WATCHDOG: Fitur keamanan yang me-reset ESP jika program macet.
   Karena program ini aman, watchdog dimatikan agar tidak mengganggu. */
void disableWatchdog() {
  esp_task_wdt_deinit();
  Serial.println("Watchdog disabled");
}

// =====================================================================
// EEPROM - MENYIMPAN TEKS AGAR TIDAK HILANG SAAT MATI LISTRIK
// =====================================================================
/* EEPROM = Electrically Erasable Programmable Read-Only Memory.
   Sederhananya: memori kecil (512 byte) yang isinya tetap ada meskipun
   listrik dicabut. Ini seperti menyimpan catatan di dalam chip ESP32.
   
   CARA KERJA:
   - Alamat 0: menyimpan PANJANG teks (contoh: 15)
   - Alamat 1-200: menyimpan HURUF teks satu per satu
   
   Saat listrik mati, teks tetap aman. Saat nyala lagi, teks terbaca otomatis. */

String readTextFromEEPROM() {
  int len = EEPROM.read(0);           // Baca panjang teks dari alamat 0
  if (len <= 0 || len > MAX_TEXT_LENGTH)
    return "SELAMAT DATANG";          // Jika kosong/error, pakai teks default

  String text = "";
  for (int i = 0; i < len; i++) {
    char c = EEPROM.read(i + 1);      // Baca per karakter dari alamat 1,2,3,...
    if (c == 0 || c == 255) break;
    text += c;
  }
  return text;
}

void writeTextToEEPROM(const String& text) {
  timerPause();                       // Hentikan timer dulu biar tidak ganggu
  delayMicroseconds(3000);

  // Kosongkan area EEPROM untuk teks
  for (int i = 0; i < MAX_TEXT_LENGTH + 2; i++) EEPROM.write(i, 0);
  EEPROM.write(0, text.length());     // Simpan panjang teks
  for (int i = 0; i < (int)text.length(); i++) EEPROM.write(i + 1, text[i]);
  EEPROM.commit();                    // Perintah "simpan" ke memori fisik

  timerResume();                      // Hidupkan timer lagi
  Serial.println("EEPROM saved: " + text);
}

// =====================================================================
// SCROLL - MENGGERAKKAN TEKS DI LAYAR
// =====================================================================
/* Cara kerja scroll:
   - Teks digambar di posisi X tertentu
   - Setiap 50ms, posisi X dikurangi 1 (geser kiri)
   - Jika teks sudah lewat dari sisi kiri, reset ke kanan lagi
   - Ini menciptakan efek "berjalan terus" */
void scrollText() {
  if (millis() - lastScrollTime < 50) return;  // Batasi kecepatan
  lastScrollTime = millis();

  dmd.clearScreen(true);
  dmd.selectFont(Arial_Black_16);

  if (runningText.length() > 0) {
    int textWidth    = runningText.length() * 9;   // Perkiraan lebar teks
    int displayWidth = DISPLAYS_ACROSS * 32;

    dmd.drawString(xPos, 0, runningText.c_str(), runningText.length(), GRAPHICS_NORMAL);

    xPos--;
    if (xPos < -textWidth) xPos = displayWidth;  // Reset ke kanan
  }
}

// =====================================================================
// SETUP - INISIALISASI AWAL (dijalankan sekali saat power on)
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  disableWatchdog();

  EEPROM.begin(EEPROM_SIZE);   // Aktifkan EEPROM dengan ukuran 512 byte

  // === MEMBUAT WiFi SENDIRI (ACCESS POINT) ===
  /* Ini seperti menyalakan "router mini" dari ESP32.
     HP/Laptop bisa melihat WiFi bernama "ESP32-RunningText"
     dan connect dengan password "12345678".
     Setelah connect, buka browser dan ketik 192.168.1.1 */
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  // Inisialisasi timer dan DMD
  initDMDTimer();
  dmd.clearScreen(true);
  dmd.selectFont(Arial_Black_16);

  // Baca teks dari EEPROM (memori yang tidak hilang)
  runningText = readTextFromEEPROM();
  xPos = DISPLAYS_ACROSS * 32;
  Serial.println("Loaded: " + runningText);

  // === SETUP HALAMAN WEB ===
  server.on("/", handleRoot);                      // Halaman utama
  server.on("/update", HTTP_POST, handleUpdateText); // Tempat kirim teks baru
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();

  Serial.println("Ready: http://192.168.1.1");
  lastScrollTime = millis();
  lastWebTime    = millis();
}

// =====================================================================
// LOOP - PROGRAM UTAMA (dijalankan terus menerus)
// =====================================================================
void loop() {
  // Jika ada update teks dari web
  if (pendingUpdate) {
    pendingUpdate = false;
    runningText = pendingText;
    xPos = DISPLAYS_ACROSS * 32;
    writeTextToEEPROM(runningText);   // Simpan agar tidak hilang
    dmd.clearScreen(true);
    dmd.selectFont(Arial_Black_16);
    Serial.println("Updated: " + runningText);
  }

  // Proses permintaan web setiap 10ms
  if (millis() - lastWebTime >= 10) {
    lastWebTime = millis();
    server.handleClient();
  }

  scrollText();   // Jalankan scrolling teks
}

// =====================================================================
// WEB HANDLER - MEMBUAT HALAMAN HTML
// =====================================================================
/* Halaman web ini yang muncul saat kita buka 192.168.1.1
   Isinya: form untuk mengisi teks, tombol simpan, dll. */
void handleRoot() {
  String html = F("<!DOCTYPE html><html><head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Running Text P10</title>"
    "<style>"
    "*{margin:0;padding:0;box-sizing:border-box}"
    "body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#1a1a2e,#16213e);min-height:100vh;padding:20px}"
    ".card{max-width:480px;margin:0 auto;background:#fff;border-radius:20px;padding:28px;box-shadow:0 20px 40px rgba(0,0,0,.3)}"
    "h1{text-align:center;color:#16213e;margin-bottom:6px}"
    ".sub{text-align:center;color:#888;font-size:13px;margin-bottom:24px}"
    ".cur{background:#f0f8ff;border-left:4px solid #3498db;border-radius:8px;padding:14px;margin-bottom:20px}"
    ".cur b{display:block;margin-bottom:8px;color:#2c3e50}"
    "#curTxt{font-family:monospace;background:#fff;padding:8px;border-radius:6px;word-break:break-all}"
    "label{font-weight:bold;color:#2c3e50;display:block;margin-bottom:6px}"
    "textarea{width:100%;padding:10px;border:2px solid #ddd;border-radius:10px;font-size:14px;resize:vertical;min-height:90px;font-family:monospace}"
    "textarea:focus{outline:none;border-color:#3498db}"
    "button{margin-top:14px;width:100%;padding:14px;background:linear-gradient(135deg,#3498db,#2980b9);color:#fff;border:none;border-radius:10px;font-size:16px;font-weight:bold;cursor:pointer}"
    ".alert{margin-top:14px;padding:12px;border-radius:8px;display:none}"
    ".ok{background:#d4edda;color:#155724;border:1px solid #c3e6cb}"
    ".err{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}"
    "</style></head><body>"
    "<div class='card'>"
    "<h1>📟 Running Text P10</h1>"
    "<div class='sub'>ESP32 + 2x Panel P10</div>"
    "<div class='cur'><b>📝 Teks Saat Ini:</b><div id='curTxt'></div></div>"
    "<label>✏️ Teks Baru:</label>"
    "<textarea id='txt' maxlength='200'></textarea>"
    "<button id='btn' onclick='send()'>💾 Simpan & Tampilkan</button>"
    "<div id='alert' class='alert'></div>"
    "</div>"
    "<script>"
    "const cur=");

  html += "\"" + runningText + "\"";

  html += F(";"
    "document.getElementById('curTxt').textContent=cur;"
    "document.getElementById('txt').value=cur;"
    "function send(){"
      "const t=document.getElementById('txt').value.trim();"
      "if(!t){showAlert('Teks tidak boleh kosong!','err');return;}"
      "const btn=document.getElementById('btn');"
      "btn.disabled=true;btn.textContent='⏳ Menyimpan...';"
      "fetch('/update',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"
        "body:'text='+encodeURIComponent(t)})"
      ".then(r=>{"
        "if(!r.ok)throw new Error('Gagal');"
        "return r.text();"
      "})"
      ".then(()=>{"
        "document.getElementById('curTxt').textContent=t;"
        "showAlert('✅ Berhasil disimpan!','ok');"
      "})"
      ".catch(()=>showAlert('❌ Gagal, coba lagi.','err'))"
      ".finally(()=>{btn.disabled=false;btn.textContent='💾 Simpan & Tampilkan';});"
    "}"
    "function showAlert(m,c){"
      "const a=document.getElementById('alert');"
      "a.textContent=m;a.className='alert '+c;a.style.display='block';"
      "setTimeout(()=>a.style.display='none',3000);"
    "}"
    "</script></body></html>");

  /* ===============================================================
     📡 KODE RESPON HTTP (yang dikirim server ke browser)
     ===============================================================
     
     Kode 200 (OK)        : "Semua beres, permintaan sukses!"
                            Contoh: Halaman HTML berhasil dikirim,
                            atau teks baru berhasil diterima.
                            
     Kode 400 (Bad Request): "Maaf, permintaan kamu salah ya..."
                            Contoh: Lupa mengisi teks, form dikirim kosong.
                            
     Kode 404 (Not Found)  : "Wah, halaman yang kamu cari tidak ada."
                            Contoh: Kamu ngetik "192.168.1.1/salah" 
                            di browser, padahal halaman itu tidak tersedia.
     =============================================================== */
  server.send(200, "text/html", html);  // 200 = OK
}

// Menangani POST dari form web untuk mengupdate teks
void handleUpdateText() {
  // Cek apakah ada parameter "text" dan tidak kosong
  if (!server.hasArg("text") || server.arg("text").length() == 0) {
    server.send(400, "text/plain", "Teks kosong");  // 400 = Bad Request
    return;
  }

  String newText = server.arg("text");
  if ((int)newText.length() > MAX_TEXT_LENGTH)
    newText = newText.substring(0, MAX_TEXT_LENGTH);

  server.send(200, "text/plain", "OK");  // 200 = OK (berhasil)

  pendingText   = newText;
  pendingUpdate = true;
}
