; hariboe-os
; TAB=4

; BOOT_INFO関係（メモリマップを意識）
CYLS    EQU     0x0ff0      ; ブートセクタを設定する
LEDS    EQU     0x0ff1
VMODE   EQU     0x0ff2      ; 色数に関する情報。何ビットカラーか？
SCRNX   EQU     0x0ff4      ; 解像度のx
SCRNY   EQU     0x0ff6      ; 解像度のy
VRAM    EQU     0x0ff8      ; グラフィックバッファの開始番地

    ORG     0xc200      ; このプログラムがメモリ上のどこに読み込まれるか

    MOV     AL,0x13     ; VGAグラフィックス、320x200x8bitカラー
    MOV     AH,0x00
    INT     0x10
    MOV     BYTE    [VMODE],8   ; 画面モードをメモする
    MOV     WORD    [SCRNX],320
    MOV     WORD    [SCRNY],200
    MOV     DWORD   [VRAM],0x000a0000

; キーボードのLED状態をBIOSに教えてもらう

    MOV     AH,0x02
    INT     0x16        ; keyboad BIOS
    MOV     [LEDS],AL

fin:
    HLT                 ; CPU停止命令
    JMP     fin         ; 無限ループ