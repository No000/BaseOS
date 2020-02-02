/* メモリ関係 */
#include "bootpack.h"

#define EFLAGS_AC_BIT       0x00040000
#define CR0_CACHE_DISABLE   0x60000000

/* 表をイメージすること */
unsigned int memtest(unsigned int start, unsigned int end)
{
    char flg486 = 0;            /* 486かのフラグをオフとして定義 */
    unsigned int eflg, cr0, i;  /* eflg:EFLAGのビットパターン格納用, cr0:cr0のビットパターン格納用, i:メモリの容量用 */

    /* 386か、486以降なのかの確認(EFLAGの18ビットの判定) */
    eflg = io_load_eflags(); /* EFLAGの内容を変数eflgに格納 */
    eflg |= EFLAGS_AC_BIT; /* AC-bit = 1（ACフラグをON） */
    io_store_eflags(eflg); /* EFLAGに読み込み */
    eflg = io_load_eflags(); /* 再度読み出し */
    if ((eflg & EFLAGS_AC_BIT) != 0) {  /* 386ではAC=1にしても自動で0に戻ってしまう（386か486の判定） */
        flg486 = 1;                     /* 486であると判定しフラグをON */
    }
    eflg &= ~EFLAGS_AC_BIT;    /* AC-bit = 0(AND演算子とビット反転を使って全ビットを0にリセット) */
    io_store_eflags(eflg);      /* EFLAGに読み込み */

    if (flg486 != 0) {          /* もし486なら */
        cr0 = load_cr0();       /* cr0の現在のビットパターンをロードして */
        cr0 |= CR0_CACHE_DISABLE;   /* キャッシュの許可をセット */
        store_cr0(cr0);         /* cr0にストア（というより戻す？）する */
    }

    i = memtest_sub(start, end);    /* メモリ容量の測定 */

    if (flg486 != 0) {              /* もし486なら */
        cr0 = load_cr0();           /* cr0のビットパターンを読み出し */
        cr0 &= ~CR0_CACHE_DISABLE;   /* キャッシュの許可をセット */
        store_cr0(cr0);             /* CR0に戻す */
    }

    return i;                       /* メモリの容量を返す */
}

void memman_init(struct MEMMAN *man)
{
    man->frees = 0;         /* 空き情報の個数：一番重要 */
    man->maxfrees = 0;      /* 状況観察用：freesの最大値 */
    man->lostsize = 0;      /* 開放に失敗した合計サイズ */
    man->losts = 0;         /* 開放に失敗した回数 */
    return;
}
/* メモリの空きサイズを報告する */
unsigned int memman_total(struct MEMMAN *man)
/* あきサイズの合計を報告 */
{
    unsigned int i, t = 0;              /* i:カウンタ, t:空きが何個あるかのカウンタ */
    for ( i = 0; i < man->frees; i++) { /* freesの個数だけ繰り返し */
        t += man->free[i].size;         /* それぞれのfreesにおけるサイズを加算してゆく */
    }
    return t;                           /* 合計サイズを返す */
}
/* メモリ確保用の関数 */
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
/* 確保 */
{
    unsigned int i,a;   /* i:メモリの確保した空き情報のカウンタ, a:アドレス格納用 */
    for (i = 0; i < man->frees; i++) {      /* fressの最後までを繰り返しで確認 */
        if (man->free[i].size >= size) {    /* 必要なサイズ以上の空きがあったなら */
            /* 十分な広さのあきを発見 */
            a = man->free[i].addr;          /* 変数aにアドレスを格納 */
            man->free[i].addr += size;      /* アドレスを確保した分進める */
            man->free[i].size -= size;      /* 空き容量のサイズを確保した分減らす */
            if (man->free[i].size == 0) {   /* もしi番目の空きサイズが足りなくなったら */
                /* free[i]がなくなったので前へ詰める */
                man->frees--;               /* 空き情報の個数を減らし */
                for (; i< man->frees; i++) {    /* 繰り返しで空き情報の順番を前に詰める */
                    man->free[i] = man->free[i + 1]; /* 構造体の代入 */
                }
            }
            return a;          /* 確保したアドレスを返す */
        }
    }
    return 0; /* あきがない */
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i, j;
    /* まとめやすさを考えると、free[]がaddr順に並んでいるほうがいい */
    /* だからまず、どこに入れるべきかを決める */
    for (i = 0; i < man->frees; i++) {      /* freesを先頭から繰り返し */
        if (man->free[i].addr > addr) {     /* 開放するアドレスが順番になる直前に来たら */
            break;                          /* ブレイク */
        }
    }
    /* free[i - 1].addr < addr < free[i].addr */
    if (i > 0) {
        /* 前がある */
        if (man->free[i-1].addr + man->free[i - 1].size == addr) {
            /* 前の空き容量にまとめられる */
            man->free[i - 1].size += size;
            if (i < man->frees) {
                /* 後ろもある */
                if (addr + size == man->free[i].addr) {
                    /* なんと後ろともまとめられる */
                    man->free[i - 1].size += man->free[i].size;
                    /* man->free[i]の削除 */
                    /* free[i]がなくなったので前へ詰める */
                    man->frees--;
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i + 1]; /* 構造体の代入 */
                    }
                }
            }
            return 0; /* 成功終了 */
        }
    }
    /* 前とはまとめられなかった */
    if (i < man->frees) {
        if (addr + size == man->free[i].addr) {
            /* 後ろとはまとめられる */
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0;   /* 成功終了 */
        }
    }
    /* 前にも後ろにもまとめられない */
    if (man->frees < MEMMAN_FREES) {
        /* free[i]より後ろを、後ろへずらして、隙間を作る */
        for (j = man->frees; j > i; j--) {
            man->free[j] = man->free[j -1];
        }
        man->frees++;
        if (man->maxfrees < man->frees) {
            man->maxfrees = man->frees; /* 最大値を更新 */
        }
        man->free[i].addr = addr;
        man->free[i].size= size;
        return 0; /* 成功終了 */
    }
    /* 後ろにずらせなかった */
    man->losts++;
    man->lostsize += size;
    return -1; /* 失敗終了 */
}

/* メモリ確保を4KBごとに */
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
  unsigned int a;
  size = (size + 0xfff) & 0xfffff000; /* サイズをAND命令を使い切り上げ、切り下げをする */
  a = memman_alloc(man, size); /* 切り上げ下げを行ったサイズで確保 */
  return a;
}

/* メモリ開放を4KBごとに */
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
  int i;
  size = (size + 0xfff) & 0xfffff000; /* サイズをAND命令を使い切り上げ、切り下げをする */
  i = memman_free(man, addr, size); /* 切り上げ下げを行ったサイズで開放 */
  return i;
}
