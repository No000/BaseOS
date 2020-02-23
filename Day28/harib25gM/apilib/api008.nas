[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api008.nas"]    ; ソースファイル名情報

    GLOBAL  _api_initmalloc

[SECTION .text]

_api_initmalloc:    ; void _api_initmalloc(void);
    PUSH    EBX
    MOV     EDX,8
    MOV     EBX,[CS:0x0020]     ; malloc領域の番地
    MOV     EAX,EBX
    ADD     EAX,32*1024     ; 32KBを足す
    MOV     ECX,[CS:0x0000] ; データセグメントの大きさ
    SUB     ECX,EAX
    INT     0x40
    POP     EBX
    RET
