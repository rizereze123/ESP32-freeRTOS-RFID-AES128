Sistem Pengamanan Pintu RFID berbasis IoT dengan Algoritma Enkripsi AES-128 (Protokol Komunikasi MQTT).
Repository ini untuk Microcontroller ESP32 dengan menggunakan kernel sistem operasi FreeRTOS dalam menjalankan beberapa inputan reader RFID, sensor fingerprint, serta luaran respons LED/LCD/Buzzer dan aktuator relay doorlock solenoid

Sistem ini merupakan buah hasil dari pengembangan dan penelitian yang dilakukan oleh Dhimaz Purnama Adjhi untuk menyelesaikan skripsinya dalam meraih gelar sarjana teknik. Inovasi ini diharapkan menjadi standar industri untuk ruangan dengan tingkat keamanan tinggi seperti ruangan server, ruang balita di rumah sakit, laboratorium, gudang, dan brankas. Dengan demikian, penelitian ini berkontribusi pada pengembangan paradigma keamanan yang lebih tangguh dalam protokol komunikasi MQTT melalui penerapan enkripsi data AES-128, meningkatkan integritas dan kerahasiaan data dalam sistem pengamanan pintu RFID berbasis IoT. Hasil penelitian menunjukkan bahwa implementasi algoritma enkripsi AES-128 berhasil mengamankan data penting seperti UID kartu/tag RFID pada sistem pengamanan pintu RFID berbasis IoT, mencegah penyalahgunaan hak akses, dan mengurangi risiko serangan network sniffing dan MiTM.

<img width="411" alt="wdefrgtgrfe" src="https://github.com/user-attachments/assets/b12d470b-72cf-4897-965d-4d77e9be4337">
<img width="593" alt="efrgtrfe" src="https://github.com/user-attachments/assets/294d7db5-7c30-4441-852f-41e4c3fd4282">
<img width="593" alt="efrgtrfe" src="https://github.com/user-attachments/assets/0a009085-0711-4cac-866a-efa55dbd2b41">
<img width="593" alt="efrgtrfe" src="https://github.com/user-attachments/assets/f7fdbfba-8e79-423c-9159-0e8d0f068f26">

## Feature

- Kernel Sistem Operasi FreeRTOS, Multi-Task
- Pembacaan Tapping RFID
- Scanning Fingerprint
- LCD 16x2 Karakter
- RGB LED dan Buzzer Alarm
- Tombol Menutup dan Membuka Pintu dari dalam Ruangan
- Tombol Pendaftaran dari dalam Ruangan
- Keypad untuk Pendaftaran ID Fingerprint
- MQTT Protocol Based

## Tech

#include <Crypto.h>
#include <AES.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
