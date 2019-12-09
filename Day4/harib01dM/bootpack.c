/* 他のファイルで作った関数がありますよとCコンパイラに伝える */
void io_hlt(void);
void write_mem8(int addr, int data);
/* 関数宣言なのに、｛｝がなくていきなり;を書くと、他のファイルにあるからよろしくねという意味になる */

void HariMain(void)
{
    char *p;
    int i ; /* 変数宣言。iという変数は、32ビット整数型 */

    p = (char *) 0xa0000; /* 番地を代入 */

    for (i = 0; i <= 0xaffff ; i++)
    {
        *(p + i) = i & 0x0f; /*アドレスのインクリメントみたいなイメージ？*/
    }

    for (;;)
    {
        io_hlt();
    }
}