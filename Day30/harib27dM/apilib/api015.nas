[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api015.nas"]    ; ソースファイル名情報

    GLOBAL  _api_getkey

[SECTION .text]

_api_getkey:        ; int api_getkey(int mode);
    MOV     EDX,15
    MOV     EAX,[ESP+4] ; mode
    INT     0x40
    RET
