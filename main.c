#include <stdio.h>
#include <libpq-fe.h>   // PostgreSQL C kutuphanesi (libpq)

/* ------------------ SABITLER ------------------ */

// Toplam RAM boyutu (1 MB)
#define RAM_SIZE 1048576

// Segment boyutu (64 KB)
#define SEGMENT_SIZE 65536

// Round Robin zaman dilimi (time quantum)
#define TIME_QUANTUM 3

/* ------------------ RAM ------------------ */

// Fiziksel bellek simulasyonu (1 MB byte dizisi)
unsigned char ram[RAM_SIZE];

/* ------------------ REGISTER YAPISI ------------------ */

// 8086 benzeri segment registerlari
typedef struct Registers {
    unsigned short cs;  // Code Segment
    unsigned short ds;  // Data Segment
    unsigned short ss;  // Stack Segment
} Registers;

/* ------------------ PROCESS STATE ------------------ */

// Bir prosesin durumlari
typedef enum {
    READY,       // Calismaya hazir
    RUNNING,     // CPU uzerinde calisiyor
    WAITING,     // Beklemede (kullanilmiyor ama eklendi)
    TERMINATED   // Bitmis
} ProcessState;

/* ------------------ PROCESS YAPISI ------------------ */

// Isletim sistemindeki proses modeli
typedef struct {
    int pid;                    // Process ID
    unsigned short segment;     // Prosesin segment adresi
    int burst_time;             // Toplam calisma suresi
    int remaining_time;         // Kalan calisma suresi
    int priority;               // Oncelik (Round Robin icin kullanilmiyor)
    ProcessState state;          // Proses durumu
} Process;

/* ------------------ VERITABANI LOG FONKSIYONU ------------------ */

// Simulasyon adimlarini PostgreSQL veritabanina kaydeder
void log_to_db(PGconn *conn,
               int pid,
               const char* action,
               unsigned short seg,
               unsigned short off,
               int phys) {

    // Baglanti kopuksa hicbir sey yapma
    if (PQstatus(conn) == CONNECTION_BAD) return;

    char query[512];     // SQL sorgusu
    char seg_str[10];    // Segment adresi string
    char off_str[10];    // Offset adresi string

    // Segment ve offset hexadecimal stringe cevriliyor
    sprintf(seg_str, "0x%04X", seg);
    sprintf(off_str, "0x%04X", off);

    // INSERT SQL sorgusu hazirlaniyor
    sprintf(query,
        "INSERT INTO simulation_logs "
        "(pid, action_type, segment_addr, offset_addr, physical_addr, details) "
        "VALUES (%d, '%s', '%s', '%s', %d, 'Round Robin step completed');",
        pid, action, seg_str, off_str, phys
    );

    // Sorgu calistiriliyor
    PGresult *res = PQexec(conn, query);

    // Hata kontrolu
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Sorgu hatasi: %s\n", PQerrorMessage(conn));
    }

    // Result bellekten temizleniyor
    PQclear(res);
}

/* ------------------ PROCESS KUYRUGU ------------------ */

// Maksimum 10 proseslik sabit dizi
Process process_queue[10];

// Aktif proses sayisi
int process_count = 0;

/* ------------------ ADRES CEVIRME ------------------ */

// Segment:Offset -> Fiziksel adres cevirimi
// 8086 mantigi: fiziksel = segment * 16 + offset
int translate_address(unsigned short segment, unsigned short offset) {
    return (segment * 16) + offset;
}

/* ------------------ RAM YAZMA ------------------ */

// RAM'e bir byte yazma fonksiyonu
void write_byte(unsigned short segment,
                unsigned short offset,
                unsigned char value) {

    int phys_addr = translate_address(segment, offset);

    // RAM sinirlari icinde mi kontrolu
    if (phys_addr < RAM_SIZE) {
        ram[phys_addr] = value;

        // Yapilan yazma islemi ekrana basiliyor
        printf("[WRITE] Segment: 0x%04X, Offset: 0x%04X -> "
               "Physical: 0x%05X, Value: 0x%02X\n",
               segment, offset, phys_addr, value);
    }
}

/* ------------------ PROSES OLUSTURMA ------------------ */

// Yeni proses olusturur ve kuyruga ekler
void create_process(int burst, int priority) {

    // Maksimum proses sayisi asilmamali
    if (process_count >= 10) return;

    Process p;

    // PID otomatik artiyor
    p.pid = process_count + 1;

    // Her proses farkli segment aliyor
    p.segment = (process_count + 1) * 0x1000;

    // Burst ve kalan sure ayarlaniyor
    p.burst_time = burst;
    p.remaining_time = burst;

    // Oncelik bilgisi
    p.priority = priority;

    // Baslangic durumu READY
    p.state = READY;

    // Proses kuyruğa ekleniyor
    process_queue[process_count++] = p;

    printf("[PROCESS] PID %d olusturuldu. Segment: 0x%04X, "
           "Burst: %d, Priority: %d\n",
           p.pid, p.segment, p.burst_time, p.priority);
}

/* ------------------ ROUND ROBIN SCHEDULER ------------------ */

// Round Robin zamanlayici
void run_round_robin(PGconn *conn) {

    printf("\n--- Round Robin Cizelgeleme Baslatiliyor "
           "(TQ: %d) ---\n", TIME_QUANTUM);

    int completed_processes = 0;

    // Tum prosesler bitene kadar devam et
    while (completed_processes < process_count) {

        // Proses kuyruğunu dolaş
        for (int i = 0; i < process_count; i++) {

            Process *p = &process_queue[i];

            // Bitmis prosesler atlanir
            if (p->state != TERMINATED) {

                // Proses CPU'ya aliniyor
                p->state = RUNNING;

                printf("[RUNNING] PID %d calisiyor "
                       "(Kalan: %d)... ",
                       p->pid, p->remaining_time);

                // Harcanacak sure hesaplanir
                int time_spent;

                if (p->remaining_time > TIME_QUANTUM) {
                    time_spent = TIME_QUANTUM;
                } else {
                    time_spent = p->remaining_time;
                }

                // Kalan sure azaltılır
                p->remaining_time -= time_spent;

                // Kalan sure RAM'e yaziliyor
                write_byte(p->segment, 0xFF,
                           (unsigned char)p->remaining_time);

                // Fiziksel adres hesaplanir
                int phys_addr =
                    translate_address(p->segment, 0xFF);

                // Veritabani log kaydi
                log_to_db(conn,
                          p->pid,
                          "PROCESS_RUN",
                          p->segment,
                          0xFF,
                          phys_addr);

                // Proses bitmis mi?
                if (p->remaining_time == 0) {
                    p->state = TERMINATED;
                    completed_processes++;
                    printf("BITTI!\n");
                } else {
                    // Bitmediyse tekrar READY olur
                    p->state = READY;
                    printf("Siraya dondu.\n");
                }
            }
        }
    }
}

/* ------------------ MAIN ------------------ */

int main(void) {

    printf("--- YTU 8086 OS Simulasyonu "
           "(Mert Danaci) ---\n\n");

    // PostgreSQL baglantisi
    PGconn *conn =
        PQconnectdb("user=postgres password=12345 "
                    "dbname=sim_db host=localhost");

    // Baglanti kontrolu
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr,
                "[ERROR] Baglanti Basarisiz: %s\n",
                PQerrorMessage(conn));
        PQfinish(conn);
        return 1;
    }

    printf("[SUCCESS] PostgreSQL baglantisi kuruldu.\n");

    // Ornek prosesler
    create_process(10, 2);
    create_process(5, 1);
    create_process(7, 3);

    // Round Robin baslatiliyor
    run_round_robin(conn);

    // Veritabani baglantisi kapatiliyor
    PQfinish(conn);

    printf("\nSimulasyon tamamlandi. "
           "Loglar veritabanina kaydedildi.\n");

    return 0;
}
