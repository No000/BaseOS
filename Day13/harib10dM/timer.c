/* タイマ関係 */

#include "bootpack.h"

#define PIT_CTRL  0x0043  /* ポート：コントロールレジスタを指定 */
#define PIT_CNT0  0x0040  /* ポート：カウンタ0を指定 */

struct TIMERCTL timerctl; /* タイマー管理用 */

#define TIMER_FLAGS_ALLOC   1   /* 確保した状態 */
#define TIMER_FLAGS_USING   2   /* タイマ作動中 */

/* IRQ0の割り込み処理変更 */
void init_pit(void)
{
  int i;
  io_out8(PIT_CTRL, 0x34);  
  /* 2進数カウント、レートジェネレータ、16bitリードロード（下位8bit、上位8bitの順）、カウンタ0 */
  io_out8(PIT_CNT0, 0x9c);  /* 割り込み周期の下位8bit */
  io_out8(PIT_CNT0, 0x2e);  /* 割り込み周期の上位8bit */
  timerctl.count = 0;
  for (i = 0; i < MAX_TIMER; i++) {
    timerctl.timer[i].flags = 0;
  }
  return;
}

struct TIMER *timer_alloc(void)
{
  int i;
  for (i = 0; i < MAX_TIMER; i++) {
    if ( timerctl.timer[i].flags == 0) {
      timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
      return &timerctl.timer[i];
    }
  }
  return 0; /* 見つからなかった */
}

void timer_free(struct TIMER *timer)
{
  timer->flags = 0;  /* 未使用 */
  return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data)
{
  timer->fifo = fifo;
  timer->data = data;
  return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
  timer->timeout = timeout;
  timer->flags = TIMER_FLAGS_USING;
  return;
}

/* IDT20の処理設定 */
void inthandler20(int *esp)
{
  int i;
  io_out8(PIC0_OCW2, 0x60); /* IRQ-00受付完了をPICに通知 */
  timerctl.count++;   /* 指定に従いカウントする */
  for (i = 0; i < MAX_TIMER; i++) {
    if (timerctl.timer[i].flags == TIMER_FLAGS_USING) {
      timerctl.timer[i].timeout--;
      if (timerctl.timer[i].timeout == 0) {
        timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
        fifo8_put(timerctl.timer[i].fifo, timerctl.timer[i].data);
      }
    }
  }
  return;
}
