[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api020.nas"]    ; ソースファイル名情報

    GLOBAL  _api_beep

[SECTION .text]

_api_beep:          ; void api_beep(int tone);
    MOV     EDX,20  ; API指定
    MOV     EAX,[ESP+4]     ; tone
    INT     0x40
    RET
