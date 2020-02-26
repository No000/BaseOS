[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api026.nas"]    ; ソースファイル名情報

    GLOBAL  _api_cmdline

[SECTION .text]

_api_cmdline:   ; int api_cmdline(char *buf, int maxsize);
    PUSH    EBX
    MOV     EDX,26
    MOV     ECX,[ESP+12]    ; mazsize
    MOV     EBX,[ESP+8]     ; buf
    INT     0x40
    POP     EBX
    RET
