; hariboe-os
; TAB=4

    ORG     0xc200      ; このプログラムがメモリ上のどこに読み込まれるか
fin:
    HLT                 ; CPU停止命令
    JMP     fin         ; 無限ループ