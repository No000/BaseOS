; naskfunc
; TAB=4

[FORMAT "WCOFF"]        ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]      ; 486命令まで使いたいという記述
[BITS 32]               ; 32ビットモード用の機械語を作らせる
[FILE "naskfunc.nas"]   ; ソースファイル名情報
; オブジェクトファイルのための情報
    GLOBAL  _io_hlt,  _io_cli, _io_sti, _io_stihlt   ; このプログラムに含まれる関数名(グローバル関数？)
    GLOBAL  _io_in8,  _io_in16,  _io_in32
    GLOBAL  _io_out8, _io_out16, _io_out32
    GLOBAL  _io_load_eflags, _io_store_eflags
    GLOBAL  _load_gdtr, _load_idtr
    GLOBAL  _load_cr0, _store_cr0
    GLOBAL  _load_tr
    GLOBAL  _asm_inthandler20, _asm_inthandler21 
    GLOBAL  _asm_inthandler27, _asm_inthandler2c
    GLOBAL  _memtest_sub
    GLOBAL  _farjmp, _farcall
    GLOBAL  _asm_hrb_api, _start_app
    EXTERN  _inthandler20, _inthandler21
    EXTERN  _inthandler27, _inthandler2c
    EXTERN  _hrb_api
; 以下は実際の関数
[SECTION .text]         ; オブジェクトファイルではこれを書いてからプログラムを書く

_io_hlt:    ; void io_hlt(void);
    HLT
    RET

_io_cli:    ; void io_cli(void);
    CLI
    RET

_io_sti:    ; void io_sti(void);
    STI
    RET

_io_stihlt: ; void io_stihlt(void);
    STI
    HLT
    RET

_io_in8:    ; int io_in8(int port);
    MOV     EDX,[ESP+4]     ; port：ESP+4なのは、CALLを飛び越えるため
    MOV     EAX,0
    IN      AL,DX
    RET

_io_in16:   ; int io_in16(int port);
    MOV     EDX,[ESP+4]     ; port
    MOV     EAX,0
    IN      AX,DX
    RET

_io_in32:   ; void io_in32(int port);
    MOV     EDX,[ESP+4]     ; port
    IN      EAX,DX
    RET

_io_out8:   ; void io_out8(int port, int data);
    MOV     EDX,[ESP+4]     ; port
    MOV     AL,[ESP+8]      ; data
    OUT     DX,AL
    RET

_io_out16:  ; void io_out16(int port, int data);
    MOV     EDX,[ESP+4]     ; port
    MOV     EAX,[ESP+8]     ; data
    OUT     DX,AX
    RET

_io_out32: ; void io_out32(int port, int data);
    MOV     EDX,[ESP+4]     ; port
    MOV     EAX,[ESP+8]     ; data
    OUT     DX,EAX
    RET

_io_load_eflags: ; void io_load_eflags(void);
    PUSHFD      ; PUSH EFLAGS という意味
    POP     EAX
    RET

_io_store_eflags: ; void io_store_eflags(int eflags);
    MOV     EAX,[ESP+4]
    PUSH    EAX
    POPFD   ; POP EFLAGSという意味
    RET

_load_gdtr:     ; void load_gdtr(int limit, int addr);
    MOV     AX,[ESP+4]      ; limit
    MOV     [ESP+6],AX
    LGDT    [ESP+6]
    RET

_load_idtr:     ; void load_idtr(int limit, int addr);
    MOV     AX,[ESP+4]      ; limit
    MOV     [ESP+6],AX
    LIDT    [ESP+6]
    RET

_load_cr0:      ; void load_cr0(void);
    MOV     EAX,CR0     ; CR0をEAXに格納（読み出し）
    RET

_store_cr0:     ; void store_cr0(int cr0);
    MOV     EAX,[ESP+4] ; スタックポインタのデータをEAXに読み出し
    MOV     CR0,EAX     ; CR0に戻す
    RET

_load_tr:       ; void load_tr(int tr);
    LTR     [ESP+4]     ; 引数trの指定
    RET                 ; Harimainに戻る

_asm_inthandler20:  ;timer
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX,SS
    CMP     AX,1*8
    JNE     .from_app
; OSが動いているときに割り込まれたのでほぼ今までどおり
    MOV     EAX,ESP
    PUSH    SS
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    _inthandler20
    ADD     ESP,8
    POPAD
    POP     DS
    POP     ES
    IRETD
.from_app:
; アプリが動いているときに割り込まれた
    MOV     EAX,1*8
    MOV     DS,AX       ; とりあえずDSだけOS用にする
    MOV     ECX,[0xfe4]     ; OS用のESP
    ADD     ECX,-8
    MOV     [ECX+4],SS      ; 割り込まれたときのSSを保存
    MOV     [ECX  ],ESP     ; 割り込まれたときのESPを保存
    MOV     SS,AX
    MOV     ES,AX
    MOV     ESP,ECX
    CALL    _inthandler20
    POP     ECX
    POP     EAX
    MOV     SS,AX       ; SSをアプリ用に戻す
    MOV     ESP,ECX         ; ESPもアプリ用に戻す
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler21:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX,SS
    CMP     AX,1*8
    JNE     .from_app
; OSが動いているときに割り込まれたのでほぼ今までどおり
    MOV     EAX,ESP
    PUSH    SS
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    _inthandler21
    ADD     ESP,8
    POPAD
    POP     DS
    POP     ES
    IRETD
.from_app:
; アプリが動いているときに割り込まれた
    MOV     EAX,1*8
    MOV     DS,AX       ; とりあえずDSだけOS用にする
    MOV     ECX,[0xfe4]     ; OS用のESP
    ADD     ECX,-8
    MOV     [ECX+4],SS      ; 割り込まれたときのSSを保存
    MOV     [ECX  ],ESP     ; 割り込まれたときのESPを保存
    MOV     SS,AX
    MOV     ES,AX
    MOV     ESP,ECX
    CALL    _inthandler21
    POP     ECX
    POP     EAX
    MOV     SS,AX       ; SSをアプリ用に戻す
    MOV     ESP,ECX         ; ESPもアプリ用に戻す
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler27:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX,SS
    CMP     AX,1*8
    JNE     .from_app
; OSが動いているときに割り込まれたのでほぼ今までどおり
    MOV     EAX,ESP
    PUSH    SS
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    _inthandler27
    ADD     ESP,8
    POPAD
    POP     DS
    POP     ES
    IRETD
.from_app:
; アプリが動いているときに割り込まれた
    MOV     EAX,1*8
    MOV     DS,AX       ; とりあえずDSだけOS用にする
    MOV     ECX,[0xfe4]     ; OS用のESP
    ADD     ECX,-8
    MOV     [ECX+4],SS      ; 割り込まれたときのSSを保存
    MOV     [ECX  ],ESP     ; 割り込まれたときのESPを保存
    MOV     SS,AX
    MOV     ES,AX
    MOV     ESP,ECX
    CALL    _inthandler27
    POP     ECX
    POP     EAX
    MOV     SS,AX       ; SSをアプリ用に戻す
    MOV     ESP,ECX         ; ESPもアプリ用に戻す
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler2c:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX,SS           ; SSのデータをAXに移動
    CMP     AX,1*8
    JNE     .from_app
; OSが動いているときに割り込まれたのでほぼ今までどおり
    MOV     EAX,ESP
    PUSH    SS
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    _inthandler2c
    ADD     ESP,8
    POPAD
    POP     DS
    POP     ES
    IRETD
.from_app:
; アプリが動いているときに割り込まれた
    MOV     EAX,1*8
    MOV     DS,AX       ; とりあえずDSだけOS用にする
    MOV     ECX,[0xfe4]     ; OS用のESP
    ADD     ECX,-8
    MOV     [ECX+4],SS      ; 割り込まれたときのSSを保存
    MOV     [ECX  ],ESP     ; 割り込まれたときのESPを保存
    MOV     SS,AX
    MOV     ES,AX
    MOV     ESP,ECX
    CALL    _inthandler2c
    POP     ECX
    POP     EAX
    MOV     SS,AX       ; SSをアプリ用に戻す
    MOV     ESP,ECX         ; ESPもアプリ用に戻す
    POPAD
    POP     DS
    POP     ES
    IRETD

_memtest_sub:       ; unsigned int memtest_sub(unsigned int start, unsigned int end)
    PUSH    EDI     ; (EBXm, ESI, EDIも使いたいので)
    PUSH    ESI
    PUSH    EBX
    MOV     ESI,0xaa55aa55      ; pat0 = 0xaa55aa55;
    MOV     EDI,0x55aa55aa      ; pat1 = 0x55aa55aa;
    MOV     EAX,[ESP+12+4]      ; i = start;
mts_loop:
    MOV     EBX,EAX
    ADD     EBX,0xffc           ; p = i + 0xffc;
    MOV     EDX,[EBX]           ; old = *p; /* いじる前の値を覚えておく */
    MOV     [EBX],ESI           ; *p = pat0; /* 試しに書いてみる */
    XOR     DWORD [EBX],0xffffffff  ;  *p ^= 0xffffffff /* そしてそれをメモリ上で反転してみる */
    CMP     EDI,[EBX]               ; if (*p != pat1) goto fin; /* 反転結果になってなければ */
    JNE     mts_fin                 ; break
    XOR     DWORD [EBX],0xffffffff  ; *p ^= 0xffffffff /* もう一度反転してみる */
    CMP     ESI,[EBX]               ; if (*p != pat0) goto fin; /* 元に戻ったか？ */
    JNE     mts_fin
    MOV     [EBX],EDX               ; *p = old /* いじった値を元に戻す */
    ADD     EAX,0x1000              ; i += 0x1000; /* forの開始カウンタ */
    CMP     EAX,[ESP+12+8]          ; if (i <= end) goto mts_loop; /* カウンタの上限定義 */
    JBE     mts_loop
    POP     EBX
    POP     ESI
    POP     EDI
    RET
mts_fin:
    MOV     [EBX],EDX               ; *p = old; /* いじった値を元に戻す */
    POP     EBX
    POP     ESI
    POP     EDI
    RET

_farjmp:        ; void farjmp(int eip, int cs);
    JMP     FAR [ESP+4]     ; eip, cs
    RET

_farcall:       ; void farcall(int eip, int cs);
    CALL    FAR [ESP+4]     ; eip, cs(スタックの内容から直接データを渡してる)
    RET

_asm_hrb_api:
    ; 都合の良いことに最初空割り込み禁止になっている
    PUSH    DS
    PUSH    ES
    PUSHAD      ; 保存のためのPUSH
    MOV     EAX,1*8
    MOV     DS,AX       ; とりあえずDSだけOS用にする
    MOV     ECX,[0xfe4]     ; OS用のESP
    ADD     ECX,-40
    MOV     [ECX+32],ESP    ; アプリのESPを保存
    MOV     [ECX+36],SS     ; アプリのSSを保存

; PUSHADした値をシステムのスタックにコピーする
    MOV     EDX,[ESP   ]
    MOV     EBX,[ESP+ 4]
    MOV     [ECX   ],EDX    ; hrb_apiに渡すためコピー
    MOV     [ECX+ 4],EBX    ; hrb_apiに渡すためコピー
    MOV     EDX,[ESP+ 8]
    MOV     EBX,[ESP+12]
    MOV     [ECX+ 8],EDX    ; hrb_apiに渡すためコピー
    MOV     [ECX+12],EBX    ; hrb_apiに渡すためコピー
    MOV     EDX,[ESP+16]
    MOV     EBX,[ESP+20]
    MOV     [ECX+16],EDX    ; hrb_apiに渡すためコピー
    MOV     [ECX+20],EBX    ; hrb_apiに渡すためコピー
    MOV     EDX,[ESP+24]
    MOV     EBX,[ESP+28]
    MOV     [ECX+24],EDX    ; hrb_apiに渡すためコピー
    MOV     [ECX+28],EBX    ; hrb_apiに渡すためコピー

    MOV     ES,AX
    MOV     SS,AX
    MOV     ESP,ECX
    STI         ; やっと割り込み許可

    CALL    _hrb_api

    MOV     ECX,[ESP+32]    ; アプリのESPを思い出す
    MOV     EAX,[ESP+36]    ; アプリのSSを思い出す
    CLI
    MOV     SS,AX
    MOV     ESP,ECX
    POPAD
    POP     ES
    POP     DS
    IRETD   ; この命令が自動でSTIしてくれる

_start_app:     ; void start_app(int eip, int cs, int esp, int ds);
    PUSHAD      ; 32ビットレジスタを全部保存しておく
    MOV     EAX,[ESP+36]    ; アプリ用のEIP
    MOV     ECX,[ESP+40]    ; アプリ用のCS
    MOV     EDX,[ESP+44]    ; アプリ用のESP
    MOV     EBX,[ESP+48]    ; アプリ用のDS/SS
    MOV     [0xfe4],ESP     ; OS用のESP(アドレス)
    CLI     ; 切り替え中に割り込みが起きてほしくないので禁止
    MOV     ES,BX   ; 念のため
    MOV     SS,BX
    MOV     DS,BX
    MOV     FS,BX   ; 念のため
    MOV     GS,BX   ; 念のため
    MOV     ESP,EDX ; アプリ用のESPを格納
    STI         ; 切り替え完了なので割り込み可能に戻す
    PUSH    ECX        ; far-CALLのためにPUSH(cs)
    PUSH    EAX        ; far-CALLのためにPUSH(eip)
    CALL    FAR [ESP]   ; アプリを呼び出す

; アプリが終了するトここに帰ってくる(メインルーチンに戻る)

    MOV     EAX,1*8     ; OS用のDS
    CLI         ; また切り替えるので割り込み禁止
    MOV     ES,AX
    MOV     SS,AX
    MOV     DS,AX
    MOV     FS,AX
    MOV     GS,AX
    MOV     ESP,[0xfe4]
    STI         ; 切り替え完了なので割り込み可能に戻す
    POPAD   ; 保存しておいたレジスタを回復
    RET
