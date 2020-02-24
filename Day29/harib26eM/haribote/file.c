/* ファイル関連 */

#include "bootpack.h"

void file_readfat(int *fat, unsigned char *img)
/* ディスクイメージ内のFATの圧縮をとく */
{
    int i, j = 0;   /* カウンタ */
    for (i = 0; i < 2880; i += 2) { /* 2880=全セクタ数 */
        fat[i + 0] = (img[j + 0]      | img[j + 1] << 8) & 0xfff;   /* データを並べ直し */
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;   /* 並べ方は書籍P388を参照 */
        j += 3; /* 圧縮される側のデータのカウンタを進める */
    }
    return;
}

void file_loadfile(int clustno, int size, char *buf, int *fat, char *img)
{   /* 引数の情報を元にメモリ上に正しく並べる */
    int i;
    for (;;) {
        if (size <= 512) {  /* 最後のセクタのロード */
            for (i = 0; i < size; i++) {    /* データをロード */
                buf[i] = img[clustno * 512 + i];
            }
            break;  /* 無限ループから出る */
        }
        for (i = 0; i < 512; i++) { /* 2以上のセクタのロード */
            buf[i] = img[clustno * 512 + i];
        }
        size -= 512;
        buf += 512;
        clustno = fat[clustno]; /* アドレスを私てる */
    }
    return;
}

struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max)
{
    int i, j;
    char s[12];
    for (j = 0; j < 11; j++) {
        s[j] = ' ';
    }
    j = 0;
    for (i = 0; name[i] != 0; i++) {
        if (j >= 11) { return 0; /* 見つからなかった */ }
        if (name[i] == '.' && j <= 8) {
            j = 8;
        } else {
            s[j] = name[i];
            if ('a' <= s[j] && s[j] <= 'z') {
                /* 小文字は大文字に直す */
                s[j] -= 0x20;
            }
            j++;
        }
    }
    for (i = 0; i < max; ) {
        if (finfo->name[0] == 0x00) {
            break;
        }
        if ((finfo[i].type & 0x18) == 0) {
            for (j = 0; j < 11; j++) {
                if (finfo[i].name[j] != s[j]) {
                    goto next;
                }
            }
            return finfo + i; /* ファイルが見つかった */
        }
next:
        i++;
    }
    return 0; /* 見つからなかった */
}

char *file_loadfile2(int clustno, int *psize, int *fat)
{
    int size = *psize, size2;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char *buf, *buf2;
    buf = (char *) memman_alloc_4k(memman, size);
    file_loadfile(clustno, size, buf, fat, (char *) (ADR_DISKIMG + 0x003e00));
    if (size >= 17) {
        size2 = tek_getsize(buf);
        if (size2 > 0) {    /* tek圧縮がかかっていた */
            buf2 = (char *) memman_alloc_4k(memman, size2);
            tek_decomp(buf, buf2, size2);
            memman_free_4k(memman, (int) buf, size);
            buf = buf2;
            *psize = size2;
        }
    }
    return buf; /* 返り値は、復元したファイルの開始番地 */
}
