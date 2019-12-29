/* マウス関係 */

#include "bootpack.h"

struct FIFO32 *mousefifo;
int mousedata0;

void inthandler2c(int *esp)
/* PS/2マウスからの割り込み */
{
  int data;
  io_out8(PIC1_OCW2, 0x64);       /* IRQ-12受付完了をPIC1に伝達（スレーブへ） */
  io_out8(PIC0_OCW2, 0x62);       /* IRQ-02受付完了をPIC0に伝達 （マスタへ）*/
  data = io_in8(PORT_KEYDAT);     /* キーボードと同じ */
  fifo32_put(mousefifo, data + mousedata0);    /* キーボードと同じ */
  return;
}

#define KEYCMD_SENDTO_MOUSE     0xd4        /* マウスへのデータ送信コマンド */
#define MOUSECMD_ENABLE         0xf4        /* マウス有効化命令 */

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec)
{
    /* 書き込み先のFIFOバッファを記憶 */
    mousefifo = fifo;
    mousedata0 = data0;
    /* マウス有効化 */
    wait_KBC_sendready();                           /* 送信可能かの確認 */
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);      /* KBCへマウスへのデータ送信依頼 */
    wait_KBC_sendready();                           /* 送信可能かの確認 */
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);          /* マウスへ有効化命令を伝達 */
    mdec->phase = 0;                                /* マウスの0xfaを待っている段階 */
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
