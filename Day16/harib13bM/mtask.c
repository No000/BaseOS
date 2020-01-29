/* マルチタスク関係 */

#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman) /* タスク割り当てプログラム(タスクの管理下におかれる) */
{
  int i;
  struct TASK *task;
  struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
  taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
  for (i = 0; i < MAX_TASKS;i++) {
    taskctl->tasks0[i].flags = 0;
    taskctl->tasks0[i].sel   = (TASK_GDT0 + i) * 8;
    set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss,AR_TSS32); /* タスクをGDTに登録 */
  }
  task = task_alloc();
  task->flags = 2;  /* 動作中のマーク */
  taskctl->running = 1;
  taskctl->now = 0;
  taskctl->tasks[0] = task;
  load_tr(task->sel);
  task_timer = timer_alloc();
  timer_settime(task_timer, 2);
  return task;  /* アドレスが帰ってくる */
}

struct TASK *task_alloc(void) /* タスク構造体の確保 */
{
  int i;
  struct TASK *task;
  for (i = 0; i < MAX_TASKS; i++) {
    if (taskctl->tasks0[i].flags == 0) {  /* 初期化されてないことの確認 */
      task = &taskctl->tasks0[i];
      task->flags = 1; /* 使用中のマーク */
      task->tss.eflags = 0x00000202; /* IF = 1;(STI後のフラグ) */
      task->tss.eax = 0; /* とりあえず0にしておくことにする */
      task->tss.ecx = 0;
      task->tss.edx = 0;
      task->tss.ebx = 0;
      task->tss.ebp = 0;
      task->tss.esi = 0;
      task->tss.edi = 0;
      task->tss.es = 0;
      task->tss.ds = 0;
      task->tss.fs = 0;
      task->tss.gs = 0;
      task->tss.ldtr = 0;
      task->tss.iomap = 0x40000000;
      return task;
    }
  }
  return 0; /* もう全部使用中 */
}

void task_run(struct TASK *task)  /* タスクを1つ追加する関数 */
{
  task->flags = 2; /* 動作中マーク */
  taskctl->tasks[taskctl->running] = task;
  taskctl->running++;
  return;
}

void task_switch(void)
{
  timer_settime(task_timer, 2);
  if (taskctl->running >= 2) {  /* タスクが1つの時の処理 */
    taskctl->now++;
    if (taskctl->now == taskctl->running) { /* 一番後ろだったら一番前にする */
      taskctl->now = 0;
    }
    farjmp(0, taskctl->tasks[taskctl->now]->sel);
  }
  return;
}
