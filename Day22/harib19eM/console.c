/* コンソール関係 */

#include "bootpack.h"
#include <stdio.h>
#include <string.h>

void console_task(struct SHEET *sheet, unsigned int memtotal)
{
    struct TIMER *timer;    /* タイマ */
    struct TASK *task = task_now(); /* スリープから復帰する際にtask_nowから参照する */
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    int i, fifobuf[128], *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
    struct CONSOLE cons;
    char cmdline[30];    /* cmdline：キー入力保管バッファ */
    cons.sht = sheet;
    cons.cur_x =  8;
    cons.cur_y = 28;
    cons.cur_c = -1;
    *((int *) 0x0fec) = (int) &cons;    /* consのデータをメモ */

    fifo32_init(&task->fifo, 128, fifobuf, task); /* バッファの初期化 */
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);   /* カーソル用（0.5秒） */
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));

    /* プロンプトの表示 */
    cons_putchar(&cons, '>', 1);

    for (;;) {
        io_cli();   /* IF=0(割り込み無効) */
        if (fifo32_status(&task->fifo) == 0) {    /* バッファにデータが来ていなければ */
            task_sleep(task);   /* スリープ */
            io_sti();   /* IF=1 */
        } else {
            i = fifo32_get(&task->fifo);  /* バッファからデータを1バイト引き出す */
            io_sti();   /* IF=1 */
            if (i <= 1) {   /* カーソル用タイマ */
                if (i != 0) {
                    timer_init(timer, &task->fifo, 0); /* 次は0を */
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1); /* 次は1を */
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_000000;
                    }
                }
                timer_settime(timer, 50);
            }
            if (i == 2) {   /* カーソルON */
                cons.cur_c = COL8_FFFFFF;
            }
            if (i == 3) {   /* カーソルOFF */
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
                cons.cur_c = -1;
            }
            if (256 <= i && i <= 511) { /* キーボードデータ（タスクA経由） */
                if (i == 8 + 256) {
                    /* バックスペース */
                    if (cons.cur_x > 16) {
                        /* カーソルをスペースで消してから、カーソルを1つ戻す */
                        cons_putchar(&cons, ' ', 0);
                        cons.cur_x -= 8;
                    }
                } else if (i == 10 + 256) {
                    /* Enter */
                    /* カーソルをスペースで消してから改行する */
                    cons_putchar(&cons, ' ', 0);
                    cmdline[cons.cur_x / 8 - 2] = 0;
                    cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal);  /* コマンド実行 */
                    /* プロンプト表示 */
                    cons_putchar(&cons, '>', 1);
                } else {
                    /* 一般文字 */
                    if (cons.cur_x < 240) {
                        /* 一文字表示をしてから、カーソルを1つ進める */
                        cmdline[cons.cur_x / 8 - 2] = i - 256;
                        cons_putchar(&cons, i - 256, 1);
                    }
                }
            }
            /* カーソルの再表示 */
            if (cons.cur_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
            }
            sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
        }
    }
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
    char s[2];
    s[0] = chr;
    s[1] = 0;
    if (s[0] == 0x09) { /* タブ */
        for (;;) {
            putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240) {
                cons_newline(cons);
            }
            if (((cons->cur_x - 8) & 0x1f) == 0) {
                break; /* 32で割り切れたらbreak */
            }
        }
    } else if (s[0] == 0x0a) {  /* 改行 */
        cons_newline(cons);
    } else if (s[0] == 0x0d) {  /* 復帰 */
        /* とりあえずなにもしない */
    } else {    /* 普通の文字 */
        putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
        if (move != 0) {
            /* moveが0の時はカーソルを進めない */
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240) {
                cons_newline(cons);
            }
        }
    }
    return;
}

void cons_newline(struct CONSOLE *cons)
{
    int x, y;
    struct SHEET *sheet = cons->sht;
    if (cons->cur_y < 28 + 112) {
        cons->cur_y += 16; /* 次の行へ */
    } else {
        /* スクロール */
        for (y = 28; y < 28 + 112; y++) {
            for (x = 8; x < 8 + 240; x++) {
                sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
            }
        }
        for (y = 28 + 112; y < 28 + 128; y++) {
            for (x = 8; x < 8 + 240; x++) {
                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
            }
        }
        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    }
    cons->cur_x = 8;
    return;
}
/*  */
void cons_putstr0(struct CONSOLE *cons, char *s) /* 文字コード0判定型 */
{
    for (; *s != 0; s++) {
        cons_putchar(cons, *s, 1);
    }
    return;
}

void cons_putstr1(struct CONSOLE *cons, char *s, int l) /* 文字数指定型 */
{
    int i;
    for (i = 0; i < l; i++) {
        cons_putchar(cons, s[i], 1);
    }
    return;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal)
{
    if (strcmp(cmdline, "mem") == 0) {
        cmd_mem(cons, memtotal);
    } else if (strcmp(cmdline, "cls") == 0) {
        cmd_cls(cons);
    } else if (strcmp(cmdline, "ls") == 0) {
        cmd_dir(cons);
    } else if (strncmp(cmdline, "type ", 5) == 0) {
        cmd_type(cons, fat, cmdline);
    } else if (cmdline[0] != 0) {
        if (cmd_app(cons, fat, cmdline) == 0) { /* コマンドラインのファイルを確認して検索してくれる */
            /* コマンドではなく、さらに空行でもない */
            cons_putstr0(cons, "Bad command.\n\n");
        }
    }
    return;
}

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char s[60];
    sprintf(s, "total   %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    cons_putstr0(cons, s);
    return;
}

void cmd_cls(struct CONSOLE *cons)
{
    int x, y;
    struct SHEET *sheet = cons->sht;
    for (y = 28; y < 28 + 128; y++) {   /* 横方向の画素 */
        for (x = 8; x < 8 + 240; x++) { /* 縦方向の画素 */
            sheet->buf[x + y * sheet->bxsize] = COL8_000000;
        }
    }
    sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    cons->cur_y = 28;
    return;
}

void cmd_dir(struct CONSOLE *cons)
{
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    int i, j;
    char s[30];
    for (i = 0; i < 224; i++) {
        if (finfo[i].name[0] == 0x00) { /* fileデータがこれ以上ない */
            break;
        }
        if (finfo[i].name[0] != 0xe5) {
            if ((finfo[i].type & 0x18) == 0) {
                sprintf(s, "filename.ext   %7d\n", finfo[i].size);
                for (j = 0; j < 8; j++) {
                    s[j] = finfo[i].name[j];
                }
                s[ 9] = finfo[i].ext[0];
                s[10] = finfo[i].ext[1];
                s[11] = finfo[i].ext[2];
                cons_putstr0(cons, s);
            }
        }
    }
    cons_newline(cons);
    return;
}

void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo = file_search(cmdline + 5, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    char *p;
    if (finfo != 0) {
        /* ファイルが見つかった場合 */
        p = (char *) memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        cons_putstr1(cons, p, finfo->size);
        memman_free_4k(memman, (int) p, finfo->size);
    } else {
        /* ファイルが見つからなかった場合 */
        cons_putstr0(cons, "File not found.\n");
    }
    cons_newline(cons);
    return;
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    char name[18], *p, *q;
    struct TASK *task = task_now();
    int i;

    /* コマンドラインからファイル名を生成 */
    for (i = 0; i < 13; i++) {
        if (cmdline[i] <= ' ') {
            break;
        }
        name[i] = cmdline[i];
    }
    name[i] = 0; /* とりあえずファイル名の後ろを0にする */

    /* ファイルを探す */
    finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    if (finfo == 0 && name[i - 1] != '.') { /* 拡張子無しの処理 */
        /* 見つからなかったので後ろに".HRB"をつけてもう一度探して見る */
        name[i    ] = '.';
        name[i + 1] = 'H';
        name[i + 2] = 'R';
        name[i + 3] = 'B';
        name[i + 4] = 0;
        finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);   /* ファイル名探索 */
    }

    if (finfo != 0) {
        /* ファイルが見つかった場合のプログラム実行処理 */
        p = (char *) memman_alloc_4k(memman, finfo->size);
        q = (char *) memman_alloc_4k(memman, 64 * 1024);    /* APP用 */
        *((int *) 0xfe8) = (int) p; /* 0xfecにコードッセグメントのアドレスをバックアップ */
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        set_segmdesc(gdt + 1003, finfo->size - 1, (int) p, AR_CODE32_ER + 0x60); /* アプリ用のセグメント設定 */
        set_segmdesc(gdt + 1004, 64 * 1024 - 1,   (int) q, AR_DATA32_RW + 0x60); /* たぶんアクセス権がらみ？ */
        if (finfo->size >= 8 && strncmp(p + 4, "Hari", 4) == 0) {
            start_app(0x1b, 1003 * 8, 64 * 1024, 1004 * 8, &(task->tss.esp0));
        } else {
            start_app(0, 1003 * 8, 64 * 1024, 1004 * 8, &(task->tss.esp0)); /* TSSに登録(espとOSのセグメント) */
        }
        memman_free_4k(memman, (int) p, finfo->size);
        memman_free_4k(memman, (int) q, 64 * 1024);
        cons_newline(cons);
        return 1;
    }
    /* ファイルが見つからなかった場合 */
    return 0;
}

int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) /* 機能番号対応文字表示API */
{
    int cs_base = *((int *) 0xfe8);
    struct TASK *task = task_now();
    struct CONSOLE *cons = (struct CONSOLE *) *((int *) 0x0fec);
    char s[12];
    if (edx == 1) {
        cons_putchar(cons, eax & 0xff, 1);
    } else if (edx == 2) {
    /*    cons_putstr0(cons, (char *) ebx + cs_base); */
    sprintf(s,"%08X\n", ebx);
    cons_putstr0(cons, s);
    } else if (edx == 3) {
        cons_putstr1(cons, (char *) ebx + cs_base, ecx);
    } else if (edx == 4) {  /* 終了API */
        return &(task->tss.esp0);
    }
    return 0;
}

int *inthandler0c(int *esp)
{
    struct CONSOLE *cons = (struct CONSOLE *) *((int *) 0x0fec);
    struct TASK *task = task_now();
    char s[30];
    cons_putstr0(cons, "\nINT 0C :\n Stack Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]);
    cons_putstr0(cons, s);
    return &(task->tss.esp0); /* 異常終了させる */
}

int *inthandler0d(int *esp)
{
    struct CONSOLE *cons = (struct CONSOLE *) *((int *) 0x0fec);
    struct TASK *task = task_now();
    char s[30];
    cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]);
    cons_putstr0(cons, s);
    return &(task->tss.esp0); /* 異常終了させる */
}
