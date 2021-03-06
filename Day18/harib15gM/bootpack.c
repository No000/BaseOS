/* bootpackのメイン */

#include "bootpack.h"
#include <stdio.h>
#include <string.h>

struct FILEINFO {
    unsigned char name[8], ext[3], type; /* name：Fileネーム、ext：拡張子、type：ファイル情報 */
    char reserve[10];   /* 予約領域 */
    unsigned short time, data, clustno; /* time：時刻、data：日付、clustno：セクタ番号*/
    unsigned int size;  /* Fileサイズ */
};

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);  /* 文字出力をまとめる */
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void console_task(struct SHEET *sheet, unsigned int memtotal); /* コンソール用のタスク */
int cons_newline(int cursor_y, struct SHEET *sheet);

#define KEYCMD_LED      0xed

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct SHTCTL *shtctl; /* シートの管理 */
    char s[40];
    struct FIFO32 fifo, keycmd;
    int fifobuf[128], keycmd_buf[32];
    int mx, my, i, cursor_x, cursor_c;
    unsigned int memtotal;
    struct MOUSE_DEC mdec; /* マウスのデータを構造体で管理（タグ：mdec） */
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
    struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;    /* ウィンドウを4つ用意 */
    struct TASK *task_a, *task_cons;    /* タスクAとコンソールタスク */
    struct TIMER *timer;
    static char keytable0[0x80] = {
        0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
    };
    static char keytable1[0x80] = {
        0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
    };
    int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1; /* key_to：ウィンドウ, key_shift：シフト, key_les：SL・NL・CL */

    init_gdtidt(); /* GDT、IDTの初期化 */
    init_pic(); /* PICの初期か */
    io_sti();   /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */
    fifo32_init(&fifo, 128, fifobuf, 0);
    init_pit(); /* PITの初期化 */
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);
    io_out8(PIC0_IMR, 0xf8); /* PITとPIC1とキーボードを許可（1111_1000） */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可（1110_1111） */
    fifo32_init(&keycmd, 32, keycmd_buf, 0);

    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 -0x0009efff */
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();/* 以下重ね合わせ処理 */
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);  /* シート全体の情報の初期化 */
    task_a = task_init(memman);
    fifo.task = task_a;
    task_run(task_a, 1, 2); /* レベルは１, priorityは0？ */
    
    /* sht_back */
    sht_back  = sheet_alloc(shtctl);    /* 背景のシートの確保 */
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);     /* 背景バッファのメモリ確保 */
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 透明色なし */
    init_screen8(buf_back, binfo->scrnx, binfo->scrny); /* 背景を描画 */

    /* sht_cons */
    sht_cons = sheet_alloc(shtctl);
    buf_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
    sheet_setbuf(sht_cons, buf_cons, 256, 165, -1); /* 透明色なし */
    make_window8(buf_cons, 256, 165, "console", 0);
    make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);  /* 黒のテキストボックス */
    task_cons = task_alloc();
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;   /* スタックを増やすために6->12 */
    task_cons->tss.eip = (int) &console_task;   /* console_taskを登録 */
    task_cons->tss.es = 1 * 8;
    task_cons->tss.cs = 2 * 8;
    task_cons->tss.ss = 1 * 8;
    task_cons->tss.ds = 1 * 8;
    task_cons->tss.fs = 1 * 8;
    task_cons->tss.gs = 1 * 8;
    *((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;
    *((int *) (task_cons->tss.esp + 8)) = memtotal; /* Harimainから渡してもらうためのスタック */
    task_run(task_cons, 2, 2); /* level=2, priority=2 */

    /* sht_win */
    sht_win   = sheet_alloc(shtctl);    /* ウィンドウ用のメモリ確保 */
    buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52); /* ウィンドウバッファのメモリ確保 */
    sheet_setbuf(sht_win, buf_win, 144, 52, -1); /* 透明色なし */
    make_window8(buf_win, 144, 52, "task_a", 1); /* カウンターウィンドウ */
    make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF);    /* テキストボックス */
    cursor_x = 8;               /* カーソルの表示位置保管変数 */
    cursor_c = COL8_FFFFFF;      /* カーソルの文字の色保管変数 */
    timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settime(timer, 50);

    /* sht_mouse */
    sht_mouse = sheet_alloc(shtctl);    /* マウスのシートの確保 */
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); /* マウスのバッファのセット */
    init_mouse_cursor8(buf_mouse, 99);  /* マウスを描画 */
    mx = (binfo->scrnx - 16) / 2; /* 画面中央になるx座標 */
    my = (binfo->scrny - 28 -16) / 2; /* 画面中央になるy座標 */

    sheet_slide(sht_back,  0,  0);    /* 背景の位置指定 */
    sheet_slide(sht_cons, 32,  4);
    sheet_slide(sht_win,  64, 56);
    sheet_slide(sht_mouse, mx, my); /* マウスの位置指定 */
    sheet_updown(sht_back,  0); /* 背景の下敷きの高さを指定 */
    sheet_updown(sht_cons,  1);
    sheet_updown(sht_win,   2); /* ウィンドウの下敷きの高さを指定 */
    sheet_updown(sht_mouse, 3); /* マウスの下敷きの高さを指定 */

    /* 最初にキーボードの状態との食い違いが無いように、設定しておくこと */
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);
    /* キーボードとマウスの許可は上に移動 */
    for (;;) {
        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
            /* キーボードコントローラに送るデータがあれば、送る */
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }
        io_cli();  /* 外部割り込み禁止（割り込み処理中の割り込み対策） */
        if (fifo32_status(&fifo) == 0) { /* どちらからもデータが来てないことの確認 */
            task_sleep(task_a);
            io_sti();  /* 外部割り込みの許可 */
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if (256 <= i && i <= 511) {  /* もしキーボードの方のデータが来ていたら */
                if (i < 0x80 + 256) {   /* キーコードを文字コードに変換 */
                    if (key_shift == 0) {
                        s[0] = keytable0[i - 256];  /* ShiftキーOFF */
                    } else {
                        s[0] = keytable1[i - 256];  /* ShiftキーON */
                    }
                } else {
                    s[0] = 0;   /* 変換できなさそうなキーコードの場合 */
                }
                if ('A' <= s[0] && s[0] <= 'Z') {    /* 入力文字がアルファベット */
                    if (((key_leds & 4) == 0 && key_shift== 0) ||   /* shiftとcapslock */
                            ((key_leds & 4) != 0 && key_shift != 0)) {
                        s[0] += 0x20; /* 大文字を小文字に変換 */
                    }
                }
                if (s[0] != 0) {    /* 通常文字 */
                    if (key_to == 0) {  /* タスクAへ */
                        if (cursor_x < 128) {
                            /* １文字表示してから、カーソルを1つ進める */
                            s[1] = 0;
                            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
                            cursor_x += 8;
                        }
                    } else {    /* コンソールへバッファ経由で */
                        fifo32_put(&task_cons->fifo, s[0] + 256);  /* 先に文字コードの値で渡してる */
                    }
                }
                if ( i == 256 + 0x0e) { /* バックスペース */
                    if (key_to == 0) {  /* タスクAへ */
                        if (cursor_x > 8) {
                            /* カーソルをスペースで消してから、カーソルを1つ戻す */
                            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
                            cursor_x -= 8;
                        }
                    } else {    /* コンソールへ */
                        fifo32_put(&task_cons->fifo, 8 + 256);  /* コンソール用タスクにバッファ経由で */
                    }
                }
                if (i == 256 + 0x1c) {  /* Enter */
                    if (key_to != 0) {  /* コンソールへ */
                        fifo32_put(&task_cons->fifo, 10 + 256); /* コンソールタスクへ */
                    }
                }
                if (i == 256 + 0x0f) {  /* Tab */
                    if (key_to == 0) {
                        key_to = 1; /* コンソールタスク */
                        make_wtitle8(buf_win,  sht_win->bxsize,  "task_a",  0); /* actは1で標準色、0で灰色 */
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
                        cursor_c = -1; /* カーソルを消す */
                        boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);   /* カーソルを消す再描画 */
                        fifo32_put(&task_cons->fifo, 2); /* コンソールのカーソルON */
                    } else {
                        key_to = 0; /* タスクA */
                        make_wtitle8(buf_win,  sht_win->bxsize,  "task_a",  1);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
                        cursor_c = COL8_000000; /* カーソルを出す */
                        fifo32_put(&task_cons->fifo, 3); /* コンソールのカーソルOFF */
                    }
                    sheet_refresh(sht_win,  0, 0, sht_win->bxsize,  21);
                    sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
                }
                if (i == 256 + 0x2a) {  /* 左シフト ON */
                    key_shift |= 1;
                }
                if (i == 256 + 0x36) {  /* 右シフト ON */
                    key_shift |= 2;
                }
                if (i == 256 + 0xaa) {  /* 左シフト OFF */
                    key_shift &= ~1;    /* ~(NOT) */
                }
                if (i == 256 + 0xb6) {  /* 右シフト OFF */
                    key_shift &= ~2;
                }
                if (i == 256 + 0x3a) {  /* CapsLock */
                    key_leds ^= 4;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x45) {  /* NumLock */
                    key_leds ^= 2;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x46) {  /* ScrollLock */
                    key_leds ^= 1;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0xfa) {  /* キーボードがデータを無事に受け取った */
                    keycmd_wait = -1;
                }
                if (i == 256 + 0xfe) {  /* キーボードがデータを無事に受け取れなかった */
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }
                /* カーソルの再表示 */
                if (cursor_c >= 0) {
                    boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                }
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            } else if (512 <= i && i <= 767) {         /* もしもマウスのデータが来ていたら */
                      /* IFに1をセット（外部割り込みの許可） */
                if (mouse_decode(&mdec, i - 512) != 0) {
                    /* マウスカーソルの移動 */
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 1) {
                        mx = binfo->scrnx - 1;
                    }
                    if (my > binfo->scrny - 1) {
                        my = binfo->scrny - 1;
                    }
                    sheet_slide(sht_mouse, mx, my);
                    if ((mdec.btn & 0x01) != 0) {
                        /* 左ボタンを押していたら、sht_winを動かす */
                        sheet_slide(sht_win, mx - 80, my - 8);
                    }
                }
            } else if (i <= 1) {    /* カーソル用タイマ */
                if (i != 0) {
                    timer_init(timer, &fifo, 0); /* 次は0を */
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                }else {
                    timer_init(timer, &fifo, 1); /* 次は1を */
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                }
                timer_settime(timer, 50);
                if (cursor_c >= 0) {
                    boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                    sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
                }
            }
        }
    }
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act)
{
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
    make_wtitle8(buf, xsize, title, act);
    return;
}
/* タイトルの描画を専用の関数として独立 */
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act)
{
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"
    };
    int x, y;
    char c, tc, tbc;
    if (act != 0) {
        tc = COL8_FFFFFF;
        tbc = COL8_000084;
    } else {
        tc = COL8_C6C6C6;
        tbc = COL8_848484;
    }
    boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
    putfonts8_asc(buf, xsize, 24, 4, tc, title);
    for (y = 0; y < 14; y++) {
        for (x = 0; x < 16; x++) {
            c = closebtn[y][x];
            if (c == '@') {
                c = COL8_000000;
            } else if (c == '$') {
                c = COL8_848484;
            } else if (c == 'Q') {
                c = COL8_C6C6C6;
            } else {
                c = COL8_FFFFFF;
            }
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}

void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l)   
/* x y: 表示位置の座標, c: 文字色, b: 背景色, s: 文字列, l: 文字長*/
{
    boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
    putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
    sheet_refresh(sht, x, y, x + l * 8, y + 16);
    return;
}

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
{
    int x1 = x0 + sx, y1 = y0 + sy;
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, c,           x0 - 1, y0 - 1, x1 + 0, y1 + 0);
    return;
}

void console_task(struct SHEET *sheet, unsigned int memtotal)
{
    struct TIMER *timer;    /* タイマ */
    struct TASK *task = task_now(); /* スリープから復帰する際にtask_nowから参照する */
    int i, fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1;
    char s[30], cmdline[30];    /* cmdline：キー入力保管バッファ */
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    int x, y;
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);

    fifo32_init(&task->fifo, 128, fifobuf, task); /* バッファの初期化 */
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);   /* カーソル用（0.5秒） */

    /* プロンプトの表示 */
    putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

    for (;;) {
        io_cli();   /* IF=0(割り込み無効) */
        if (fifo32_status(&task->fifo) == 0) {    /* バッファにデータが来ていなければ */
            task_sleep(task);   /* スリープ */
            io_sti();   /* IF=1 */
        } else {
            i = fifo32_get(&task->fifo);  /* バッファからデータを1バイト引き出す */
            io_sti();   /* IF=1 */
            if (i <= 1) {   /* カーソル用タイマ */
                if (i != 0) {
                    timer_init(timer, &task->fifo, 0); /* 次は0を */
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1); /* 次は1を */
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                }
                timer_settime(timer, 50);
            }
            if (i == 2) {   /* カーソルON */
                cursor_c = COL8_FFFFFF;
            }
            if (i == 3) {   /* カーソルOFF */
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
                cursor_c = -1;
            }
            if (256 <= i && i <= 511) { /* キーボードデータ（タスクA経由） */
                if (i == 8 + 256) {
                    /* バックスペース */
                    if (cursor_x > 16) {
                        /* カーソルをスペースで消してから、カーソルを1つ戻す */
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                        cursor_x -= 8;
                    }
                } else if (i == 10 + 256) {
                    /* Enter */
                    /* カーソルをスペースで消してから改行する */
                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                    cmdline[cursor_x / 8 - 2] = 0;
                    cursor_y = cons_newline(cursor_y, sheet);
                    /* コマンド実行 */
                    if (strcmp(cmdline, "mem") == 0) {
                        /* memコマンド */
                        sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                        sprintf(s, "free %dKB", memman_total(memman) / 1024);
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (strcmp(cmdline, "cls") == 0) {
                        /* clsコマンド */
                        for (y = 28; y < 28 + 128; y++) {   /* 横方向の画素 */
                            for (x = 8; x < 8 + 240; x++) { /* 縦方向の画素 */
                                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                            }
                        }
                        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
                        cursor_y = 28;
                    } else if (strcmp(cmdline, "ls") == 0) {
                        /* dirコマンド */
                        for (x = 0; x < 224; x++) {
                            if (finfo[x].name[0] == 0x00) { /* fileデータがこれ以上ない */
                                break;
                            }
                            if (finfo[x].name[0] != 0xe5) {
                                if ((finfo[x].type & 0x18) == 0) {
                                    sprintf(s, "filename.ext   %7d", finfo[x].size);
                                    for (y = 0; y < 8; y++) {
                                        s[y] = finfo[x].name[y];
                                    }
                                    s[ 9] = finfo[x].ext[0];
                                    s[10] = finfo[x].ext[1];
                                    s[11] = finfo[x].ext[2];
                                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                            }
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (cmdline[0] != 0) {
                        /* コマンドではなく、さらに空行でもない */
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Bad command", 12);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    /* プロンプト表示 */
                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">", 1);
                    cursor_x = 16;
                } else {
                    /* 一般文字 */
                    if (cursor_x < 240) {
                        /* 一文字表示をしてから、カーソルを1つ進める */
                        s[0] = i - 256;
                        s[1] = 0;
                        cmdline[cursor_x / 8 - 2] = i - 256;
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                        cursor_x += 8;
                    }
                }
            }
            /* カーソルの再表示 */
            if (cursor_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
            }
            sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
        }
    }
}

int cons_newline(int cursor_y, struct SHEET *sheet)
{
    int x, y;
    if (cursor_y < 28 + 112) {
        cursor_y += 16; /* 次の行へ */
    } else {
        /* スクロール */
        for (y = 28; y < 28 + 112; y++) {
            for (x = 8; x < 8 + 240; x++) {
                sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
            }
        }
        for (y = 28 + 112; y < 28 + 128; y++) {
            for (x = 8; x < 8 + 240; x++) {
                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
            }
        }
        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    }
    return cursor_y;
}
