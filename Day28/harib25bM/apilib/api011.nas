[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api011.nas"]    ; ソースファイル名情報

    GLOBAL  _api_point

[SECTION .text]

_api_point:         ; void api_point(int win, int x, int y, int col);
    PUSH    EDI
    PUSH    ESI
    PUSH    EBX
    MOV     EDX,11
    MOV     EBX,[ESP+16]    ; win
    MOV     ESI,[ESP+20]    ; x
    MOV     EDI,[ESP+24]    ; y
    MOV     EAX,[ESP+28]    ; col
    INT     0x40
    POP     EBX
    POP     ESI
    POP     EDI
    RET
