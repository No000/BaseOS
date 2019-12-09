; naskfunk
; TAB=4

[FORMAT "WCOFF"]        ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]      ; 486命令まで使いたいという記述
[BITS 32]               ; 32ビットモード用の機械語を作らせる
[FILE "naskfunk.nas"]   ; ソースファイル名情報

    GLOBAL  _io_hlt,_write_mem8     ; このプログラムに含まれる関数名(グローバル関数？)

; オブジェクトファイルのための情報



; 以下は実際の関数

[SECTION .text]         ; オブジェクトファイルではこれを書いてからプログラムを書く


_io_hlt:  ; void io_hlt(void);
    HLT
    RET


_write_mem8:    ; void write_mem8(int addr, int data);
    MOV     ECX,[ESP+4]     ; [ESP+4]にaddrが入っているのでそれをECXに読み込む（スタック領域を参照）
    MOV     AL,[ESP+8]      ; [ESP+8]にdataが入っているのでそれをALに読み込む
    MOV     [ECX],AL
    RET