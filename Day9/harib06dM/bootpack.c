/* bootpackのメイン */

#include "bootpack.h"
#include <stdio.h>
/* メモリ管理用 */
#define MEMMAN_FREES    4090    /*  */
#define MEMMAN_ADDR     0x003c0000
/* Freeのアドレスとサイズ管理 */
struct FREEINFO {       /* あき情報 */
    unsigned int addr, size;    /* 各種、addr:アドレス, size:大きさ */
};
/* 失ったサイズやどれだけFreeかの管理 */
struct MEMMAN {         /* メモリ管理 */
    int frees, maxfrees, lostsize, losts;   /* 関数memman_initを参照 */
    struct FREEINFO free[MEMMAN_FREES];     /* 配列で表にする */
};

/* 新しく追加する関数 */
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128]; /* 各種バッファ */
    int mx, my, i;
    unsigned int memtotal;
    struct MOUSE_DEC mdec; /* マウスのデータを構造体で管理（タグ：mdec） */
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

    init_gdtidt(); /* GDT、IDTの初期化 */
    init_pic(); /* PICの初期か */
    io_sti();   /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */

    fifo8_init(&keyfifo, 32, keybuf);           /* キーボードバッファの初期化 */
    fifo8_init(&mousefifo, 128, mousebuf);      /* マウスバッファの初期化 */
    io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可（1111_10001） */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可（1110_1111） */

    init_keyboard();                            /* キーボードの初期化 */
    enable_mouse(&mdec);  /* マウス有効化処理 */
    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 -0x0009efff */
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2; /* 画面中央になるx座標 */
    my = (binfo->scrny - 28 -16) / 2; /* 画面中央になるy座標 */
    init_mouse_cursor8(mcursor, COL8_008484);   /* マウス描写 */
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my); /* マウスの位置が画面にデバッグ */
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
    /* キーボードとマウスの許可は上に移動 */
    /* 理想的な割り込み処理 */

    sprintf(s, "memory %dMB   free : %dKB", memtotal / (1024 *1024), memman_total(memman) / 1024);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    for (;;) {
        io_cli();                                                     /* 外部割り込み禁止（割り込み処理中の割り込み対策） */
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) { /* どちらからもデータが来てないことの確認 */
            io_stihlt();                                              /* 外部割り込みの許可と、CPU停止命令(割り込みの終了) */
        } else {
            if (fifo8_status(&keyfifo) != 0) {  /* もしキーボードの方のデータが来ていたら */
                i = fifo8_get(&keyfifo); /* キーコードのアドレスを変数iに格納 */
                io_sti(); /* IFに1をセット（外部割り込みの許可） */ 
                sprintf(s, "%02X", i); /* メモリのデータの参照 */
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31); /* 画面のリセット */
                putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s); /* 文字の表示 */
            } else if (fifo8_status(&mousefifo) != 0) {         /* もしもマウスのデータが来ていたら */
                i = fifo8_get(&mousefifo);                      /* マウスのデータをバッファから取得し */
                io_sti();                                       /* IFに1をセット（外部割り込みの許可） */
                if (mouse_decode(&mdec, i) != 0) {
                    /* データが3バイト揃ったので表示 */
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y); /* それぞれのデータを文字列sに格納 */
                    if ((mdec.btn & 0x01) != 0) {                /* 左ボタンのデータ以外のデータをマスクして判定 */
                        s[1] = 'L';                              /* lを大文字に */
                    }
                    if ((mdec.btn & 0x02) != 0) {                /* 右ボタン以外のデータをマスクして判定 */
                        s[3] = 'R';                              /* rを大文字に */
                    }
                    if ((mdec.btn & 0x04) != 0) {                /* 中ボタン以外のデータをマスクして判定 */
                        s[2] = 'C';                              /* cを大文字に */
                    }
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);       /* ボックスの描写 */
                    putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);                   /* 文字の描写 */
                    /* マウスカーソルの移動 */
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);         /* マウスを消す */
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx -16) {
                        mx = binfo->scrnx -16;
                    }
                    if (my > binfo->scrny -16) {
                        my = binfo->scrny -16;
                    }
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
                    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
                    putblock8_8(binfo->vram, binfo->scrnx, 16,16, mx, my, mcursor, 16);
                }
            }
        }
    }
}

#define EFLAGS_AC_BIT       0x00040000
#define CR0_CACHE_DISABLE   0x60000000

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
    for (i = 0; i < man->frees; i++) {      /* freesの最後までを繰り返しで確認 */
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
