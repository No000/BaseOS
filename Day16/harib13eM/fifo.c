/* FIFOライブラリ */

#include "bootpack.h"

#define FLAGS_OVERRUN   0x0001

void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task)
/* FIFOバッファの初期化 */
{
  fifo->size = size;
  fifo->buf  = buf;
  fifo->free = size; /* 空き */
  fifo->flags = 0;
  fifo->p = 0; /* 書き込み位置 */
  fifo->q = 0; /* 読み込み位置 */
  fifo->task = task; /* データが入った時に起こすタスク */
  return;
}

int fifo32_put(struct FIFO32 *fifo, int data)
/* FIFOへデータを送り込んで蓄える */
{
  if (fifo->free == 0) {  /* もし空きがなければ */
    /* 空きがなくてあふれた */
    fifo->flags |= FLAGS_OVERRUN;
    return -1;      /* -1を返す */
  }
  fifo->buf[fifo->p] = data;
  fifo->p++;                      /* 書き込み位置を進める */
  if (fifo->p == fifo->size) {    /* 書き込み位置が最後に来たら */
    fifo->p = 0;                  /* 0に移動 */
  }
  fifo->free--;                   /* バッファの空きが減った */
  if (fifo->task != 0) {
    if (fifo->task->flags != 2) { /* タスクが寝ていたら */
      task_run(fifo->task, 0); /* 起こしてあげる（優先度は変更しないため0） */
    }
  }
  return 0;                       /* 0を返す */
}

int fifo32_get(struct FIFO32 *fifo)
/* FIFOからデータを1つとって来る */
{
  int data;
  if (fifo->free == fifo->size) {
    /* バッファが空っぽの時は、とりあえず-1が返される */
    return -1;
  }
  data = fifo->buf[fifo->q];   /* バッファ上の読み込み位置のデータをdataに格納 */
  fifo->q++;                   /* 読み込み位置を1つ進める */
  if (fifo->q == fifo->size) { /* バッファの最後にたどり着いたら */
    fifo->q = 0;               /* バッファの始めに移動する */
  }
  fifo->free++;                /* バッファのデータを一つ渡したことをfreeに空きが1つ増えたとして記録 */
  return data;                 /* int型でdataを返す */
}

int fifo32_status(struct FIFO32 *fifo)
/* どのくらいデータがたまっているかを報告する */
{
  return fifo->size - fifo->free;   /* バッファの大きさから空きの大きさを引く */
}
