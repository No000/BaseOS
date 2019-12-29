/* キーボード関係 */

#include "bootpack.h"

struct FIFO8 keyfifo;   /* 構造体をkeyinfoと名付けている */
int keydate0;

void inthandler21(int *esp)
/* PS/2キーボードからの割り込み */
{
  int data;
  io_out8(PIC0_OCW2, 0x61); /*  */
  data = io_in8(PORT_KEYDAT);
  fifo8_put(&keyfifo, data);
  return;
}

#define PORT_KEYSTA             0x0064      /* KBCの装置番号 */
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

void init_keyboard(struct FIFO32 *fifo, int data0)
{
    /* 書き込み先のFIFOバッファを記憶 */
    keyfifo = fifo;
    keydate0 = data0;
    /* キーボードコントローラーの初期化 */
    wait_KBC_sendready();                       /* 送信可能かの確認 */
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);    /* モード設定コマンド */
    wait_KBC_sendready();                       /* 送信可能かの確認 */
    io_out8(PORT_KEYDAT, KBC_MODE);             /* マウスモードを指定 */
    return;
}
