[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api017.nas"]    ; ソースファイル名情報

    GLOBAL  _api_inittimer

[SECTION .text]

_api_inittimer:    ; void api_inittimer(int timer, int data);
    PUSH    EBX
    MOV     EDX,17
    MOV     EBX,[ESP+ 8]    ; timer
    MOV     EAX,[ESP+12]    ; data
    INT     0x40
    POP     EBX
    RET
