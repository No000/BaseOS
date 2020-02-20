[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api005.nas"]    ; ソースファイル名情報

    GLOBAL  _api_openwin

[SECTION .text]

_api_openwin: ; int _api_openwin(char *buf, int xsiz, int ysiz, int col_inv, char *title);
    PUSH    EDI
    PUSH    ESI
    PUSH    EBX
    MOV     EDX,5
    MOV     EBX,[ESP+16]    ; buf
    MOV     ESI,[ESP+20]    ; xsiz
    MOV     EDI,[ESP+24]    ; ysiz
    MOV     EAX,[ESP+28]    ; col_inv
    MOV     ECX,[ESP+32]    ; title
    INT     0x40
    POP     EBX
    POP     ESI
    POP     EDI
    RET
