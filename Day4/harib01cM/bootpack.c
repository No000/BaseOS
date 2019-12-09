/* 他のファイルで作った関数がありますよとCコンパイラに伝える */
void io_hlt(void);
void write_mem8(int addr, int data);
/* 関数宣言なのに、｛｝がなくていきなり;を書くと、他のファイルにあるからよろしくねという意味になる */

void HariMain(void)
{
    char *p;
    int i ; /* 変数宣言。iという変数は、32ビット整数型 */
    for (i = 0xa0000; i <= 0xaffff ; i++)
    {
        p = i;
        *p = i & 0x0f; /* MOV BYTE [i],15 */
    }

    for (;;)
    {
        io_hlt();
    }
}