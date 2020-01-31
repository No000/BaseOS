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
  task->priority = 2; /* 初期値は0.02秒 */
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

void task_run(struct TASK *task, int priority)  /* タスクを1つ追加する関数 */
{
  if (priority > 0) { /* 優先度0は優先度を変えたくない時に使用 */
    task->priority = priority;  /* 優先度の変更 */
  }
  if (task->flags != 2) {
    task->flags = 2; /* 動作中マーク */
    taskctl->tasks[taskctl->running] = task;
    taskctl->running++;
  }
  return;
}

void task_switch(void)
{
  struct TASK *task;
  taskctl->now++;
  if (taskctl->now == taskctl->running) {
    taskctl->now = 0;
  }
  task = taskctl->tasks[taskctl->now];
  timer_settime(task_timer, task->priority);  /* ここで優先度の時間設定がされる */
  if (taskctl->running >= 2) {  /* タスクが2つであることの確認（CPUがエラーを起こす） */
    farjmp(0, task->sel);
  }
  return;
}

void task_sleep(struct TASK *task)
{
  int i;
  char ts = 0;  /* タスクスイッチするタイミングを最後にするためのフラグ */
  if (task->flags == 2) {   /* 指定タスクがもし起きていたら */
    if (task == taskctl->tasks[taskctl->now]) {
      ts = 1; /* 自分自身を寝かせるので、あとでタスクスイッチする */
    }
    /* taskがどこにいるかを探す */
    for (i = 0; i < taskctl->running; i++) {
      if (taskctl->tasks[i] == task) {
        /* ここにいた */
        break;
      }
    }
    taskctl->running--; /* 動作するタスクの記録用メタデータを減らす */
    if (i < taskctl->now) { /* 現在動作しているメタデータをずらす */
      taskctl->now--; /* ずれるので、これもあわせておく */
    }
    /* ずらし */
    for (; i < taskctl->running; i++) {
      taskctl->tasks[i] = taskctl->tasks[i + 1];
    }
    task->flags = 1; /* 動作をしていない状態 */
    if (ts != 0) {
      /* タスクスイッチをする */
      if (taskctl->now >= taskctl->running) {
        /* nowがおかしな値になっていたら、修正する */
        taskctl->now = 0;
      }
      farjmp(0, taskctl->tasks[taskctl->now]->sel); /* タスクスイッチ */
    }
  }
  return;
}
