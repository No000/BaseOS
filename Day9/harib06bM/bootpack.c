/* bootpackのメイン */

#include "bootpack.h"
#include <stdio.h>
/* 新しく追加する関数 */
unsigned int memtest(unsigned int start, unsigned int end);
unsigned int memtest_sub(unsigned int start, unsigned int end);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128]; /* 各種バッファ */
    int mx, my, i;
    struct MOUSE_DEC mdec; /* マウスのデータを構造体で管理（タグ：mdec） */

    init_gdtidt(); /* GDT、IDTの初期化 */
    init_pic(); /* PICの初期か */
    io_sti();   /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */

    fifo8_init(&keyfifo, 32, keybuf);           /* キーボードバッファの初期化 */
    fifo8_init(&mousefifo, 128, mousebuf);      /* マウスバッファの初期化 */
    io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可（1111_10001） */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可（1110_1111） */

    init_keyboard();                            /* キーボードの初期化 */
    enable_mouse(&mdec);  /* マウス有効化処理 */

    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2; /* 画面中央になるx座標 */
    my = (binfo->scrny - 28 -16) / 2; /* 画面中央になるy座標 */
    init_mouse_cursor8(mcursor, COL8_008484);   /* マウス描写 */
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my); /* マウスの位置が画面にデバッグ */
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
    /* キーボードとマウスの許可は上に移動 */
    /* 理想的な割り込み処理 */

    i = memtest(0x00400000, 0xbfffffff) / (1024 * 1024);
    sprintf(s, "memory %dMB", i);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    for (;;) {
        io_cli();                                                     /* 外部割り込み禁止（割り込み処理中の割り込み対策） */
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) { /* どちらからもデータが来てないことの確認 */
            io_stihlt();                                              /* 外部割り込みの許可と、CPU停止命令(割り込みの終了) */
        } else {
            if (fifo8_status(&keyfifo) != 0) {  /* もしキーボードの方のデータが来ていたら */
                i = fifo8_get(&keyfifo); /* キーコードのアドレスを変数iに格納 */
                io_sti(); /* IFに1をセット（外部割り込みの許可） */ 
                sprintf(s, "%02X", i); /* メモリのデータの参照 */
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31); /* 画面のリセット */
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s); /* 文字の表示 */
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
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);       /* ボックスの描写 */
                    putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);                   /* 文字の描写 */
                    /* マウスカーソルの移動 */
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);         /* マウスを消す */
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx -16) {
                        mx = binfo->scrnx -16;
                    }
                    if (my > binfo->scrny -16) {
                        my = binfo->scrny -16;
                    }
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
                    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
                    putblock8_8(binfo->vram, binfo->scrnx, 16,16, mx, my, mcursor, 16);
                }
            }
        }
    }
}

#define EFLAGS_AC_BIT       0x00040000
#define CR0_CACHE_DISABLE   0x60000000

unsigned int memtest(unsigned int start, unsigned int end)
{
    char flg486 = 0;            /* 486かのフラグをオフとして定義 */
    unsigned int eflg, cr0, i;  /* eflg:EFLAGのビットパターン格納用, cr0:cr0のビットパターン格納用, i:メモリの容量用 */

    /* 386か、486以降なのかの確認(EFLAGの18ビットの判定) */
    eflg = io_load_eflags(); /* EFLAGの内容を変数eflgに格納 */
    eflg |= EFLAGS_AC_BIT; /* AC-bit = 1（ACフラグをON） */
    io_store_eflags(eflg); /* EFLAGに読み込み */
    eflg = io_load_eflags(); /* 再度読み出し */
    if ((eflg & EFLAGS_AC_BIT) != 0) {  /* 386ではAC=1にしても自動で0に戻ってしまう（386か486の判定） */
        flg486 = 1;                     /* 486であると判定しフラグをON */
    }
    eflg &= ~EFLAGS_AC_BIT;    /* AC-bit = 0(AND演算子とビット反転を使って全ビットを0にリセット) */
    io_store_eflags(eflg);      /* EFLAGに読み込み */

    if (flg486 != 0) {          /* もし486なら */
        cr0 = load_cr0();       /* cr0の現在のビットパターンをロードして */
        cr0 |= CR0_CACHE_DISABLE;   /* キャッシュの許可をセット */
        store_cr0(cr0);         /* cr0にストア（というより戻す？）する */
    }

    i = memtest_sub(start, end);    /* メモリ容量の測定 */

    if (flg486 != 0) {              /* もし486なら */
        cr0 = load_cr0();           /* cr0のビットパターンを読み出し */
        cr0 &= ~CR0_CACHE_DISABLE;   /* キャッシュの許可をセット */
        store_cr0(cr0);             /* CR0に戻す */
    }

    return i;                       /* メモリの容量を返す */
}
/* アドレスstartからendまででのメモリ容量の判定 */
unsigned int memtest_sub(unsigned int start, unsigned int end)
{
    unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
    for (i = start; i <= end; i += 0x1000) {
        p = (unsigned int *) (i + 0xffc);
        old = *p;           /* いじる前の値を覚えておく */
        *p = pat0;          /* 試しに書いてみる */
        *p ^= 0xffffffff;   /* そしてそれをメモリ上で反転してみる */
        if (*p != pat1) {   /* 反転結果になってなければ */
not_memory:
            *p = old;       /* データを元に戻す */
            break;          /* ブレイク */
        }
        *p ^= 0xffffffff;   /* もう一度反転してみる */
        if (*p != pat0) {   /* 元に戻ったか？ */
            goto not_memory;    /* 指定ラベルに飛ぶ */
        }
        *p = old;           /* いじった値を元に戻す */
    }
    return i;               /* 測定した値を返す */
}
