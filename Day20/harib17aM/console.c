/* コンソール関係 */

#include "bootpack.h"
#include <stdio.h>
#include <string.h>

void console_task(struct SHEET *sheet, unsigned int memtotal)
{
    struct TIMER *timer;    /* タイマ */
    struct TASK *task = task_now(); /* スリープから復帰する際にtask_nowから参照する */
    int i, fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1;
    char s[30], cmdline[30], *p;    /* cmdline：キー入力保管バッファ */
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    int x, y;
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

    fifo32_init(&task->fifo, 128, fifobuf, task); /* バッファの初期化 */
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);   /* カーソル用（0.5秒） */
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));

    /* プロンプトの表示 */
    putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

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
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1); /* 次は1を */
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                }
                timer_settime(timer, 50);
            }
            if (i == 2) {   /* カーソルON */
                cursor_c = COL8_FFFFFF;
            }
            if (i == 3) {   /* カーソルOFF */
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
                cursor_c = -1;
            }
            if (256 <= i && i <= 511) { /* キーボードデータ（タスクA経由） */
                if (i == 8 + 256) {
                    /* バックスペース */
                    if (cursor_x > 16) {
                        /* カーソルをスペースで消してから、カーソルを1つ戻す */
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                        cursor_x -= 8;
                    }
                } else if (i == 10 + 256) {
                    /* Enter */
                    /* カーソルをスペースで消してから改行する */
                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                    cmdline[cursor_x / 8 - 2] = 0;
                    cursor_y = cons_newline(cursor_y, sheet);
                    /* コマンド実行 */
                    if (strcmp(cmdline, "mem") == 0) {
                        /* memコマンド */
                        sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                        sprintf(s, "free %dKB", memman_total(memman) / 1024);
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (strcmp(cmdline, "cls") == 0) {
                        /* clsコマンド */
                        for (y = 28; y < 28 + 128; y++) {   /* 横方向の画素 */
                            for (x = 8; x < 8 + 240; x++) { /* 縦方向の画素 */
                                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                            }
                        }
                        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
                        cursor_y = 28;
                    } else if (strcmp(cmdline, "ls") == 0) {
                        /* dirコマンド */
                        for (x = 0; x < 224; x++) {
                            if (finfo[x].name[0] == 0x00) { /* fileデータがこれ以上ない */
                                break;
                            }
                            if (finfo[x].name[0] != 0xe5) {
                                if ((finfo[x].type & 0x18) == 0) {
                                    sprintf(s, "filename.ext   %7d", finfo[x].size);
                                    for (y = 0; y < 8; y++) {
                                        s[y] = finfo[x].name[y];
                                    }
                                    s[ 9] = finfo[x].ext[0];
                                    s[10] = finfo[x].ext[1];
                                    s[11] = finfo[x].ext[2];
                                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                            }
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (strncmp(cmdline, "type ", 5) == 0) {   /* 5で比較する文字数指定 */
                            /* typeコマンド */
                            /* ファイル名を準備する */
                            for (y = 0; y < 11; y++) {
                                s[y] = ' '; /* s[0~10]を空白で埋める */
                            }
                            y = 0;  /* yをリセット */
                            for (x = 5; y < 11 && cmdline[x] != 0; x++) {   /* x=5 */
                                if (cmdline[x] == '.' && y <= 8) {  /* .なので拡張子がくる */
                                    y = 8;  /* 構造体の形式に合わせてる */
                                } else {
                                    s[y] = cmdline[x];  /* ファイル名をa[y]へ */
                                    if ('a' <= s[y] && s[y] <= 'z') {
                                        /* 小文字は大文字に直す */
                                        s[y] -= 0x20;   /* ASC2 */
                                    }
                                    y++;    /* 1文字ずつ */
                                }
                            }
                            /* ファイルを探す */
                            for (x = 0; x < 224; ) {    /* データの探索 */
                                if (finfo[x].name[0] == 0x00) { /* 最後の証の0x00についた場合 */
                                    break;
                                }
                                if ((finfo[x].type & 0x18) == 0) {  /* ファイルではない情報＋ディレクトリ ""以外のデータ"" */
                                    for (y = 0; y < 11; y++) {  /* ファイル名を探索 */
                                        if (finfo[x].name[y] != s[y]) { /* 1文字ずつチェック */
                                            goto type_next_file;    /* ファイル名が一致しなかった */
                                        }
                                    }
                                    break;  /* ファイルが見つかった */
                                }
                        type_next_file:
                                x++;    /* 次のファイル */
                        }
                        if (x < 224 && finfo[x].name[0] != 0x00) {
                            /* ファイルが見つかった場合 */
                            p = (char *) memman_alloc_4k(memman, finfo[x].size);
                            file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
                            cursor_x = 8;   /* カーソルの位置を調整 */
                            for (y = 0; y < finfo[x].size; y++) {
                                /* 1文字ずつ出力 */
                                s[0] = p[y];    /* 1文字ずつ入れる */
                                s[1] = 0;
                                if (s[0] == 0x09) { /* タブ */
                                    for(;;) {
                                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
                                        cursor_x += 8;
                                        if (cursor_x == 8 + 240) {
                                            cursor_x = 8;
                                            cursor_y = cons_newline(cursor_y, sheet);
                                        }
                                        if (((cursor_x - 8) & 0x1f) == 0) { /* -8はコンソール8ドット分 */
                                            break;  /* 32で割り切れたらbreak */
                                        }
                                    }
                                } else if (s[0] == 0x0a) {  /* 改行 */
                                    cursor_x = 8;
                                    cursor_y = cons_newline(cursor_y, sheet);
                                } else if (s[0] == 0x0d) {  /* 復帰 */
                                    /* とりあえずなにもしない */
                                } else {    /* 普通の文字 */
                                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                                    cursor_x += 8;  /* 一文字カーソルを進める */
                                    if (cursor_x == 8 + 240) {  /* カーソルの位置がコンソールの端まで来たら */
                                        cursor_x = 8;   /* 元に戻す */
                                        cursor_y = cons_newline(cursor_y, sheet);   /* 次の行に移動 */
                                    }
                                }
                            }
                            memman_free_4k(memman, (int) p, finfo[x].size);
                        } else {    /* ファイルがなかった時のエラー処理 */
                            /* ファイルが見つからなかった場合 */
                            putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
                            cursor_y = cons_newline(cursor_y, sheet);
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (strcmp(cmdline, "hlt") == 0) {
                        /* hlt.hrbアプリケーションを起動 */
                        for (y = 0; y < 11; y++) {
                            s[y] = ' '; /* 11バイト分スペースを入れる */
                        }
                        s[0] = 'H';
                        s[1] = 'L';
                        s[2] = 'T';
                        s[8] = 'H';
                        s[9] = 'R';
                        s[10] = 'B';    /* 探索するファイル名を格納 */
                        for (x = 0; x < 224; ) {
                            if (finfo[x].name[0] == 0x00) {
                                break;  /* プログラムが無かった */
                            }
                            if ((finfo[x].type & 0x18) == 0) {
                                for (y = 0; y < 11; y++) {
                                    if (finfo[x].name[y] != s[y]) { /* sに指定したプログラムで無かったら */
                                        goto hlt_next_file; /* 次のプログラムを確認しにいく */
                                    }
                                }
                                break; /* ファイルが見つかった */
                            }
                hlt_next_file:
                            x++;
                        }
                        if (x < 224 && finfo[x].name[0] != 0x00) {
                            /* ファイルが見つかった場合 */
                            p = (char *) memman_alloc_4k(memman, finfo[x].size); /* メモリ確保 */
                            file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *) (ADR_DISKIMG + 0x003e00)); /* ファイルをロード */
                            set_segmdesc(gdt + 1003, finfo[x].size - 1, (int) p, AR_CODE32_ER); /* ロードしたファイルをセグメントに登録 */
                            farjmp(0, 1003 * 8); /* セグメントのタスクへ飛ぶ */
                            memman_free_4k(memman, (int) p, finfo[x].size); /* プログラム実行後、メモリ解放 */
                        } else {
                            /* ファイルが見つからなかった場合 */
                            putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found", 15);
                            cursor_y = cons_newline(cursor_y, sheet);
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (cmdline[0] != 0) {
                        /* コマンドではなく、さらに空行でもない */
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Bad command.", 12);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    /* プロンプト表示 */
                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">", 1);
                    cursor_x = 16;
                } else {
                    /* 一般文字 */
                    if (cursor_x < 240) {
                        /* 一文字表示をしてから、カーソルを1つ進める */
                        s[0] = i - 256;
                        s[1] = 0;
                        cmdline[cursor_x / 8 - 2] = i - 256;
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
                        cursor_x += 8;
                    }
                }
            }
            /* カーソルの再表示 */
            if (cursor_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
            }
            sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
        }
    }
}



int cons_newline(int cursor_y, struct SHEET *sheet)
{
    int x, y;
    if (cursor_y < 28 + 112) {
        cursor_y += 16; /* 次の行へ */
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
    return cursor_y;
}
