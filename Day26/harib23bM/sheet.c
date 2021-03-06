/* マウスやウィンドウの傘ね合わせ処理 */  

#include "bootpack.h"

#define SHEET_USE   1
/* 構造体が戻り値 */
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{   /* (struct SHTCTL *)はポインタに型が違うものを代入しようとしていると警告されることへの対策 */
  struct SHTCTL *ctl; /* 構造体定義 */
  int i;  /* ループカウンタ */
  ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof (struct SHTCTL)); /* メモリを確保する（sizeofで構造体の必要なメモリを計算） */
  if (ctl == 0) {     /* メモリが確保できなかった場合 */
    goto err;         /* 例外処理 */
  }
  ctl->map = (unsigned char *) memman_alloc_4k(memman, xsize * ysize); /* VRAMと同じ多きさのメモリ確保 */
  if (ctl->map == 0) { /* 確保をできなかったら */
    memman_free_4k(memman, (int) ctl, sizeof(struct SHTCTL)); /* メモリを解放する */
    goto err; /* 例外処理 */
  }
  ctl->vram  = vram;  /* vramのアドレスを渡す */
  ctl->xsize = xsize; /* 画面サイズxの情報を渡す */
  ctl->ysize = ysize; /* 画面サイズyの情報を渡す */
  ctl->top   = -1;  /* シートは1枚もない */
  for (i = 0; i < MAX_SHEETS; i++) {  /* シートの最大数まで処理を行う */
    ctl->sheets0[i].flags = 0; /* 未使用マークを添付 */
    ctl->sheets0[i].ctl = ctl; /* 所属を記録 */
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
      sht->task = 0;  /* 自動で閉じる機能を使わない */
      return sht;   /* 情報のある場所の構図体のアドレスを返す */
    }
  }
  return 0; /* 全てのシートが使用中だった */
}

/* 要求された下敷き等の情報を構造体のメンバに入れる */
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
  sht->buf = buf;           /* バッファを設定する */
  sht->bxsize = xsize;      /* xの大きさ */
  sht->bysize = ysize;      /* yの大きさ */
  sht->col_inv = col_inv;   /* 透明色の設定 */
  return;
}

/* マップ生成関数：refreshsubの色番号をIDに変えただけ */
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0)
{
  int h, bx, by, vx, vy, bx0, by0, bx1, by1, sid4, *p;
  unsigned char *buf, sid, *map = ctl->map; /* sid：sheetid */
  struct SHEET *sht;
  if (vx0 < 0) { vx0 = 0; }
  if (vy0 < 0) { vy0 = 0; }
  if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
  if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
  for (h = h0; h <= ctl->top; h++) {
    sht = ctl->sheets[h];
    sid = sht - ctl->sheets0; /* 番地を引き算することで、下敷き番号として使用する */
    buf = sht->buf;
    bx0 = vx0 - sht->vx0;
    by0 = vy0 - sht->vy0;
    bx1 = vx1 - sht->vx0;
    by1 = vy1 - sht->vy0;
    if (bx0 < 0) { bx0 = 0; }
    if (by0 < 0) { by0 = 0; }
    if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
    if (by1 > sht->bysize) { by1 = sht->bysize; }
    if (sht->col_inv == -1) {
      if ((sht->vx0 & 3) == 0 && (bx0 & 3) == 0 && (bx1 & 3) == 0) {
        /* 透明色なし専用の高速版(4バイト型) */
        bx1 = (bx1 - bx0) / 4;  /* MOV回数 */
        sid4 = sid | sid << 8 | sid << 16 | sid << 24;
        for (by = by0; by < by1; by++) {
          vy = sht->vy0 + by;
          vx = sht->vx0 + bx0;
          p = (int *) &map[vy * ctl->xsize + vx];
          for (bx = 0; bx < bx1; bx++) {
            p[bx] = sid4;
          }
        }
      } else {
        /* 透明色ない専用の高速版(1バイト型) */
        for (by = by0; by < by1; by++) {
          vy = sht->vy0 + by;
          for (bx = bx0; bx < bx1; bx++) {
            vx = sht->vx0 + bx;
            map[vy * ctl->xsize + vx] = sid;
          }
        }
      }
    } else {
      /* 透明色ありの一般版 */
      for (by = by0; by < by1; by++) {
        vy = sht->vy0 + by;
        for (bx = bx0; bx < bx1; bx++) {
          vx = sht->vx0 + bx;
          if (buf[by * sht->bxsize + bx] != sht->col_inv) {
            map[vy * ctl->xsize + vx] = sid;
          }
        }
      }
    }
  }
  return;
}
/* 一部対応させる描きなおし関数（ローカル） */
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) /* h0 < HIGHT < h1 */
{
  int h, bx, by, vx, vy, bx0, by0, bx1, by1;  /* h:, bx:, by:, vx:, vy: */
  unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid; /* buf:, c:, vram: */
  struct SHEET *sht;  /* シート一枚分のデータ */
  /* refresh範囲が画面外にはみ出していたら補正 */
  if (vx0 < 0) { vx0 = 0; } /* 画面外に出ている間は、下敷きの対応範囲xを修正 */
  if (vy0 < 0) { vy0 = 0; } /* 画面外に出ている間は、下敷きの対応範囲xを修正 */
  if (vx1 > ctl->xsize) { vx1 = ctl->xsize; } /* 右の端っこの補正 */
  if (vy1 > ctl->ysize) { vy1 = ctl->ysize; } /* 下の端っこの補正 */
  for (h = h0; h <= h1; h++) { /* h0から最上位までの下敷きを */
    sht = ctl->sheets[h];
    buf = sht->buf;
    sid = sht - ctl->sheets0; /* 該当するIDを格納 */
    /* vx0~vy1を使って、bx0~by1を逆算する */
    bx0 = vx0 - sht->vx0; /* bx0のデータがvx0の値に依存する */
    by0 = vy0 - sht->vy0;
    bx1 = vx1 - sht->vx0;
    by1 = vy1 - sht->vy0;
    if (bx0 < 0) { bx0 = 0; }   /* 右下に小さい重ね合わせ処理が来た場合 */
    if (by0 < 0) { by0 = 0; }   /*  右下に小さい重ね合わせ処理が来た場合*/
    if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
    if (by1 > sht->bysize) { by1 = sht->bysize; }
    for (by = by0; by < by1; by++) {
      vy = sht->vy0 + by;
      for (bx = bx0; bx < bx1; bx++) {
        vx = sht->vx0 + bx;
        if (map[vy * ctl->xsize + vx] == sid) { /* 指定のマップ番号とIDが一致したら */
          vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
          /* VRAMに書き込む */
        }
      }
    }
  }
  return;
}

/* 下敷きの高さを設定する関数 */
void sheet_updown(struct SHEET *sht, int height)
{
  struct SHTCTL *ctl = sht->ctl; /* 元々引数で渡していたデータを格納 */
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
        ctl->sheets[h] = ctl->sheets[h - 1];  /* 1個下のメンバに更新 */
        ctl->sheets[h]->height = h;     /* 1個下という高さの情報を現在の高さに変更 */
      }
      ctl->sheets[height] = sht;    /* 一枚分のでーたを割り振り */
      sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
      sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);
    } else {  /* 非表示化 */
      if (ctl->top > old) {
        /* 上になっているものをおろす */
        for (h = old; h < ctl->top; h++) {  /* 設定前の高さから、設定後の高さまで */
          ctl->sheets[h] = ctl->sheets[h + 1];  /* 一個上のメンバに更新 */
          ctl->sheets[h]->height = h;   /* 1個上という高さの情報を現在の高さに変更 */
        }
      }
      ctl->top--; /* 表示中の下敷きが一つ減るので一番上の高さが減る */
      sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
      sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1); /* 新しい下敷きの情報に沿って画面を描き直す */
    }
  } else if (old < height) {    /* もし高さが、設定前より高くなるのなら */
    if (old >= 0) {   /* 設定前の高さが0以上（表示）だったら */
      /* 間のものを押し上げる */
      for (h = old; h< height; h++) { /* oldから設定後の高さまでを */
        ctl->sheets[h] = ctl->sheets[h + 1];  /* 一個上のメンバに更新 */
        ctl->sheets[h]->height = h;   /* 1個上という高さの情報を現在の高さに変更 */
      }
      ctl->sheets[height] = sht;    /* 用意できた高さにデータを挿入 */
    } else {  /* 非表示状態から表示状態へ */
      /* 上になるものを持ち上げる */
      for (h = ctl->top; h >= height; h--) {  /* 引数の高さまで */
        ctl->sheets[h + 1] = ctl->sheets[h];  /* 一個下のデータを入れる */
        ctl->sheets[h + 1]->height = h + 1;   /* 高さの情報を一個下からhの高さへ */
      }
      ctl->sheets[height] = sht;  /* 用意できた高さへデータを挿入 */
      ctl->top++; /* 表示中の下敷きが1つ増えるので、一番上の高さが増える */
    }
    sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
    sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height); 
    /* 新しい下敷きの情報に沿って画面を描きなおす */
  }
  return;
}

void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1)
{
  if (sht->height >= 0) {   /* もしも表示中なら、新しい下敷きの情報に沿って画面を描きなおす */
    sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height); 
    /* 画面全体を対象としてsheet_refreshsubを実行・マップは必要なし */
  }
  return;
}

void sheet_slide(struct SHEET *sht, int vx0, int vy0)
{
  struct SHTCTL *ctl = sht->ctl;
  int old_vx0 = sht->vx0, old_vy0 = sht->vy0; /* 移動前の座標を記録 */
  sht->vx0 = vx0; /* 移動したx座標の情報を格納 */
  sht->vy0 = vy0; /* 移動したy座標の情報を格納 */
  if (sht->height >= 0) { /* もしも表示中なら(下に描画があれば) */
    sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
    /* 移動前のマップ */
    sheet_refreshmap(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
    /* 移動後のマップ */
    sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1); 
    /* 移動前の描きなおし（移動させるレイヤーの一個下まで） */
    sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height); 
    /* 移動後の描きなおし（移動した下敷きのみ） */
  }
  return;
}

void sheet_free(struct SHEET *sht)
{
  if (sht->height >= 0) { /* 表示中なら */
    sheet_updown(sht, -1);  /* 表示中ならまず非表示にする */
  }
  sht->flags = 0; /* 未使用マークを添付 */
  return;
}
