[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "a_nask.nas"]    ; ソースファイル名情報

    GLOBAL  _api_putchar  ; 参照可能にする

[SECTION .text]

_api_putchar: ; void api_putchar(int c);
    MOV   EDX,1
    MOV   AL,[ESP+4]    ; c(引数回収)
    INT   0x40
    RET
