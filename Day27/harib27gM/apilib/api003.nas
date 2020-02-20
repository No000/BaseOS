[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api003.nas"]    ; ソースファイル名情報

    GLOBAL  _api_putstr1

[SECTION .text]

_api_putstr1: ; void api_putstr1(char *s, int l);
    PUSH  EBX
    MOV   EDX, 3
    MOV   EBX,[ESP+ 8]  ; s
    MOV   ECX,[ESP+12]  ; l
    INT   0x40
    POP   EBX
    RET
