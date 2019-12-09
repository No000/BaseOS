/* 他のファイルで作った関数がありますよとCコンパイラに伝える */
void io_hlt(void);
/* 関数宣言なのに、｛｝がなくていきなり;を書くと、他のファイルにあるからよろしくねという意味になる */

void HariMain(void)
{
  fin:
    io_hlt();/* これでnaskfunk.nasの_io_hltが実行されます */
             /* ここにHLTを入れたいがC言語ではHLTが使えないのでアセンブリのラベルfinを使う */
    goto fin;


}