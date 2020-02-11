/* GDTやIDTなどのdescriptor table 関係 */
#include "bootpack.h"

/* GDT・IDTの初期化 */
void init_gdtidt(void)
{
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT; /* GDT:0x00270000~0x0027ffff */
    struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT; /* IDT:0x0026f800~0x0026ffff */
    int i;

    /* GDTの初期化 */
    for (i = 0; i <= LIMIT_GDT / 8; i++) {
        set_segmdesc(gdt + i, 0, 0, 0); /* for文でリミット（セグメントのマイナス１）、ベース（あどれす）、アクセス権属性を初期化 */
    }
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW); /* CPUの全メモリ表示（４GB） */
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER); /* bootpack.hrbの格納（512KB） */
    load_gdtr(LIMIT_GDT, ADR_GDT); /* アセンブリのラベル_load_gdtrの実行 */

    /* IDTの初期化 */
    for (i = 0; i <= LIMIT_IDT / 8; i++) {
        set_gatedesc(idt + i, 0, 0, 0); /* IDTの初期化 */
    }
    load_idtr(LIMIT_IDT, ADR_IDT); /* アセンブリのラベル_load_idtrの実行 */

    /* IDTの設定 */
    set_gatedesc(idt + 0x0c, (int) asm_inthandler0c, 2 * 8, AR_INTGATE32);  /* 0x0c番目にスタック例外の割り込みを設定 */
    set_gatedesc(idt + 0x0d, (int) asm_inthandler0d, 2 * 8, AR_INTGATE32);  /* 0x0d番目に一般保護例外の割り込みを設定 */
    set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 * 8, AR_INTGATE32);  /* 0x20番目にタイマー割り込みを設定 */
    set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);  /* 0x21番目にasm_inthandler21を登録 */
    set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);  /* 0x27番目にasm_inthandler27を登録 */
    set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);  /* 0x2c番目にasm_inthandler2cを登録 */
    set_gatedesc(idt + 0x40, (int) asm_hrb_api,      2 * 8, AR_INTGATE32 + 0x60);  /* 0x40番目にasm_hrb_apiを登録 */

    return;
}

void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
    if (limit > 0xfffff) {
        ar |= 0x8000; /* G_bit = 1 */
        limit /= 0x1000;
    }
    sd->limit_low    = limit & 0xffff;
    sd->base_low     = base & 0xffff;
    sd->base_mid     = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_high    = (base >> 24) & 0xff;
    return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
    gd->offset_low   = offset & 0xffff;
    gd->selector     = selector;
    gd->dw_count     = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high  = (offset >> 16) & 0xffff;
    return;
}
