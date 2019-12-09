/* bootpackのメイン */

#include "bootpack.h"
#include <stdio.h>

extern struct KEYBUF keybuf;

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256];
    int mx, my, i, j;

    init_gdtidt(); /* GDT、IDTの初期化 */
    init_pic(); /* PICの初期か */
    io_sti();   /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */

    io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可（1111_10001） */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可（1110_1111） */

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

    for (;;) {
        io_cli(); /* 外部割り込み禁止（割り込み処理中の割り込み対策） */
        if (keybuf.next == 0) { /* バッファが空かの確認 */
            io_stihlt(); /* 外部割り込みの許可と、CPU停止命令 */
        } else {
            i = keybuf.data[0]; /* キーコードのアドレスを変数iに格納 */
            keybuf.next--; /* nextをデクリメント */
            for (j = 0; j < keybuf.next; j++) { /* FIFO型に従い繰り返し処理で格納場所をずらす */
                keybuf.data[j] = keybuf.data[j + 1]; /* 1つずらす */
            }
            io_sti(); /* IFに1をセット（外部割り込みの許可） */ 
            sprintf(s, "%02X", i); /* メモリのデータの参照 */
            boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31); /* ボックスの描写 */
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s); /* 文字の表示 */
        }
    }
}
