/* bootpackのメイン */

#include "bootpack.h"
#include <stdio.h>

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);  /* 文字出力をまとめる */
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    struct FIFO32 fifo;
    char s[40];
    int fifobuf[128];
    struct TIMER *timer, *timer2, *timer3;
    int mx, my, i, cursor_x, cursor_c;
    unsigned int memtotal;
    struct MOUSE_DEC mdec; /* マウスのデータを構造体で管理（タグ：mdec） */
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHTCTL *shtctl;  /* シートの管理 */
    struct SHEET *sht_back, *sht_mouse, *sht_win;     /* 壁紙、マウス、ウィンドウの下敷き */
    unsigned char *buf_back, buf_mouse[256], *buf_win;    /* 壁紙とマウス、ウィンドウのバッファ */
    static char keytable[0x54] = {
        0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.' 
    };

    init_gdtidt(); /* GDT、IDTの初期化 */
    init_pic(); /* PICの初期か */
    io_sti();   /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */
    fifo32_init(&fifo, 128, fifobuf);
    init_pit(); /* PITの初期化 */
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);
    io_out8(PIC0_IMR, 0xf8); /* PITとPIC1とキーボードを許可（1111_1000） */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可（1110_1111） */

    timer = timer_alloc();
    timer_init(timer, &fifo, 10);
    timer_settime(timer, 1000);
    timer2 = timer_alloc();
    timer_init(timer2, &fifo, 3);
    timer_settime(timer2, 300);
    timer3 = timer_alloc();
    timer_init(timer3, &fifo, 1);
    timer_settime(timer3, 50);

    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 -0x0009efff */
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();/* 以下重ね合わせ処理 */
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);  /* シート全体の情報の初期化 */
    sht_back  = sheet_alloc(shtctl);    /* 背景のシートの確保 */
    sht_mouse = sheet_alloc(shtctl);    /* マウスのシートの確保 */
    sht_win   = sheet_alloc(shtctl);    /* ウィンドウ用のメモリ確保 */
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);     /* 背景バッファのメモリ確保 */
    buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 *52); /* ウィンドウバッファのメモリ確保 */
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 透明色なし */
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); /* マウスのバッファのセット */
    sheet_setbuf(sht_win, buf_win, 160, 52, -1); /* 透明色なし */
    init_screen8(buf_back, binfo->scrnx, binfo->scrny); /* 背景を描画 */
    init_mouse_cursor8(buf_mouse, 99);  /* マウスを描画 */
    make_window8(buf_win, 160, 52, "window"); /* カウンターウィンドウ */
    make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);    /* テキストボックス */
    cursor_x = 8;               /* カーソルの表示位置保管変数 */
    cursor_c = COL8_FFFFFF;      /* カーソルの文字の色保管変数 */
    sheet_slide(sht_back, 0, 0);    /* 背景の位置指定 */
    mx = (binfo->scrnx - 16) / 2; /* 画面中央になるx座標 */
    my = (binfo->scrny - 28 -16) / 2; /* 画面中央になるy座標 */
    sheet_slide(sht_mouse, mx, my); /* マウスの位置指定 */
    sheet_slide(sht_win, 80, 72);
    sheet_updown(sht_back,  0); /* 背景の下敷きの高さを指定 */
    sheet_updown(sht_win,   1); /* ウィンドウの下敷きの高さを指定 */
    sheet_updown(sht_mouse, 2); /* マウスの下敷きの高さを指定 */
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
    sprintf(s, "memory %dMB   free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024); 
    putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);
    /* キーボードとマウスの許可は上に移動 */
    /* 理想的な割り込み処理 */

    for (;;) {
        io_cli();                                                     /* 外部割り込み禁止（割り込み処理中の割り込み対策） */
        if (fifo32_status(&fifo) == 0) { /* どちらからもデータが来てないことの確認 */
            io_stihlt();  /* 外部割り込みの許可と、CPU停止命令一時削除 */
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if (256 <= i && i <= 511) {  /* もしキーボードの方のデータが来ていたら */
                sprintf(s, "%02X", i - 256);
                putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
                if (i < 256 + 0x54){
                    if (keytable[i - 256] != 0 && cursor_x < 144) {   /* 通常文字 */
                        /* 一文字を表示してから、カーソルを一つ進める */
                        s[0] = keytable[i - 256];   /* i = 0x1e + 256 */
                        s[1] = 0;   /*  */
                        putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_C6C6C6, s, 1);
                        cursor_x += 8;
                    }
                }
                if ( i == 256 + 0x0e && cursor_x > 8) { /* バックスペース */
                    /* カーソルをスペースで消してから、カーソルを1つ戻す */
                    putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
                    cursor_x -= 8;
                }
                /* カーソルの再表示 */
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            } else if (512 <= i && i <= 767) {         /* もしもマウスのデータが来ていたら */
                      /* IFに1をセット（外部割り込みの許可） */
                if (mouse_decode(&mdec, i - 512) != 0) {
                    /* データが3バイト揃ったので表示 */
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y); /* それぞれのデータを文字列sに格納 */
                    if ((mdec.btn & 0x01) != 0) {                /* 左ボタンのデータ以外のデータをマスクして判定 */
                        s[1] = 'L';                              /* lを大文字に */
                    }
                    if ((mdec.btn & 0x02) != 0) {                /* 右ボタン以外のデータをマスクして判定 */
                        s[3] = 'R';                              /* rを大文字に */
                    }
                    if ((mdec.btn & 0x04) != 0) {                /* 中ボタン以外のデータをマスクして判定 */
                        s[2] = 'C';                              /* cを大文字に */
                    }
                    putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);
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
                    sprintf(s, "(%3d, %3d)", mx, my);
                    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
                    sheet_slide(sht_mouse, mx, my);
                    if ((mdec.btn & 0x01) != 0) {
                        /* 左ボタンを押していたら、sht_winを動かす */
                        sheet_slide(sht_win, mx - 80, my - 8);
                    }
                }
            } else if (i == 10) {   /* 10秒タイマ */
                putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]", 7);
            } else if (i == 3) {    /* 3秒タイマ */
                putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]", 6);
            } else if (i <= 1) {    /* カーソル用タイマ */
                if (i != 0) {
                    timer_init(timer3, &fifo, 0); /* 次は0を */
                    cursor_c = COL8_000000;
                }else {
                    timer_init(timer3, &fifo, 1); /* 次は1を */
                    cursor_c = COL8_FFFFFF;
                }
                timer_settime(timer3, 50);
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
            }
        }
    }
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title)
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
    char c;
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_000084, 3,         3,         xsize - 4, 20       );
    boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
    putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
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
