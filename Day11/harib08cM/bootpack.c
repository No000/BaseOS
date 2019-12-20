/* bootpackのメイン */

#include "bootpack.h"
#include <stdio.h>

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], keybuf[32], mousebuf[128]; /* 各種バッファ */
    int mx, my, i;
    unsigned int memtotal;
    struct MOUSE_DEC mdec; /* マウスのデータを構造体で管理（タグ：mdec） */
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHTCTL *shtctl;  /* シートの管理 */
    struct SHEET *sht_back, *sht_mouse;     /* 壁紙の下敷きとマウスの下敷き */
    unsigned char *buf_back, buf_mouse[256];    /* 壁紙とマウスのバッファ */

    init_gdtidt(); /* GDT、IDTの初期化 */
    init_pic(); /* PICの初期か */
    io_sti();   /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */
    fifo8_init(&keyfifo, 32, keybuf);           /* キーボードバッファの初期化 */
    fifo8_init(&mousefifo, 128, mousebuf);      /* マウスバッファの初期化 */
    io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可（1111_10001） */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可（1110_1111） */

    init_keyboard();                            /* キーボードの初期化 */
    enable_mouse(&mdec);  /* マウス有効化処理 */
    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 -0x0009efff */
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();/* 以下重ね合わせ処理 */
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);  /* シート全体の情報の初期化 */
    sht_back  = sheet_alloc(shtctl);    /* 背景のシートの確保 */
    sht_mouse = sheet_alloc(shtctl);    /* マウスのシートの確保 */
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);     /* 背景バッファのメモリ確保 */
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 透明色なし */
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99); /* マウスのバッファのセット */
    init_screen8(buf_back, binfo->scrnx, binfo->scrny); /* 背景を描画 */
    init_mouse_cursor8(buf_mouse, 99);  /* マウスを描画 */
    sheet_slide(sht_back, 0, 0);    /* 背景の位置指定 */
    mx = (binfo->scrnx - 16) / 2; /* 画面中央になるx座標 */
    my = (binfo->scrny - 28 -16) / 2; /* 画面中央になるy座標 */
    sheet_slide(sht_mouse, mx, my); /* マウスの位置指定 */
    sheet_updown(sht_back,  0); /* 背景の下敷きの高さを指定 */
    sheet_updown(sht_mouse, 1); /* マウスの下敷きの高さを指定 */
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
    sprintf(s, "memory %dMB   free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024); 
    putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
    sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);  /* 画面の描画しなおし */
    /* キーボードとマウスの許可は上に移動 */
    /* 理想的な割り込み処理 */

    for (;;) {
        io_cli();                                                     /* 外部割り込み禁止（割り込み処理中の割り込み対策） */
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) { /* どちらからもデータが来てないことの確認 */
            io_stihlt();                                              /* 外部割り込みの許可と、CPU停止命令(割り込みの終了) */
        } else {
            if (fifo8_status(&keyfifo) != 0) {  /* もしキーボードの方のデータが来ていたら */
                i = fifo8_get(&keyfifo); /* キーコードのアドレスを変数iに格納 */
                io_sti(); /* IFに1をセット（外部割り込みの許可） */ 
                sprintf(s, "%02X", i); /* メモリのデータの参照 */
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31); /* 画面のリセット */
                putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s); /* 文字の表示 */
                sheet_refresh(sht_back, 0, 16, 16, 32);
            } else if (fifo8_status(&mousefifo) != 0) {         /* もしもマウスのデータが来ていたら */
                i = fifo8_get(&mousefifo);                      /* マウスのデータをバッファから取得し */
                io_sti();                                       /* IFに1をセット（外部割り込みの許可） */
                if (mouse_decode(&mdec, i) != 0) {
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
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);       /* ボックスの描写 */
                    putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);                   /* 文字の描写 */
                    sheet_refresh(sht_back, 32, 16, 32 + 15 * 8, 32);
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
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
                    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
                    sheet_refresh(sht_back, 0, 0, 80, 16);
                    sheet_slide(sht_mouse, mx, my);
                }
            }
        }
    }
}
