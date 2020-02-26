[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api024.nas"]    ; ソースファイル名情報

    GLOBAL  _api_fsize

[SECTION .text]

_api_fsize:     ; int api_fsize(int fhandle, int mode);
    MOV   EDX,24
    MOV   EAX,[ESP+4]     ; fhandle
    MOV   ECX,[ESP+8]     ; mode
    INT   0x40
    RET
