[FORMAT "WCOFF"]      ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]    ; 486の命令まで使いたいという記述
[BITS 32]             ; 32ビットモード要の機械語を作らせる
[FILE "api023.nas"]    ; ソースファイル名情報

    GLOBAL  _api_fseek

[SECTION .text]

_api_fseek:     ; void api_fseek(int fhandle, int offset, int mode);
  PUSH    EBX
  MOV     EDX,23
  MOV     EAX,[ESP+8]   ; fhandle
  MOV     ECX,[ESP+16]  ; mode
  MOV     EBX,[ESP+12]  ; offset
  INT     0x40
  POP     EBX
  RET
