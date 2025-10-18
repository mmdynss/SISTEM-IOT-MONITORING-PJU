# SISTEM-IOT-MONITORING-PJU
🌙 Monitoring Intensitas Cahaya Lampu Jalan Berbasis IoT

Proyek ini bertujuan untuk memantau intensitas cahaya lampu jalan secara real-time menggunakan ESP32 sebagai mikrokontroler utama. Sistem ini tidak hanya mengukur tingkat pencahayaan, tetapi juga merekam data lingkungan dan lokasi geografis secara otomatis untuk analisis lebih lanjut.

🧠 Deskripsi Singkat

Sistem ini dirancang untuk melakukan monitoring intensitas cahaya lampu jalan pada malam hari. Dengan integrasi berbagai sensor, alat ini mampu:

Mendeteksi intensitas cahaya menggunakan sensor BH1750

Mengukur suhu dan kelembapan menggunakan sensor HTU21D

Menentukan posisi geografis dengan sensor GPS NEO-M8N

Menampilkan data secara langsung di layar SSD1306 OLED

Menyimpan seluruh data ke MicroSD dalam format CSV dengan nama file otomatis harian

Data yang dikumpulkan dapat digunakan untuk:

Analisis performa lampu jalan

Efisiensi energi

Deteksi dini gangguan pencahayaan

Perencanaan sistem penerangan berbasis data

⚙️ Komponen yang Digunakan

🧩 ESP32 (mikrokontroler utama & konektivitas WiFi)

💡 BH1750 (sensor intensitas cahaya)

🌡️ HTU21D (sensor suhu & kelembapan)

📍 GPS NEO-M8N (penentuan koordinat lokasi)

🖥️ SSD1306 OLED Display (tampilan data real-time)

💾 MicroSD Module (penyimpanan data CSV)

🗂️ Fitur Utama

✅ Monitoring intensitas cahaya secara real-time
✅ Pencatatan data suhu, kelembapan, dan koordinat GPS
✅ Penyimpanan otomatis ke MicroSD dengan format harian
✅ Tampilan data langsung di layar OLED
✅ Siap dikembangkan untuk integrasi cloud (Firebase / ThingsBoard)

📊 Format Data CSV
Waktu	Intensitas (lux)	Suhu (°C)	Kelembapan (%)	Latitude	Longitude
21:00:01	120	29.5	72	-0.92145	104.45722
🚀 Rencana Pengembangan

Integrasi dashboard web untuk visualisasi peta & grafik historis

Penambahan fitur notifikasi jika lampu padam

Pengiriman data otomatis ke Firebase / server IoT

💬 Proyek ini dikembangkan sebagai bagian dari penelitian dan eksplorasi IoT dalam bidang energi & smart city oleh mahasiswa Teknik Elektro Universitas Maritim Raja Ali Haji.
