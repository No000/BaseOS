/* bootpackのメイン */

#include "bootpack.h"
#include <stdio.h>

#define KEYCMD_LED      0xed

int keywin_off(struct SHEET *key_win, struct SHEET *sht_win, int cur_c, int cur_x);
int keywin_on(struct SHEET *key_win, struct SHEET *sht_win, int cur_c);

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
    unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons[2];
    struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons[2];    /* ウィンドウを4つ用意 */
    struct TASK *task_a, *task_cons[2], *task;    /* タスクAとコンソールタスク,タスク */
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
    int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1; /* key_to：ウィンドウ, key_shift：シフト, key_les：SL・NL・CL */
    int j, x, y, mmx = -1, mmy = -1;    /* ウィンドウupdpwn */
    struct SHEET *sht = 0, *key_win;

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
    *((int *) 0x0fe4) = (int) shtctl;

    /* sht_back */
    sht_back  = sheet_alloc(shtctl);    /* 背景のシートの確保 */
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);     /* 背景バッファのメモリ確保 */
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 透明色なし */
    init_screen8(buf_back, binfo->scrnx, binfo->scrny); /* 背景を描画 */

    /* sht_cons */
    for (i = 0; i < 2; i++) {
        sht_cons[i] = sheet_alloc(shtctl);
        buf_cons[i] = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
        sheet_setbuf(sht_cons[i], buf_cons[i], 256, 165, -1); /* 透明色なし */
        make_window8(buf_cons[i], 256, 165, "console", 0);
        make_textbox8(sht_cons[i], 8, 28, 240, 128, COL8_000000);  /* 黒のテキストボックス */
        task_cons[i] = task_alloc();
        task_cons[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;   /* スタックを増やすために6->12 */
        task_cons[i]->tss.eip = (int) &console_task;   /* console_taskを登録 */
        task_cons[i]->tss.es = 1 * 8;
        task_cons[i]->tss.cs = 2 * 8;
        task_cons[i]->tss.ss = 1 * 8;
        task_cons[i]->tss.ds = 1 * 8;
        task_cons[i]->tss.fs = 1 * 8;
        task_cons[i]->tss.gs = 1 * 8;
        *((int *) (task_cons[i]->tss.esp + 4)) = (int) sht_cons[i];
        *((int *) (task_cons[i]->tss.esp + 8)) = memtotal; /* Harimainから渡してもらうためのスタック */
        task_run(task_cons[i], 2, 2); /* level=2, priority=2 */
        sht_cons[i]->task = task_cons[i];
        sht_cons[i]->flags |= 0x20; /* カーソルあり */
    }

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
    sheet_slide(sht_cons[1], 56,  6);   /* コンス１ */
    sheet_slide(sht_cons[0],  8,  2);   /* コンス0 */
    sheet_slide(sht_win,  64, 56);
    sheet_slide(sht_mouse, mx, my); /* マウスの位置指定 */
    sheet_updown(sht_back,     0); /* 背景の下敷きの高さを指定 */
    sheet_updown(sht_cons[1],  1);  /* コンス１ */
    sheet_updown(sht_cons[0],  2);  /* コンス0 */
    sheet_updown(sht_win,      3); /* ウィンドウの下敷きの高さを指定 */
    sheet_updown(sht_mouse,    4); /* マウスの下敷きの高さを指定 */
    key_win = sht_win;

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
            if (key_win->flags == 0) {  /* ウィンドウが閉じられた */
                key_win = shtctl->sheets[shtctl->top - 1];
                cursor_c = keywin_on(key_win, sht_win, cursor_c);
            }
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
                    if (key_win == sht_win) {  /* タスクAへ */
                        if (cursor_x < 128) {
                            /* １文字表示してから、カーソルを1つ進める */
                            s[1] = 0;
                            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
                            cursor_x += 8;
                        }
                    } else {    /* コンソールへバッファ経由で */
                        fifo32_put(&key_win->task->fifo, s[0] + 256);  /* 先に文字コードの値で渡してる */
                    }
                }
                if (i == 256 + 0x0e) { /* バックスペース */
                    if (key_win == sht_win) {  /* タスクAへ */
                        if (cursor_x > 8) {
                            /* カーソルをスペースで消してから、カーソルを1つ戻す */
                            putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
                            cursor_x -= 8;
                        }
                    } else {    /* コンソールへ */
                        fifo32_put(&key_win->task->fifo, 8 + 256);  /* コンソール用タスクにバッファ経由で */
                    }
                }
                if (i == 256 + 0x1c) {  /* Enter */
                    if (key_win != sht_win) {  /* コンソールへ */
                        fifo32_put(&key_win->task->fifo, 10 + 256); /* コンソールタスクへ */
                    }
                }
                if (i == 256 + 0x0f) {  /* Tab */
                    cursor_c = keywin_off(key_win, sht_win, cursor_c, cursor_x);
                    j = key_win->height - 1;
                    if (j == 0) {
                        j = shtctl->top - 1;
                    }
                    key_win = shtctl->sheets[j];
                    cursor_c = keywin_on(key_win, sht_win, cursor_c);
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
                if (i == 256 + 0x3b && key_shift != 0) {  /* Shift+F1(タスクがない場合は動作しない) */
                    task = key_win->task;
                    if (task != 0 && task->tss.ss0 != 0) {
                        cons_putstr0(task->cons, "\nBreak(key) :\n");
                        io_cli(); /* 強制終了中にタスクが変わると困るから */
                        task->tss.eax = (int) &(task->tss.esp0);
                        task->tss.eip = (int) asm_end_app;
                        io_sti();
                    }
                }
                if (i == 256 + 0x57 && shtctl->top > 2) {   /* F11+下敷きが2枚以上 */
                    sheet_updown(shtctl->sheets[1], shtctl->top - 1);   /* arg1：背景の一個上の下敷き、arg2：マウスの一個下の下敷き */
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
                        if (mmx < 0) {
                            /* 通常モードの場合 */
                            /* 上の下敷き空順番にマウスが指している下敷きを探す */
                            for (j = shtctl->top - 1; j > 0; j--) {
                                sht = shtctl->sheets[j];
                                x = mx - sht->vx0;
                                y = my - sht->vy0;
                                if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
                                    if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
                                        sheet_updown(sht, shtctl->top - 1);
                                        if (sht != key_win) {   /* クリックに入力切り替えの紐づけ */
                                            cursor_c = keywin_off(key_win, sht_win, cursor_c, cursor_x);
                                            key_win = sht;
                                            cursor_c = keywin_on(key_win, sht_win, cursor_c);
                                        }
                                        if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
                                            mmx = mx;   /* ウィンドウの移動モードへ */
                                            mmy = my;
                                        }
                                        if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) { /* ×ボタンの座標 */
                                            /* 「x」ボタンをクリック */
                                            if ((sht->flags & 0x10) != 0) {   /* タスクがあれば */
                                                task = sht->task;    
                                                cons_putstr0(task->cons, "\nBreak(mouse) :\n");
                                                io_cli();   /* 強制終了処理中にタスクが変わると困るから */
                                                task->tss.eax = (int) &(task->tss.esp0);
                                                task->tss.eip = (int) asm_end_app;
                                                io_sti();
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        } else {
                            /* ウィンドウ移動モードの場合 */
                            x = mx - mmx;   /* マウスの移動量を計算 */
                            y = my - mmy;
                            sheet_slide(sht, sht->vx0 + x, sht->vy0 + y);
                            mmx = mx;   /* 移動後の座標を更新 */
                            mmy = my;
                        }
                    } else {
                        /* 左ボタンを押していない */
                        mmx = -1;    /* 通常モードへ */
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

int keywin_off(struct SHEET *key_win, struct SHEET *sht_win, int cur_c, int cur_x)
{
    change_wtitle8(key_win, 0);
    if (key_win == sht_win) {
        cur_c = -1; /* カーソルを消す */
        boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cur_x, 28, cur_x + 7, 43);
    } else {
        if ((key_win->flags & 0x20) != 0) {
            fifo32_put(&key_win->task->fifo, 3);   /* コンソールのカーソルOFF */
        }
    }
    return cur_c;
}

int keywin_on(struct SHEET *key_win, struct SHEET *sht_win, int cur_c)
{
    change_wtitle8(key_win, 1);
    if (key_win == sht_win) {
        cur_c = COL8_000000; /* カーソルを出す */
    } else {
        if ((key_win->flags & 0x20) != 0) {
            fifo32_put(&key_win->task->fifo, 2); /* コンソールのカーソルON */
        }
    }
    return cur_c;
}
