; hariboe-os
; TAB=4

    ORG     0xc200      ; このプログラムがメモリ上のどこに読み込まれるか

    MOV     AL,0x13     ; VGAグラフィックス、320x200x8bitカラー
    MOV     AH,0x00
    INT     0x10
fin:
    HLT                 ; CPU停止命令
    JMP     fin         ; 無限ループ