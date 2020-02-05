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
