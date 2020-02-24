[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api021.nas"]    ; ソースファイル名情報

    GLOBAL  _api_fopen

[SECTION .text]

_api_fopen:     ; int api_fopen(char *fname);
    PUSH  EBX
    MOV   EDX,21
    MOV   EBX,[ESP+8]     ; fname
    INT   0x40
    POP   EBX
    RET
