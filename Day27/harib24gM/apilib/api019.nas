[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api019.nas"]    ; ソースファイル名情報

    GLOBAL  _api_freetimer

[SECTION .text]

_api_freetimer:     ; void api_freetimer(int timer);
    PUSH    EBX
    MOV     EDX,19
    MOV     EBX,[ESP+ 8]    ; timer
    INT     0x40
    POP     EBX
    RET
