/* タイマ関係 */

#include "bootpack.h"

#define PIT_CTRL  0x0043  /* ポート：コントロールレジスタを指定 */
#define PIT_CNT0  0x0040  /* ポート：カウンタ0を指定 */

/* IRQ0の割り込み処理変更 */
void init_pit(void)
{
  io_out8(PIT_CTRL, 0x34);  
  /* 2進数カウント、レートジェネレータ、16bitリードロード（下位8bit、上位8bitの順）、カウンタ0 */
  io_out8(PIT_CNT0, 0x9c);  /* 割り込み周期の下位8bit */
  io_out8(PIT_CNT0, 0x2e);  /* 割り込み周期の上位8bit */
  return;
}

/* IDT20の処理設定 */
void inthandler20(int *esp)
{
  io_out8(PIC0_OCW2, 0x60); /* IRQ-00受付完了をPICに通知 */
  /* とりあえず何もしない */
  return;
}
