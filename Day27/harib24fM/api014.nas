[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api014.nas"]    ; ソースファイル名情報

    GLOBAL  _api_closewin

[SECTION .text]

_api_closewin:      ; void _api_closewin(int win);
    PUSH    EBX
    MOV     EDX,14
    MOV     EBX,[ESP+8] ; win
    INT     0x40
    POP     EBX
    RET
