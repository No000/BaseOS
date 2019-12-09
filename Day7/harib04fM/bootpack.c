/* bootpackのメイン */

#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo;
void enable_mouse(void);
void init_keyboard(void);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32];
    int mx, my, i;

    init_gdtidt(); /* GDT、IDTの初期化 */
    init_pic(); /* PICの初期か */
    io_sti();   /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */

    fifo8_init(&keyfifo, 32, keybuf);
    io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可（1111_10001） */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可（1110_1111） */

    init_keyboard();

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

    enable_mouse();

    for (;;) {
        io_cli(); /* 外部割り込み禁止（割り込み処理中の割り込み対策） */
        if (fifo8_status(&keyfifo) == 0) { /* バッファが空かの確認 */
            io_stihlt(); /* 外部割り込みの許可と、CPU停止命令(割り込みの終了) */
        } else {
            i = fifo8_get(&keyfifo); /* キーコードのアドレスを変数iに格納 */
            io_sti(); /* IFに1をセット（外部割り込みの許可） */ 
            sprintf(s, "%02X", i); /* メモリのデータの参照 */
            boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31); /* 画面のリセット */
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s); /* 文字の表示 */
        }
    }
}

#define PORT_KEYDAT             0x0060      /* KBCデータ出力 */
#define PORT_KEYSTA             0x0064      /* 装置番号 */
#define PORT_KEYCMD             0x0064      /* KBC制御コマンドを出力 */
#define KEYSTA_SEND_NOTREADY    0x02        /* 下位から2ビット目が0になる判定に使用 */
#define KEYCMD_WRITE_MODE       0x60        /* モードを設定するコマンド */
#define KBC_MODE                0x47        /* ステータスレジスタのマウス使用モード指定 */

void wait_KBC_sendready(void)
{
    /* キーボードコントローラーがデータを送信可能になるのを待つ */
    for (;;) {      /* 無限ループで待機 */
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {        /* コマンドが受け付けられるようになるとbreak */
            break;
        }
    }
    return;
}

void init_keyboard(void)
{
    /* キーボードコントローラーの初期化 */
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);    /* キーボードコントローラにモード選択コマンドを送信 */
    wait_KBC_sendready();                       /* キーボードが命令を受けつけられるかの確認 */
    io_out8(PORT_KEYDAT, KBC_MODE);             /* キーボードコントローラにマウス使用モードを指定 */
    return;
}

#define KEYCMD_SENDTO_MOUSE     0xd4            /* マウスへのデータ転送コマンド */
#define MOUSECMD_ENABLE         0xf4            /* マウス有効化命令 */

void enable_mouse(void)
{
    /* マウス有効化 */
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);      /* キーボード制御回路にマウスデータ転送の依頼 */
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);          /* マウス有効化命令 */
    return; /* うまくいくとACK（0xfa）が送信されてくる */
}