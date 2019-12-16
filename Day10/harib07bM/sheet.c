/* マウスやウィンドウの傘ね合わせ処理 */  

#include "bootpack.h"

#define SHEET_USE   1

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
/* 構造体が戻り値 */
{
  struct SHTCTL *ctl; /* 構造体定義 */
  int i;  /* ループカウンタ */
  ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof (struct SHTCTL)); /* メモリを確保する（sizeofで構造体の必要なメモリを計算） */
  /* (struct SHTCTL *)はポインタに型が違うものを代入しようとしていると警告されることへの対策 */
  if (ctl == 0) {     /* メモリが確保できなかった場合 */
    goto err;         /* エラー処理を行う */
  }
  ctl->vram  = vram;  /* vramのアドレスを渡す */
  ctl->xsize = xsize; /* 画面サイズxの情報を渡す */
  ctl->ysize = ysize; /* 画面サイズyの情報を渡す */
  ctl->top   = -1;  /* シートは1枚もない */
  for (i = 0; i < MAX_SHEETS; i++) {  /* シートの最大数まで処理を行う */
    ctl->sheets0[i].flags = 0; /* 未使用マークを添付 */
  }
err:  /* エラーならここへショートカット */
  return ctl; /* 構造体を返す */
}

/* 新規の未使用シートを確保する関数 */
struct SHEET *sheet_alloc(struct SHTCTL *ctl)
/* 構造体を返す */
{
  struct SHEET *sht;    /* 構造体の定義 */
  int i;                /* カウンタの定義 */
  for (i = 0; i < MAX_SHEETS; i++) {  /* 下敷きすべてを */
    if (ctl->sheets0[i].flags == 0) {  /* もし下敷きが未使用であれば */
      sht = &ctl->sheets0[i];  /* シートの番地を構造体のアドレスにする */
      sht->flags = SHEET_USE; /* 使用中マーク */
      sht->height = -1; /* 非表示中 */
      return sht;   /* 情報のある場所のアドレスを返す */
    }
  }
  return 0; /* 全てのシートが使用中だった */
}

/* 要求された下敷き等の情報を構造体の管理下におく */
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
  sht->buf = buf;           /* バッファを設定する */
  sht->bxsize = xsize;      /* xの大きさ */
  sht->bysize = ysize;      /* yの大きさ */
  sht->col_inv = col_inv;   /* 透明色の設定 */
  return;
}

/* 下敷きの高さを設定する関数 */
void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height)
{
  int h, old = sht->height; /* 設定前の高さを記憶する */

  /* 指定が低すぎや高すぎだったら、修正を行う */
  if (height > ctl->top + 1) {  /* 下敷きの高さが足りなくなったら */
    height = ctl->top + 1;      /* 下敷きの高さを増やす */
  }
  if (height < -1) {            /* 下敷きの高さが-1より低くなったら */
    height = -1;                /* 下敷きの高さを-1に合わせる */
  }
  sht->height = height; /* 高さを設定 */

  /* 以下は主にsheets[]の並び替え */
  if (old > height) {       /* 以前よりも低くなる */
    if (height >= 0) {      /* 高さが0以上であれば */
      /* 間のものを引き上げる */
      for (h = old; h > height; h--) {    /* 設定前の高さから、設定後の高さまで */
        ctl->sheets[h] = ctl->sheets[h - 1];  /* シートの高さを一つ下げて */
        ctl->sheets[h]->height = h;     /* 現在のシートを入れる */
      }
      ctl->sheets[height] = sht;
    } else {  /* 非表示化 */
      if (ctl->top > old) {
        /* 上になっているものをおろす */
        for (h = old; h < ctl->top; h++) {
          ctl->sheets[h] = ctl->sheets[h + 1];
          ctl->sheets[h]->height = h;
        }
      }
      ctl->top--; /* 表示中の下敷きが一つ減るので一番上の高さが減る */
    }
    sheet_refresh(ctl); /* 新しい下敷きの情報に沿って画面を描き直す */
  } else if (old < height) {
    if (old >= 0) {
      /* 間のものを押し上げる */
      for (h = old; h< height; h++) {
        ctl->sheets[h] = ctl->sheets[h + 1];
        ctl->sheets[h]->height = h;
      }
      ctl->sheets[height] = sht;
    } else {  /* 非表示状態から表示状態へ */
      /* 上になるものを持ち上げる */
      for (h = ctl->top; h >= height; h--) {
        ctl->sheets[h + 1] = ctl->sheets[h];
        ctl->sheets[h + 1]->height = h + 1;
      }
      ctl->sheets[height] = sht;
      ctl->top++; /* 表示中の下敷きが1つ増えるので、一番上の高さが増える */
    }
    sheet_refresh(ctl); /* 新しい下敷きの情報に沿って画面を描きなおす */
  }
  return;
}

void sheet_refresh(struct SHTCTL *ctl)
{
  int h, bx, by, vx, vy;
  unsigned char *buf, c, *vram = ctl->vram;
  struct SHEET *sht;
  for (h = 0; h <= ctl->top; h++) {
    sht = ctl->sheets[h];
    buf = sht->buf;
    for (by = 0; by < sht->bysize; by++) {
      vy = sht->vy0 + by;
      for (bx = 0; bx < sht->bxsize; bx++) {
        vx = sht->vx0 + bx;
        c = buf[by * sht->bxsize + bx];
        if (c != sht->col_inv) {
          vram[vy * ctl->xsize + vx] = c;
        }
      }
    }
  }
  return;
}

void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0)
{
  sht->vx0 = vx0;
  sht->vy0 = vy0;
  if (sht->height >= 0) { /* もしも表示中なら */
    sheet_refresh(ctl); /* 新しい下敷きの情報に沿って画面を描き直す */
  }
  return;
}

void sheet_free(struct SHTCTL *ctl, struct SHEET *sht)
{
  if (sht->height >= 0) {
    sheet_updown(ctl, sht, -1);  /* 表示中ならまず非表示にする */
  }
  sht->flags = 0; /* 未使用マーク */
  return;
}
