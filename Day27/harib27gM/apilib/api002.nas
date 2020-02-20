[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api002.nas"]    ; ソースファイル名情報

    GLOBAL  _api_putstr0

[SECTION .text]

_api_putstr0:   ; void _api_putstr0(char *s);
    PUSH    EBX
    MOV     EDX,2
    MOV     EBX,[ESP+8]     ; s
    INT     0x40
    POP     EBX
    RET
