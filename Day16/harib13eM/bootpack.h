/* asmhead.nas */
/*構造体の宣言*/
struct BOOTINFO { /* 0x0ff0-0x0fff */
    char cyls; /* ブートセクタがどこまでディスクを呼んだか */
    char leds; /* ブート時のキーボードのLED状態 */
    char vmode; /* ビデオモード  何ビットカラーか */
    char reserve;
    short scrnx, scrny; /* 画面解像度 */
    char *vram;
};
#define ADR_BOOTINFO  0x00000ff0

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port); /* 書き忘れ、過去のコードの書き込みを要確認 */
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0(void);
void store_cr0(int cr0);
void load_tr(int tr);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void farjmp(int eip, int cs);

/* fifo.c */
struct FIFO32 {
    int *buf; /* バッファの位置を特定するためのアドレス(intに修正) */
    int p, q, size, free, flags; /* p：書き込み位置,q:読み込み位置,size:バッファの大きさ,free:バッファの空き容量,flags:あふれの確認フラグ, */
    struct TASK *task;  /* 起こしたいタスクのデータ */
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);  /* FIFOバッファの初期化 */
int fifo32_put(struct FIFO32 *fifo, int data);  /* FIFOへデータを送り込んで蓄える */
int fifo32_get(struct FIFO32 *fifo);  /* FIFOからデータを1つとって来る */
int fifo32_status(struct FIFO32 *fifo);   /* どのくらいデータがたまっているかを報告する */

/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);
#define COL8_000000     0
#define COL8_FF0000     1
#define COL8_00FF00     2
#define COL8_FFFF00     3
#define COL8_0000FF     4
#define COL8_FF00FF     5
#define COL8_00FFFF     6
#define COL8_FFFFFF     7
#define COL8_C6C6C6     8
#define COL8_840000     9
#define COL8_008400     10
#define COL8_848400     11
#define COL8_000084     12
#define COL8_840084     13
#define COL8_008484     14
#define COL8_848484     15

/* dsctbl.c */
/* GDTの中身（2*2 + 1*2 + 2 = 8バイト） */
struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
/* IDTの中身（2*2 + 1*2 + 2 = 8バイト） */
struct GATE_DESCRIPTOR{
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
/* ヘッダー関連 */
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT     0x0026f800
#define LIMIT_IDT   0x000007ff
#define ADR_GDT     0x00270000
#define LIMIT_GDT   0x0000ffff
#define ADR_BOTPAK    0x00280000
#define LIMIT_BOTPAK  0x0007ffff
#define AR_DATA32_RW  0x4092
#define AR_CODE32_ER  0x409a
#define AR_TSS32      0x0089
#define AR_INTGATE32  0x008e

/* int.c */
/* KEYBUFは削除 */
void init_pic(void);

void inthandler27(int *esp);

#define PIC0_ICW1        0x0020
#define PIC0_OCW2        0x0020
#define PIC0_IMR         0x0021
#define PIC0_ICW2        0x0021
#define PIC0_ICW3        0x0021
#define PIC0_ICW4        0x0021
#define PIC1_ICW1        0x00a0
#define PIC1_OCW2        0x00a0
#define PIC1_IMR         0x00a1
#define PIC1_ICW2        0x00a1
#define PIC1_ICW3        0x00a1
#define PIC1_ICW4        0x00a1

/* keyboard.c */
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);
#define PORT_KEYDAT     0x0060      /* KBCデータポート/キーボードデータ */
#define PORT_KEYCMD     0x0064      /* KBCへのモード設定コマンド */

/* mouse.c */
struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};
void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

/* memory.c */
/* メモリ管理用 */
#define MEMMAN_FREES    4090    /*  */
#define MEMMAN_ADDR     0x003c0000
/* Freeのアドレスとサイズ管理 */

struct FREEINFO {       /* あき情報 */
    unsigned int addr, size;    /* 各種、addr:アドレス, size:大きさ */
};
/* 失ったサイズやどれだけFreeかの管理 */
struct MEMMAN {         /* メモリ管理 */
    int frees, maxfrees, lostsize, losts;   /* 関数memman_initを参照 */
    struct FREEINFO free[MEMMAN_FREES];     /* 配列で表にする */
};

/* 新しく追加する関数 */
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* sheet.c */
#define MAX_SHEETS      256
/* 下敷き1枚の情報 */
struct SHEET {
    unsigned char *buf;
    int bxsize, bysize, vx0, vy0, col_inv, height, flags;
    /* boxsize:下敷きの大きさx, bysize:下敷きの大きさy, vx0:下敷きの位置x, vy0:下敷きの位置y */
    /*col_inv:透明色の番号, height:下敷きの高さ, flags:下敷きの設定情報 */
    struct SHTCTL *ctl; /* ctlをこちらでも管理 */
};
/* 各種下敷きの管理情報 */
struct SHTCTL {
    unsigned char *vram, *map; /* vramのアドレス, マップのアドレス管理 */
    int xsize, ysize, top; /* xsize:全体画面の大きさx, ysize:全体画面の大きさy, top:1番上の下敷きの高さ */
    struct SHEET *sheets[MAX_SHEETS]; /* SHEETのデータを並び変えるためにアドレスを管理 */
    struct SHEET sheets0[MAX_SHEETS]; /* SHEETのデータを256枚分用意 */
};
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

/* timer.c */
#define MAX_TIMER   500
struct TIMER {
    struct TIMER *next;
    unsigned int timeout, flags;
    struct FIFO32 *fifo;
    int data;
};
struct TIMERCTL {
    unsigned int count, next;    
    /* count:カウンタ,next:次に注目するタイマー,using:使用中タイマの個数 */
    struct TIMER *t0;
    /* タイマーを順番に管理 */
    struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);

/* mtask.c */
#define MAX_TASKS   1000    /* 最大タスク数 */
#define TASK_GDT0   3       /* TSSをGDTの何番目から割り当てるのか */
#define MAX_TASKS_LV     100
#define MAX_TASKLEVELS   10
struct TSS32 {  /* 32bit ver task status segment */
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;      /* タスク用変数 */
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;     /* 32bitレジスタ */
    int es, cs, ss, ds, fs, gs;     /* 16bitレジスタ */
    int ldtr, iomap;        /* LDTR=0, iomap=0x4000_0000 */
};
struct TASK {
    int sel, flags; /* sel(selector)はGDTの番号のこと */
    int level,priority;   /* priority：優先度 */
    struct TSS32 tss;   /* バックアップする各種レジスタ */
};
struct TASKLEVEL {
    int running; /* 動作しているタスクの数 */
    int now;     /* 現在動作しているタスクがどれだかわかるようにするための変数 */
    struct TASK *tasks[MAX_TASKS_LV]; /* 登録できるタスクは1レベル100個 */
};
struct TASKCTL {
    int now_lv;   /* now_lv：現在動作中のレベル */
    char lv_change;    /* lv_change：次回タスクスイッチのときに、レベルを変えたほうがいいかどうか */
    struct TASKLEVEL level[MAX_TASKLEVELS];  /* レベル10まで存在 */
    struct TASK tasks0[MAX_TASKS];
};
extern struct TIMER *task_timer;
struct TASK *task_init(struct MEMMAN *memman);  /* TASKCTLは巨大なのでメモリを確保 */
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
