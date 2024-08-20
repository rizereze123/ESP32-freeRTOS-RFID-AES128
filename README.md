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

![Screenshot 2024-06-29 at 22-45-35 EasyEDA(Standard) - A Simple and Powerful Electronic Circuit Design Tool](https://github.com/user-attachments/assets/8e3fe4ba-eede-415c-b24e-e19c189f6c6e)
![Blok Diagram Perangkat(1) drawio](https://github.com/user-attachments/assets/d4061313-11c9-4778-bd08-c69df67d0785)
![wiring_bb](https://github.com/user-attachments/assets/28f11874-fb7c-4715-8ba8-3787134c7cca)
![Arsitektur Diagram Sistem IoT drawio](https://github.com/user-attachments/assets/ff8853ec-923f-4d99-8c64-3db523a3c28c)



