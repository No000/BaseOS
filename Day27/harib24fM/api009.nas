[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api009.nas"]    ; ソースファイル名情報

    GLOBAL  _api_malloc

[SECTION .text]

_api_malloc:        ; char *_api_malloc(int size);
    PUSH    EBX
    MOV     EDX,9
    MOV     EBX,[CS:0x0020]
    MOV     ECX,[ESP+8]     ; size
    INT     0x40
    POP     EBX
    RET
