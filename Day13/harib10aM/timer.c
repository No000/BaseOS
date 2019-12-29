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
  timerctl.next = 0xffffffff; /* 最初の作動中タイマがないので */
  timerctl.using = 0;
  for (i = 0; i < MAX_TIMER; i++) {
    timerctl.timers0[i].flags = 0;  /* 未使用 */
  }
  return;
}

struct TIMER *timer_alloc(void)
{
  int i;
  for (i = 0; i < MAX_TIMER; i++) {
    if ( timerctl.timers0[i].flags == 0) {
      timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
      return &timerctl.timers0[i];
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

/* timersへの登録を追加 */
void timer_settime(struct TIMER *timer, unsigned int timeout)
{
  int e, i, j;
  timer->timeout = timeout + timerctl.count;
  timer->flags = TIMER_FLAGS_USING;
  e = io_load_eflags();
  io_cli();
  /* どこに入れればいいかを探す */
  for (i = 0; i < timerctl.using; i++) {
    if (timerctl.timers[i]->timeout >= timer->timeout) {
      break;
    }
  }
  /* うしろにずらす */
  for (j = timerctl.using; j > i; j--) {
    timerctl.timers[j] = timerctl.timers[j - 1];
  }
  timerctl.using++;
  /* 空いた隙間に入れる */
  timerctl.timers[i] = timer;
  timerctl.next = timerctl.timers[0]->timeout;
  io_store_eflags(e);
  return;
}

/* IDT20の処理設定 */
void inthandler20(int *esp)
{
  int i, j; /*  */
  io_out8(PIC0_OCW2, 0x60); /* IRQ-00受付完了をPICに通知 */
  timerctl.count++;   /* 指定に従いカウントする */
  if (timerctl.next > timerctl.count) {
    return; /* まだ次の時刻になっていないので、もうおしまい */
  }
  for (i = 0;i < timerctl.using; i++) {
    /* timers のタイマは全て動作中のものなので、flagsを確認しない */
    if (timerctl.timers[i]->timeout > timerctl.count) {
      break;
    }
    /* タイムアウト */
    timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
    fifo8_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
  }
  /* ちょうど一個のタイマがタイムアウトした。残りをずらす。 */
  timerctl.using -= i;
  for (j = 0; j < timerctl.using; j++) {
    timerctl.timers[j] = timerctl.timers[i + j];
  }
  if (timerctl.using > 0) {
    timerctl.next = timerctl.timers[0]->timeout;
  } else {
    timerctl.next = 0xffffffff;
  }
  return;
}
