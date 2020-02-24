[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api018.nas"]    ; ソースファイル名情報

    GLOBAL  _api_settimer

[SECTION .text]

_api_settimer:      ; void api_settimer(int timer, int time);
    PUSH    EBX
    MOV     EDX,18
    MOV     EBX,[ESP+ 8]    ; timer
    MOV     EAX,[ESP+12]    ; time
    INT     0x40
    POP     EBX
    RET
