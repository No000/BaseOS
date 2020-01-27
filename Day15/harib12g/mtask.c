/* マルチタスク関係 */

#include "bootpack.h"

struct TIMER *mt_timer;
int mt_tr;    /* TRレジスタの代用 */

void mt_init(void)  /* mt_timerとmt_trの初期化 */
{
  mt_timer = timer_alloc();
  /* timer_initは必要ないのでやらない */
  timer_settime(mt_timer, 2);
  mt_tr = 3 * 8;  /* 最初のセグメントをHarimainに指定 */
  return;
}

void mt_taskswitch(void)
{
  if (mt_tr == 3 * 8) {   /* 次のタスクのセグメント指定 */
    mt_tr = 4 * 8;
  } else {
    mt_tr = 3 * 8;
  }
  timer_settime(mt_timer, 2); /* 0.02秒後にセットしなおし */
  farjmp(0, mt_tr); /* 指定したセグメントにタスクスイッチ */
  return;
}