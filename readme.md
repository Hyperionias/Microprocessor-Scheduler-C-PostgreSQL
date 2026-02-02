# 8086 OS & Round Robin Scheduler Simulation

Bu proje, Mert Danacı tarafından geliştirilen, 8086 mimarisinin bellek adresleme mantığını ve işletim sistemi çizelgeleme (scheduling) algoritmalarını simüle eden bir C çalışmasıdır. Proje, hem düşük seviyeli donanım etkileşimini hem de modern veritabanı loglama sistemlerini bir araya getirir.

---

## 🚀 Öne Çıkan Özellikler

* **8086 Bellek Segmentasyonu:** 1 MB (2^20 byte) boyutundaki fiziksel bellek, gerçek 8086 mantığıyla (Segment × 16 + Offset) simüle edilmiştir.
* **Round Robin (RR) Çizelgeleyici:** Prosesler, belirli bir "Time Quantum" (3 birim) süresince CPU üzerinde çalıştırılır ve ardından kuyruğa geri alınır.
* **PostgreSQL Entegrasyonu:** `libpq` kütüphanesi kullanılarak her bellek yazma işlemi ve proses geçişi otomatik olarak `sim_db` veritabanına loglanır.
* **Dinamik Proses Yönetimi:** Proseslerin durumları (READY, RUNNING, TERMINATED) gerçek zamanlı olarak takip edilir.

---

## 🧠 Çalışma Mantığı

### 1. Adres Hesaplama
8086 işlemcilerde fiziksel adres, 16 bitlik segment ve offset değerlerinin kaydırılarak toplanmasıyla elde edilir:

```
PhysicalAddress = (Segment × 0x10) + Offset
```

**Örnek:**
- Segment: `0x1000`
- Offset: `0x00FF`
- Fiziksel Adres: `0x1000 × 16 + 0x00FF = 0x100FF` (65791)

### 2. Çizelgeleme (Round Robin)
Prosesler kuyruğa girer ve her birine eşit süre verilir. Süresi biten ancak işlemi tamamlanmayan proses, kuyruğun sonuna eklenir.

**Algoritma Akışı:**
1. Tüm prosesler başlangıçta READY durumundadır
2. Sıradaki proses RUNNING durumuna geçer
3. Time Quantum (3 birim) kadar çalışır
4. Süre bittiğinde:
   - İşlem tamamlanmışsa → TERMINATED
   - İşlem devam ediyorsa → READY (kuyruğun sonuna)
5. Tüm prosesler bitene kadar döngü devam eder

---

## 🛠️ Kurulum

### Gereksinimler
* **GCC/MinGW** derleyicisi
* **PostgreSQL 18** (Geliştirici kütüphaneleriyle birlikte: `libpq`)
* **CMake** (3.20 veya üzeri)

### Veritabanı Kurulumu
Logların kaydedilmesi için PostgreSQL üzerinde aşağıdaki komutları çalıştırın:

```sql
-- Veritabanı oluştur
CREATE DATABASE sim_db;

-- Veritabanına bağlan
\c sim_db

-- Log tablosunu oluştur
CREATE TABLE simulation_logs (
    id SERIAL PRIMARY KEY,
    pid INT NOT NULL,
    action_type VARCHAR(50),
    physical_addr INT,
    log_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### Derleme (CMake ile)

**CLion IDE kullanıyorsanız:**
1. Projeyi CLion'da açın
2. `main.c` dosyasında **satır 265**'teki şifre alanını güncelleyin:
   ```c
   PQconnectdb("user=postgres password=SIZIN_SIFRENIZ dbname=sim_db host=localhost");
   ```
3. Build → Build Project (Ctrl+F9)
4. Run → Run '8086_Simulator' (Shift+F10)

**Manuel derleme (Terminal):**
```bash
mkdir build
cd build
cmake ..
cmake --build .
./8086_Simulator.exe
```

---

## 📊 Örnek Çıktı

```
--- YTU 8086 OS Simulasyonu (Mert Danaci) ---

[SUCCESS] PostgreSQL baglantisi basarili!
[PROCESS] PID 1 olusturuldu. Segment: 0x1000, Burst: 10, Priority: 2
[PROCESS] PID 2 olusturuldu. Segment: 0x2000, Burst: 5, Priority: 1

--- Round Robin Cizelgeleme Baslatiliyor (TQ: 3) ---
[RUNNING] PID 1 calisiyor (Kalan Sure: 10)... Zaman bitti, siraya geri dondu.
[WRITE] Segment: 0x1000, Offset: 0x00FF -> Physical: 0x100FF, Value: 0x07
[RUNNING] PID 2 calisiyor (Kalan Sure: 5)... Zaman bitti, siraya geri dondu.
[WRITE] Segment: 0x2000, Offset: 0x00FF -> Physical: 0x200FF, Value: 0x02
...

--- Tum islemler basariyla tamamlandi! ---
```

---

## 📂 Proje Yapısı

```
8086_Simulator/
├── main.c              # Ana kaynak dosya (RAM, Scheduler, DB Log)
├── CMakeLists.txt      # CMake build yapılandırması
├── README.md           # Proje dokümantasyonu
└── cmake-build-debug/  # Derlenmiş dosyalar
```

---

## 🔧 Yapılandırma

### PostgreSQL Bağlantı Ayarları
`main.c` dosyasında **satır 242**'teki bağlantı dizesini kendi ortamınıza göre düzenleyin:

```c
// Satır 265 (main fonksiyonu içinde)
PQconnectdb("user=postgres password=SIZIN_SIFRENIZ dbname=sim_db host=localhost");
```

### Time Quantum Değiştirme
```c
#define TIME_QUANTUM 3  // İstediğiniz değere değiştirin (örn: 5)
```

### Bellek Boyutu
```c
#define RAM_SIZE 1048576  // 1 MB (değiştirilebilir)
```

---

## 🎯 Gelecek Geliştirmeler

- [ ] Grafiksel kullanıcı arayüzü (GUI) eklenmesi
- [ ] Priority Scheduling algoritması desteği
- [ ] Bellek fragmentasyonu görselleştirmesi
- [ ] Çoklu CPU çekirdeği simülasyonu
- [ ] İşlem çakışma (conflict) tespiti ve çözümü

---

## 👤 Geliştirici

**Mert Danacı**  
Bilgisayar Mühendisliği Bölümü  
Yıldız Teknik Üniversitesi

---

## 📝 Notlar

- Veritabanı bağlantı bilgilerini (`user`, `password`, `dbname`) kendi yerel ayarlarınıza göre `main()` fonksiyonu içerisinden güncellemeyi unutmayın.
- PostgreSQL servisinin çalıştığından emin olun (Windows Services → postgresql-x64-18).
- Proje Windows 10/11 ve PostgreSQL 18 üzerinde test edilmiştir.

---

## 📄 Lisans

Bu proje eğitim amaçlı geliştirilmiştir ve Mert Danacı kendi çalışmasıdır.
