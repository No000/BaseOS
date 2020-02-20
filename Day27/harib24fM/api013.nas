[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api013.nas"]    ; ソースファイル名情報

    GLOBAL  _api_linewin

[SECTION .text]

_api_linewin:   ; void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
    PUSH    EDI
    PUSH    ESI
    PUSH    EBP
    PUSH    EBX
    MOV     EDX,13
    MOV     EBX,[ESP+20]    ; win
    MOV     EAX,[ESP+24]    ; x0
    MOV     ECX,[ESP+28]    ; y0
    MOV     ESI,[ESP+32]    ; x1
    MOV     EDI,[ESP+36]    ; y1
    MOV     EBP,[ESP+40]    ; col
    INT     0x40
    POP     EBX
    POP     EBP
    POP     ESI
    POP     EDI
    RET
