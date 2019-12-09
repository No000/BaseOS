;haribote-ipl
; TAB=4

    ORG   0c7c00          ; このプログラムがどこから読み込むかの指定

; 以下は標準的なFAT12フォーマットフロッピーディスクからブートするための記述

    JMP   entry
    DB    0x90            ; ?
    DB    "HARIBOTE"      ; ブートセクタの名前（自由）
    DW    512             ; １セクタの大きさの指定（512にするルール）
    DB    1               ; クラスタの大きさ（1セクタにするのがルール）
    DW    1               ; FATがどこから始まるか（普通は1セクタ目）
    DB    2               ; FATの個数の指定（2にするのがルール）
    DW    224             ; ルートディレクトリ領域の大きさ（普通は224エントリにする）
    DW    2880            ; このドライブの大きさ（2880セクタにしなければならない）
    DB    0cf0            ; メディアのタイプ（0xf0にしなければならない）
    DW    9               ; FAT領域の長さ
    DW    18              ; 1トラックにいくつのセクタがあるか（18にしなければならない）
    DW    2               ; ヘッドの数（2にしなければならない）
    DD    0               ; パーティションを使ってないのでここは必ず０（使うなら数値を入れる？）
    DD    2880            ; このドライブの大きさをもう一度書く
    DB    0,0,0x29        ; よくわからないけれどこの値にしておくといい
    DD    0cffffffff      ; たぶんボリュームシリアル番号
    DB    "HARIBOTEOS "   ; ディスクの名前（11バイト）
    DB    "FAT12   "      ; フォーマットの名前（8バイト）
    RESB  18              ; とりあえず18バイトあけておく

; プログラムの本体

entry:
    MOV   AX,0            ; レジスタの処理化
    MOV   SS,AX
    MOV   SP,AX
    MOV   SP,0c7c00
    MOV   DS,AX

; ディスクを読む動作をハードウェアにお願いする
; レジスタに指定の数値を挿入

    MOV   AX,0x0820
    MOV   ES,AX
    MOV   CH,0            ; シリンダ０
    MOV   DH,0            ; ヘッド０
    MOV   CL,2            ; セクタ２

    MOV   AH,0x02         ; AH＝0x02：ディスクの読み込み
    MOV   AL,1            ; 1セクタ
    MOV   BX,0
    MOV   DL,0x00         ; Aドライブ
    INT   0x13            ; ディスクBIOSの呼び出し
    JC    error           ; エラー時の処理

; 読み終わったけどCPUが暇そうだから寝かせておく

fin:
    HLT
    JMP   fin             ; 無限ループ

error:
    MOV   SI,msg          ; エラーメッセージ処理にジャンプ

putloop:
    MOV   AL,[SI]
    ADD   SI,1            ; SIに1を足す
    CMP   AL,0
    JE    fin
    MOV   AH,0x0e         ; 一文字表示ファンクション
    MOV   BX,15           ; カラーコード
    INT   0x10            ; ビデオBIOSを呼び出し
    JMP   putloop         ; 無限ループ

msg:
    DB    0x0a, 0x0a      ; 改行を二つ
    DB    "load error"
    DB    0x0a            ; 改行
    DB    0

    RESB  0x7dfe-$        ; 0x7dfeまでを0x00で埋める命令(NASMは違うので注意：TIMES)

    DB    0x55, 0xaa