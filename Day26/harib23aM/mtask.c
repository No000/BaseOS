/* マルチタスク関係 */

#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_now(void) /* 動作中のstruct TASKのアドレスを返す関数 */
{
  struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];  /* now_lvは現在動作中のタスク */
  return tl->tasks[tl->now];
}

void task_add(struct TASK *task)  /* TASKLEVELにタスクを追加する関数 */
{
  struct TASKLEVEL *tl = &taskctl->level[task->level];
  tl->tasks[tl->running] = task;
  tl->running++;
  task->flags = 2; /* 動作中 */
  return;
}

void task_remove(struct TASK *task) /* TASKLEVELからタスクを1つ除く関数 */
{
  int i;
  struct TASKLEVEL *tl = &taskctl->level[task->level];

  /* taskがどこにいるかを探す */
  for (i = 0; i < tl->running; i++) {
    if (tl->tasks[i] == task) {
      /* ここにいた */
      break;
    }
  }

  tl->running--;
  if (i < tl->now) {
    tl->now--; /* ずれるので、これも合わせておく */
  }
  if (tl->now >= tl->running) {
    /* nowがおかしな値になっていたら、修正する */
    tl->now = 0;
  }
  task->flags = 1; /* スリープ中 */

  /* すらし */
  for (; i < tl->running; i++) {
    tl->tasks[i] = tl->tasks[i + 1];
  }

  return;
}

void task_switchsub(void)   /* タスクスイッチの際に、次のレベルを決める関数 */
{
  int i;
  /* 一番上のレベルを探す */
  for (i = 0; i < MAX_TASKLEVELS; i++) {
    if (taskctl->level[i].running > 0) {
      break; /* 見つかった */
    }
  }
  taskctl->now_lv = i;    /* 現在のレベルを記録 */
  taskctl->lv_change = 0; /* レベル変えなくていいわ的な */
  return;
}

void task_idle(void)  /* 番兵用 */
{
  for (;;) {
    io_hlt();
  }
}

struct TASK *task_init(struct MEMMAN *memman) /* タスク割り当てプログラム(タスクの管理下におかれる) */
{
  int i;
  struct TASK *task, *idle;
  struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

  taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
  for (i = 0; i < MAX_TASKS;i++) {
    taskctl->tasks0[i].flags = 0;
    taskctl->tasks0[i].sel   = (TASK_GDT0 + i) * 8;
    set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss,AR_TSS32); /* タスクをGDTに登録 */
  }
  for (i = 0; i < MAX_TASKLEVELS; i++) {
    taskctl->level[i].running = 0;
    taskctl->level[i].now = 0;
  }
  
  task = task_alloc();
  task->flags = 2;  /* 動作中のマーク */
  task->priority = 2; /* 初期値は0.02秒 */
  task->level = 0; /* 最高レベル */
  task_add(task);
  task_switchsub(); /* レベル設定 */
  load_tr(task->sel);
  task_timer = timer_alloc();
  timer_settime(task_timer, task->priority);
  /* 以下番兵のアイドルタスク */
  idle = task_alloc();
  idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
  idle->tss.eip = (int) &task_idle; /* タスクのセット */
  idle->tss.es = 1 * 8;
  idle->tss.cs = 2 * 8;
  idle->tss.ss = 1 * 8;
  idle->tss.ds = 1 * 8;
  idle->tss.fs = 1 * 8;
  idle->tss.gs = 1 * 8;
  task_run(idle, MAX_TASKLEVELS - 1, 1); /* レベル；9, 優先度：0.1秒 */

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
      task->tss.ss0 = 0;
      return task;
    }
  }
  return 0; /* もう全部使用中 */
}

void task_run(struct TASK *task, int level, int priority)  /* タスクを1つ追加する関数(levelも指定できるようになった) */
{
  if (level < 0) {
    level = task->level; /* レベルを変更しない */
  }
  if (priority > 0) { /* 優先度0は優先度を変えたくない時に使用 */
    task->priority = priority;  /* 優先度の変更 */
  }

  if (task->flags == 2 && task->level != level) { /* 動作中のレベルの変更 */
    task_remove(task); /* これを実行するとflagsは１になるので下のifも実行される */
  }
  if (task->flags !=2) {
    /* スリープから起こされる場合 */
    task->level = level;
    task_add(task);
  }

  taskctl->lv_change = 1; /* 次回タスクスイッチのときにレベルを見直す */
  return;
}

void task_sleep(struct TASK *task)  /* task_removeのおかげで短くなった */
{
  struct TASK *now_task;
  if (task->flags == 2) {
    /* 動作中だったら */
    now_task = task_now();
    task_remove(task); /* これを実行するとflagsは1になる */
    if (task == now_task) {
      /* 自分自身のスリープだったので、タスクスイッチが必要 */
      task_switchsub();
      now_task = task_now(); /* 設定後での、「現在のタスク」を教えてもらう */
      farjmp(0, now_task->sel);
    }
  }
  return;
}

void task_switch(void)
{
  struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
  struct TASK *new_task, *now_task = tl->tasks[tl->now];
  tl->now++;
  if (tl->now == tl->running) {
    tl->now = 0;
  }
  if (taskctl->lv_change != 0) {
    task_switchsub();
    tl = &taskctl->level[taskctl->now_lv];
  }
  new_task = tl->tasks[tl->now];
  timer_settime(task_timer, new_task->priority);  /* ここで優先度の時間設定がされる */
  if (new_task != now_task) {  /* タスクが2つであることの確認（CPUがエラーを起こす） */
    farjmp(0, new_task->sel);
  }
  return;
}
