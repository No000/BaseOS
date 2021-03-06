[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "a_nask.nas"]    ; ソースファイル名情報

    GLOBAL  _api_putchar  ; 参照可能にする
    GLOBAL  _api_putstr0
    GLOBAL  _api_end
    GLOBAL  _api_openwin

[SECTION .text]

_api_putchar: ; void api_putchar(int c);
    MOV     EDX,1
    MOV     AL,[ESP+4]    ; c(引数回収)
    INT     0x40
    RET

_api_putstr0:   ; void _api_putstr0(char *s);
    PUSH    EBX
    MOV     EDX,2
    MOV     EBX,[ESP+8]     ; s
    INT     0x40
    POP     EBX
    RET

_api_end:   ; void api_end(void);
    MOV     EDX,4
    INT     0x40

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