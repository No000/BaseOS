/* bootpackのメイン */

#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo, mousefifo; /* キーボードとマウスのバッファ */
void enable_mouse(void);
void init_keyboard(void);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128]; /* 各種バッファ */
    int mx, my, i;
    unsigned char mouse_dbuf[3], mouse_phase; /* マウスの3バイトのデータ処理用 */

    init_gdtidt(); /* GDT、IDTの初期化 */
    init_pic(); /* PICの初期か */
    io_sti();   /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);
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

    enable_mouse();  /* マウス有効化処理 */
    mouse_phase = 0; /* マウスから0xfaが来た状態を記録 */

    for (;;) {
        io_cli(); /* 外部割り込み禁止（割り込み処理中の割り込み対策） */
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) { /* どちらからもデータが来てないことの確認 */
            io_stihlt(); /* 外部割り込みの許可と、CPU停止命令(割り込みの終了) */
        } else {
            if (fifo8_status(&keyfifo) != 0) {      /* もしキーボードの方のデータが来ていたら */
                i = fifo8_get(&keyfifo); /* キーコードのアドレスを変数iに格納 */
                io_sti(); /* IFに1をセット（外部割り込みの許可） */ 
                sprintf(s, "%02X", i); /* メモリのデータの参照 */
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31); /* 画面のリセット */
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s); /* 文字の表示 */
            } else if (fifo8_status(&mousefifo) != 0) {         /* もしもマウスのデータが来ていたら */
                i = fifo8_get(&mousefifo);                      /* マウスのデータをバッファから取得し */
                io_sti();                                       /* IFに1をセット（外部割り込みの許可） */
                if (mouse_phase == 0) {                         /* 0xfaが届くまで待機 */
                    /* マウスの0xfaを待っている段階 */
                    if (i == 0xfa) {                            /* 0xfaが届いたら */
                        mouse_phase = 1;                        /* マウスのデータのステータスを１に変更 */
                    }
                } else if (mouse_phase == 1) {                  /* マウスのデータのステータスが1ならば（マウスボタン） */
                    /* マウスの1バイト目を待っている段階 */
                    mouse_dbuf[0] = i;                          /* マウスのデータバッファに1バイト目を格納 */
                    mouse_phase = 2;                            /* マウスのデータのステータスを2に変更 */
                } else if (mouse_phase == 2) {                  /* マウスのデータのステータスが2ならば（x座標） */
                    /* マウスの2バイト目を待っている段階 */
                    mouse_dbuf[1] = i;                          /* マウスのデータバッファに2バイト目を代入 */
                    mouse_phase = 3;                            /* マウスのデータのステータスを3に変更 */
                }else if (mouse_phase == 3) {                   /* マウスのデータのステータスが3ならば（y座標） */
                    /* マウスの3バイト目を待っている段階 */
                    mouse_dbuf[2] = i;                          /* マウスのデータバッファに3バイト目を代入 */
                    mouse_phase = 1;                            /* マウスのデータのステータスを1に変更（完了） */
                    /* データが3バイト揃ったので表示 */
                    sprintf(s, "%02X %02X %02X", mouse_dbuf[0], mouse_dbuf[1], mouse_dbuf[2]);          /* それぞれのデータを文字列sに格納 */
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 8 * 8 - 1, 31);       /* ボックスの描写 */
                    putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);                   /* 文字の描写 */
                }
            }
        }
    }
}
/* KBC(キーボードコントローラ) */
#define PORT_KEYDAT             0x0060      /* KBCデータポート/キーボードデータ */
#define PORT_KEYSTA             0x0064      /* KBCの装置番号 */
#define PORT_KEYCMD             0x0064      /* KBCへのモード設定コマンド */
#define KEYSTA_SEND_NOTREADY    0x02        /* 準備完了確認用数値（AND型） */
#define KEYCMD_WRITE_MODE       0x60        /* KBCへのモード指定コマンド（ステータスレジスタ） */
#define KBC_MODE                0x47        /* マウスモードの指定 */

void wait_KBC_sendready(void)
{
    /* キーボードコントローラーがデータを送信可能になるのを待つ */
    for (;;) {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {        /* ANDでデータ送信可能状態かの確認 */
            break;
        }
    }
    return;
}

void init_keyboard(void)
{
    /* キーボードコントローラーの初期化 */
    wait_KBC_sendready();                       /* 送信可能かの確認 */
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);    /* モード設定コマンド */
    wait_KBC_sendready();                       /* 送信可能かの確認 */
    io_out8(PORT_KEYDAT, KBC_MODE);             /* マウスモードを指定 */
    return;
}

#define KEYCMD_SENDTO_MOUSE     0xd4        /* マウスへのデータ送信コマンド */
#define MOUSECMD_ENABLE         0xf4        /* マウス有効化命令 */

void enable_mouse(void)
{
    /* マウス有効化 */
    wait_KBC_sendready();                           /* 送信可能かの確認 */
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);      /* KBCへマウスへのデータ送信依頼 */
    wait_KBC_sendready();                           /* 送信可能かの確認 */
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);          /* マウスへ有効化命令を伝達 */
    return;                                         /* うまくいくとACK（0xfa）が送信されてくる */
}