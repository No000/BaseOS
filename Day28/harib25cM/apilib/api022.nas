[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api022.nas"]    ; ソースファイル名情報

    GLOBAL  _api_fclose

[SECTION .text]

_api_fclose:    ; void api_fclose(int fhandle);
    MOV   EDX,22
    MOV   EAX,[ESP+4]     ; fhandle
    INT   0x40
    RET
