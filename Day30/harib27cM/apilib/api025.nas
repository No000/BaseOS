[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api025.nas"]    ; ソースファイル名情報

    GLOBAL  _api_fread

[SECTION .text]

_api_fread:     ; int api_fread(char *buf, int maxsize, int fhandke);
    PUSH    EBX
    MOV     EDX,25
    MOV     EAX,[ESP+16]      ; fhandle
    MOV     ECX,[ESP+12]      ; maxsize
    MOV     EBX,[ESP+8]       ; buf
    INT     0x40
    POP     EBX
    RET
