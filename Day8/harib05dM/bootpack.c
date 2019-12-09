/* bootpackのメイン */

#include "bootpack.h"
#include <stdio.h>

struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};

extern struct FIFO8 keyfifo, mousefifo; /* キーボードとマウスのバッファ */
void enable_mouse(struct MOUSE_DEC *mdec);
void init_keyboard(void);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

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

    enable_mouse(&mdec);  /* マウス有効化処理 */

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
                    mx += mdec.x;                  /* マウスのx座標にx座標の移動量を追加 */
                    my += mdec.y;                  /* マウスのy座標にy座標の移動量を追加 */
                    if (mx < 0) {                  /* 左端に来たら */
                        mx = 0;                    /* x座標を維持し続ける */
                    }
                    if (my < 0) {                  /* 上端に来たら */
                        my = 0;                    /* y座標を維持し続ける */
                    }
                    if (mx > binfo->scrnx -16) {   /* 一番右の長さからマウスの大きさ分を引いた点に来たら */
                        mx = binfo->scrnx -16;     /* x座標を維持し続ける */
                    }
                    if (my > binfo->scrny -16) {   /* 一番下の位置からマウスの大きさ分を引いた点に来たら */
                        my = binfo->scrny -16;     /* y座標を維持し続ける */
                    }
                    sprintf(s, "(%3d, %3d)", mx, my);                                   /* 指定アドレスのデータを文字列にしsに格納 */
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);     /* 背景と同じ色の描写 */
                    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);     /* 座標の描写 */
                    putblock8_8(binfo->vram, binfo->scrnx, 16,16, mx, my, mcursor, 16); /* マウスの描写 */
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

void enable_mouse(struct MOUSE_DEC *mdec)
{
    /* マウス有効化 */
    wait_KBC_sendready();                           /* 送信可能かの確認 */
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);      /* KBCへマウスへのデータ送信依頼 */
    wait_KBC_sendready();                           /* 送信可能かの確認 */
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);          /* マウスへ有効化命令を伝達 */
    return;                                         /* うまくいくとACK（0xfa）が送信されてくる */
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
    if (mdec->phase == 0) {                     /* 0xfaを受け取ってない段階 */
        /* マウスの0xfaを待っている段階 */
        if (dat == 0xfa) {                      /* 0xfaを受け取ると */
            mdec->phase = 1;                    /* 次の段階にステータスを更新 */
        }
        return 0;
    }
    if (mdec->phase == 1) {                     /* ステータスがマウスからのデータ受信が可能になっていたら */
        /* マウスの1バイト目を待っている段階 */
        if ((dat & 0xc8) == 0x08) {                 /* 0~3桁目と8~F桁目以外をマスクし、ほしいデータがやってきているかを確認 */
            /* 正しい1バイト目だった */
            mdec->buf[0] = dat;                    /* バッファにデータを格納し */
            mdec->phase = 2;                        /* 次の段階にステータスを更新 */
        }
        return 0;
    }
    if (mdec->phase == 2) {                     /* 1バイト目のデータをバッファに格納しているならば */
        /* マウスの2バイト目を待っている段階 */
        mdec->buf[1] = dat;                     /* バッファに2バイト目のデータを格納し */
        mdec->phase = 3;                        /* 次の段階にステータスを更新 */
        return 0;
    }
    if (mdec->phase == 3) {                     /* 2バイト目までのデータを受け取っているならば */
        /* ますの3バイト目を待っている段階 */
        mdec->buf[2] = dat;                     /* バッファに3バイト目のデータを格納し */
        mdec->phase = 1;                        /* 次の段階にステータスをリセット */
        mdec->btn = mdec->buf[0] & 0x07;        /* 下位3ビットがボタンのデータなのでマスクする */
        mdec->x = mdec->buf[1];                 /* バッファからメンバxへx座標を格納 */
        mdec->y = mdec->buf[2];                 /* バッファからメンバyへy座標を格納 */
        if ((mdec->buf[0] & 0x10) != 0) {       /* マウスのステータスの1バイト目のデータを使って */
            mdec->x |= 0xffffff00;              /* 8ビットより上を1にする */
        }
        if ((mdec->buf[0] & 0x20) != 0) {       /* マウスのステータスの1バイト目のデータを使って */
            mdec->y |= 0xffffff00;              /* 8ビットより上を1にする */
        }
        mdec->y = - mdec->y;                    /* マウスではy方向の符号が画面と反対なので */
        return 1;
    }
    return -1;      /* ここまで来ることはないはず */
}
