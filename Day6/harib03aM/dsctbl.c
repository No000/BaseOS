/* GDTやIDTなどのdescriptor table 関係 */

/* GDTの中身（2*2 + 1*2 + 2 = 8バイト） */
struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
/* IDTの中身（2*2 + 1*2 + 2 = 8バイト） */
struct GATE_DESCRIPTOR{
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
/* ヘッダー関連 */
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
/* GDT・IDTの初期化 */
void init_gdtidt(void)
{
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) 0x00270000; /* GDT:0x00270000~0x0027ffff */
    struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) 0x0026f800; /* IDT:0x0026f800~0x0026ffff */
    int i;

    /* GDTの初期化 */
    for (i = 0; i < 8192; i++) {
        set_segmdesc(gdt + i, 0, 0, 0); /* for文でリミット（セグメントのマイナス１）、ベース（あどれす）、アクセス権属性を初期化 */
    }
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092); /* CPUの全メモリ表示（４GB） */
    set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a); /* bootpack.hrbの格納（512KB） */
    load_gdtr(0xffff, 0x00270000); /* アセンブリのラベル_load_gdtrの実行 */

    /* IDTの初期化 */
    for (i = 0; i < 256; i++) {
        set_gatedesc(idt + i, 0, 0, 0); /* IDTの初期化 */
    }
    load_idtr(0x7ff, 0x0026f800); /* アセンブリのラベル_load_idtrの実行 */

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
