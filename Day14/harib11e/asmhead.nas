; hariboe-os boot asm
; TAB=4

[INSTRSET "i486p"]              ; 486命令まで使いたいという記述

VBEMODE EQU     0x105           ; 1024 × 768 × 8bitカラー
; （画面モード一覧）
; 0x100 :  640 ×  400 x 8bitカラー
; 0x101 :  640 x  480 x 8bitカラー
; 0x103 :  800 x  600 x 8bitカラー
; 0x105 : 1024 x  768 x 8bitカラー
; 0x107 : 1280 x 1024 x 8bitカラー

BOTPAK  EQU     0x00280000      ; bootpackのロード先
DSKCAC  EQU     0x00100000      ; ディスクキャッシュの場所
DSKCAC0 EQU     0x00008000      ; ディスクキャッシュの場所（リアルモード）

; BOOT_INFO関係（メモリマップを意識）
CYLS    EQU     0x0ff0      ; ブートセクタを設定する
LEDS    EQU     0x0ff1
VMODE   EQU     0x0ff2      ; 色数に関する情報。何ビットカラーか？
SCRNX   EQU     0x0ff4      ; 解像度のx
SCRNY   EQU     0x0ff6      ; 解像度のy
VRAM    EQU     0x0ff8      ; グラフィックバッファの開始番地

    ORG     0xc200      ; このプログラムがメモリ上のどこに読み込まれるか

; VBEの存在確認

    MOV     AX,0x9000
    MOV     ES,AX
    MOV     DI,0        ; ES:DIの領域にVBEの情報が格納
    MOV     AX,0x4f00
    INT     0x10        ;   VBEがあった場合AX=0x004fに変化
    CMP     AX,0x004f
    JNE     scrn320

; VBEのバージョンチェック(2.0)

    MOV     AX,[ES:DI+4]
    CMP     AX,0x0200
    JB      scrn320     ; if (AX < 0x0200) goto scrn320

; 画面モード情報を得る

    MOV     CX,VBEMODE
    MOV     AX,0x4f01
    INT     0x10
    CMP     AX,0x004f   ; 0x004fじゃなければ、指定した画面モードが使えなかった
    JNE     scrn320

; 画面モード情報の確認（[ES:DI]でのオフセットでの確認）

    CMP     BYTE [ES:DI+0x19],8
    JNE     scrn320
    CMP     BYTE [ES:DI+0x1b],4
    JNE     scrn320
    MOV     AX,[ES:DI+0x00]
    AND     AX,0x0080
    JZ      scrn320         ; モード属性のbit7が0だったのであきらめる

; 画面モードの切り替え(ここまで来て切り替えの準備が終了)

    MOV     BX,VBEMODE+0x4000
    MOV     AX,0x4f02
    INT     0x10
    MOV     BYTE [VMODE],8  ; 画面モードをメモする（C言語が参照する）
    MOV     AX,[ES:DI+0x12]
    MOV     [SCRNX],AX
    MOV     AX,[ES:DI+0x14]
    MOV     [SCRNY],AX
    MOV     EAX,[ES:DI+0x28]
    MOV     [VRAM],EAX
    JMP     keystatus

scrn320:
    MOV     BX,0x13     ; VBEの320×200×8bitカラー
    MOV     AX,0x00
    INT     0x10
    MOV     BYTE [VMODE],8   ; 画面モードをメモする(C言語が参照する)
    MOV     WORD [SCRNX],320
    MOV     WORD [SCRNY],200
    MOV     DWORD [VRAM],0x000a0000

; キーボードのLED状態をBIOSに教えてもらう

keystatus:
    MOV     AH,0x02
    INT     0x16        ; keyboad BIOS
    MOV     [LEDS],AL

; PICが一切の割り込みを受け付けないようにする
; AT互換機の仕様では、PICの初期化をするなら、
; こいつをCLI前にやっておかないと、たまにハングアップする
; PICの初期化はあとでやる

    MOV     AL,0xff ; ALに0xffを代入
    OUT     0x21,AL ; PIC0_IMRを指定して0xffコマンド（割り込み停止）
    NOP             ; OUT命令を連続させるとうまくいかない機種があるらしいので（1クロック休憩）
    OUT     0xa1,AL ; PIC1_IMRを指定して0xffコマンド（割り込み停止）

    CLI             ; さらにCPUレベルでも割り込み禁止

; CPUから１MB以上のメモリにアクセスできるように、A20GATEを設定
; 16ビットCPUのメモリから32ビットCPUのメモリの大きさに変化

    CALL    waitkbdout  ; KBCコマンド出力待機
    MOV     AL,0xd1     ; ALにポート番号を格納
    OUT     0x64,AL     ; アウトプットポートに書き込み
    CALL    waitkbdout  ; KBCコマンド出力待機
    MOV     AL,0xdf     ; enable A20（A20アドレスライン有効化コマンド）
    OUT     0x60,AL     ; キーボードエンコーダーにコマンドを送信
    CALL    waitkbdout  ; KBCコマンド出力待機

; プロテクトモード移行（保護あり仮想メモリモード）

    LGDT    [GDTR0]     ; 仮のGDTを読み込み
    MOV     EAX,CR0         ; コントロールレジスタ0のデータをEAXに読みだす
    AND     EAX,0x7fffffff  ; bit31を0にする（ページング禁止のため）
    OR      EAX,0x00000001  ; bit0を１にする（プロテクトモード移行のため）
    MOV     CR0,EAX         ; コントロールレジスタ0にステータスを戻す
    JMP     pipelineflush   ; 機械語が変化することによるJMP命令（必須）
pipelineflush:              ; パイプラインがかかわっているので
    MOV     AX,1*8          ; AXに0x0008を代入
    MOV     DS,AX           ; データセグメントをプロテクトモードに対応
    MOV     ES,AX           ; エクストラセグメントをプロテクトモードへ対応
    MOV     FS,AX           ; Fセグメントをプロテクトモードへ対応
    MOV     GS,AX           ; Gセグメントをプロテクトモードへ対応
    MOV     SS,AX           ; スタックセグメントをプロテクトモードへ対応
; CSは後回し（混乱するらしい）

; bootpackの転送

    MOV     ESI,bootpack    ; 転送元
    MOV     EDI,BOTPAK      ; 転送先
    MOV     ECX,512*1024/4  ; 転送サイズ（ダブルワードなので1/4）
    CALL    memcpy          ; データをメモリ上で上の3命令で指定したデータに従い転送

; ついでにディスクデータも本来の位置へ転送

; まずはブートセクタから

    MOV     ESI,0x7c00      ; 転送元
    MOV     EDI,DSKCAC      ; 転送先
    MOV     ECX,512/4       ; サイズ指定
    CALL    memcpy          ; メモリ上でのデータ転送

; 残り全部

    MOV     ESI,DSKCAC0+512 ; 転送先
    MOV     EDI,DSKCAC+512  ; 転送元
    MOV     ECX,0           ; 下位4バイトのみのデータにするためにリセット
    MOV     CL,BYTE [CYLS]
    IMUL    ECX,512*18*2/4  ; シリンダ数からバイト数/4に変換
    SUB     ECX,512/4       ; IPLの分だけ差し引く
    CALL    memcpy

; asmheadでしなければいけないことは全部し終わったので、
; あとはbootpackに任せる

; bootpackの起動

    MOV     EBX,BOTPAK
    MOV     ECX,[EBX+16]
    ADD     ECX,3       ; ECX += 3;
    SHR     ECX,2       ; ECX /= 4;
    JZ      skip        ; 転送すべきものがない（JZは直前の計算結果が0の場合）
    MOV     ESI,[EBX+20]    ; 転送元
    ADD     ESI,EBX
    MOV     EDI,[EBX+12]    ; 転送先
    CALL    memcpy          ; はりぼてアプリケーション時に説明
skip:
    MOV     ESP,[EBX+12]            ; スタックの初期値
    JMP     DWORD 2*8:0x0000001b    ; bootpack.hrbの0x1b番地にジャンプ（特別なJMP命令らしい）
; 詳細は書籍のメモリマップを見たらわかりやすい

waitkbdout:
    IN      AL,0x64                 ; キーボードにたまっていたキーボード、マウスのデータの受け取り
    AND     AL,0x02                 ; データがまだあるかの確認？
    IN      AL,0x60                    ; 空読み（受信バッファが悪さをしないように）
    JNZ     waitkbdout       ; ANDの結果が0でなければwaitbdoutへ
    RET

memcpy:                     ; メモリを4バイトずつコピーするラベル
    MOV     EAX,[ESI]       ; 転送元のデータをEAXに格納
    ADD     ESI,4           ; アドレスを4進める
    MOV     [EDI],EAX       ; メモリのデータを送信先に格納
    ADD     EDI,4           ; アドレスを4進める
    SUB     ECX,1           ; データのサイズをマイナス1する
    JNZ     memcpy          ; 引き算した結果が0でなければmemcpyへ
    RET
; memcpyはアドレスサイズプリフィクスを入れ忘れなければ、ストリング命令でも書ける

    ALIGNB  16              ; GDT0のアドレスをプロテクトモードに伴い8の倍数に修正
GDT0:                       ; 
    RESB    8               ; nullセクタなので飛ばす
    DW      0xffff,0x0000,0x9200,0x00cf ; 読み書き可能セグメント32bit
    DW      0xffff,0x0000,0x9a28,0x0047 ; 実行可能セグメント32bit (bootpack用)

    DW      0               ; 
GDTR0:                      ; LGDT命令へのヒント
    DW      8*3-1           ; 16ビットのリミット（終わり）
    DD      GDT0            ; 32ビットの開始番地

    ALIGNB  16              ; 16倍にする
bootpack:                   ; bootpackの始まり！