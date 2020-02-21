[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api016.nas"]    ; ソースファイル名情報

    GLOBAL  _api_alloctimer

[SECTION .text]

_api_alloctimer:    ; int api_alloctimer(void);
    MOV     EDX,16
    INT     0x40
    RET
