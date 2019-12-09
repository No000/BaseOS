; haribote-ipl
; TAB=4

CYLS    EQU     10      ; どこまで読み込むか

    ORG     0x7c00      ; このプログラムがどこに読み込まれるか

; 以下は標準的なFAT12フォーマットフロッピーディスクのための記述

    JMP     entry
    DB      0x90
    DB      "HARIBOTE"      ; ブートセクタの名前を自由に書いてよい(8バイト)
    DW      512             ; 1セクタの大きさ（512にしなければならない）
    DB      1               ; クラスタの大きさ（1セクタにしなければならない）
    DW      1               ; FATがどこから始まるか（普通は1セクタ目からにする）
    DB      2               ; FATの個数（2にしなければならない）
    DW      224             ; ルートディレクトリの大きさ（普通は224エントリにする）
    DW      2880            ; このドライブの大きさ（2880セクタにしなければならない）
    DB      0xf0            ; メディアのタイプ（0xf0にしなければならない）
    DW      9               ; FAT領域の長さ（9セクタにしなければならない）
    DW      18              ; 1トラックにいくつのセクタがあるのか（18にしなければならない）
    DW      2               ; ヘッドの数（2にしなければならない）
    DD      0               ; パーティションを使ってないのでここは必ず０
    DD      2880            ; このドライブの大きさをもう一度書く
    DB      0,0,0x29        ; よくわからないけれどこの値にしておくといいらしい
    DD      0xffffffff      ; たぶんボリュームシリアル番号
    DB      "HARIBOTEOS "   ; ディスクの名前（11バイト）
    DB      "FAT12   "      ; フォーマットの名前（8バイト）
    RESB    18              ; とりあえず18バイトあけておく

; プログラム本体

entry:
    MOV     AX,0            ; レジスタの初期化
    MOV     SS,AX
    MOV     SP,0x7c00
    MOV     DS,AX



    MOV     AX,0x0820
    MOV     ES,AX
    MOV     CH,0            ; シリンダ０
    MOV     DH,0            ; ヘッド０
    MOV     CL,2            ; セクタ２
readloop:
    MOV     SI,0            ; 失敗回数を数えるレジスタ
retry:
    MOV     AH,0x02         ; AH=0x02：ディスク読み込み
    MOV     AL,1            ; 1セクタ
    MOV     BX,0
    MOV     DL,0x00         ; Aドライブ
    INT     0x13            ; ディスクBIOS呼び出し
    JNC     next            ; エラーが起きなければnextへ
    ADD     SI,1            ; SIに1を足す
    CMP     SI,5            ; SIと5を比較
    JAE     error           ;
    MOV     AH,0x00
    MOV     DL,0x00         ;
    INT     0x13            ;
    JMP     retry
next:
    MOV     AX,ES
    ADD     AX,0x0020
    MOV     ES,AX
    ADD     CL,1
    CMP     CL,18
    JBE     readloop
    MOV     CL,1
    ADD     DH,1
    CMP     DH,2
    JB      readloop
    MOV     DH,0
    ADD     CH,1
    CMP     CH,CYLS
    JB      readloop



fin:
    HLT
    JMP     fin

error:
    MOV     SI,msg
putloop:
    MOV     AL,[SI]
    ADD     SI,1
    CMP     AL,0
    JE      fin
    MOV     AH,0x0e
    MOV     BX,15
    INT     0x10
    JMP     putloop
msg:
    DB      0x0a, 0x0a
    DB      "load error"
    DB      0x0a
    DB      0

    RESB    0x7dfe-$

    DB      0x55, 0xaa