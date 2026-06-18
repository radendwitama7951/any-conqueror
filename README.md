# Any Conqueror
## by Raden Dwitama B

Proyek pengembangan engine dan game strategi taktis berbasis data (*data-driven*) yang dibangun menggunakan teknologi sistem tingkat rendah modern. Engine ini mengutamakan performa tinggi, portabilitas lintas platform, dan keamanan aset melalui pipeline standar terbuka.

## 🚀 Fitur Utama

* **Core Engine:** Ditulis menggunakan standar **ISO C17** dan **ISO C++20** murni.
* **Graphics API:** **Vulkan 1.3** (menggunakan *dynamic loading* via `volk`).
* **Windowing & Input:** **SDL3** (Pre-release/Latest).
* **Interface:** **Dear ImGui** (Immediate Mode GUI) dengan backend SDL3 + Vulkan.
* **Asset Pipeline:**
* Kompilasi otomatis Shader GLSL ke SPIR-V.
* Bundling tekstur PNG ke dalam kontainer **KTX2 Texture Arrays** (UASTC encoding).


* **Logging:** Integrasi `rxi/log` untuk debugging yang thread-safe dan berwarna.

## 🛠 Prasyarat Sistem

Sebelum melakukan build, pastikan perangkat Anda memiliki komponen berikut:

### Umum (Semua Platform)

* **CMake 3.20** atau lebih tinggi.
* **Vulkan SDK** (untuk `glslangValidator` dan library Vulkan).
* **KTX-Software** (dibutuhkan `toktx` dan `ktxinfo` terinstal di PATH sistem untuk menjalankan pipeline aset).

### Platform Spesifik

* **Linux:** GCC 11+ atau Clang 13+, library pengembangan X11/Wayland.
* **Windows:** Visual Studio 2022 (MSVC) atau Clang-cl.
* **Android:** Android NDK (r25+) dan Android Studio.

## 📦 Manajemen Dependensi

Proyek ini menggunakan `FetchContent` dari CMake untuk mengunduh dan mengonfigurasi dependensi secara otomatis saat proses konfigurasi:

* **SDL3:** Manajemen jendela dan OS abstraction.
* **volk:** Meta-loader untuk API Vulkan.
* **Vulkan Memory Allocator (VMA):** Manajemen memori GPU yang efisien.
* **KTX-Software:** Library untuk memproses tekstur GPU (dimuat sebagai library eksternal berdasarkan OS).
* **Dear ImGui:** Antarmuka pengguna untuk editor dan debug.

## 🏗 Instruksi Build

### 1. Linux & Windows (Desktop)

Gunakan perintah standar CMake untuk melakukan kompilasi:

```bash
# Konfigurasi proyek
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build proyek
cmake --build build -j$(nproc)

# Menjalankan aplikasi secara otomatis (menggunakan target custom)
cmake --build build --target AnyConqueror-run

```

### 2. Android

Untuk platform Android, proyek ini menghasilkan library bersama (`.so`). Konfigurasi biasanya dilakukan melalui integrasi Gradle di Android Studio, namun secara teknis CMake akan:

* Mengunduh library KTX khusus arsitektur `arm64-v8a`.
* Menyetel nama output menjadi `libmain.so`.

## 🎨 Pipeline Aset

Build sistem akan memproses aset secara otomatis setiap kali ada perubahan pada file sumber:

1. **Shaders:** File di `src/main/res/shaders/` akan dikompilasi menjadi biner `.spv` di folder `src/main/assets/shaders/`.
2. **Textures:** File PNG di `src/main/res/drawables/` akan dibundel menjadi array tekstur `.ktx2` menggunakan `toktx`.
* Mendukung tekstur berukuran `1920x1080` dan `1024x1024`.
* Menggunakan encoding **UASTC** untuk efisiensi VRAM.



## 📂 Struktur Proyek

* `src/main/`: Kode sumber utama (C/C++).
* `src/main/res/`: Sumber aset mentah (Shader GLSL, PNG).
* `src/main/assets/`: Output aset siap pakai (SPIR-V, KTX2) yang akan dikemas ke dalam aplikasi.
* `extern/`: Library pihak ketiga yang dikelola secara manual (misal: `rxi_log`).

## ⚠️ Catatan Pengembangan

* **Vulkan Prototypes:** Proyek ini menggunakan `VK_NO_PROTOTYPES`. Semua fungsi Vulkan dimuat secara dinamis melalui `volk`.
* **ImGui & Volk:** Backend ImGui dikonfigurasi untuk menggunakan `volk` melalui definisi `IMGUI_IMPL_VULKAN_USE_VOLK`.

---

**Disclaimer:** Proyek ini masih dalam tahap pengembangan aktif. Pastikan driver GPU Anda mendukung Vulkan 1.3.
