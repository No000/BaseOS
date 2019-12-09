/* bootpack�̃��C�� */

#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo;

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32];
    int mx, my, i;

    init_gdtidt(); /* GDT�AIDT�̏����� */
    init_pic(); /* PIC�̏����� */
    io_sti();   /* IDT/PIC�̏��������I������̂�CPU�̊��荞�݋֎~������ */

    fifo8_init(&keyfifo, 32, keybuf);
    io_out8(PIC0_IMR, 0xf9); /* PIC1�ƃL�[�{�[�h�����i1111_10001�j */
    io_out8(PIC1_IMR, 0xef); /* �}�E�X�����i1110_1111�j */

    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2; /* ��ʒ����ɂȂ�x���W */
    my = (binfo->scrny - 28 -16) / 2; /* ��ʒ����ɂȂ�y���W */
    init_mouse_cursor8(mcursor, COL8_008484);   /* �}�E�X�`�� */
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my); /* �}�E�X�̈ʒu����ʂɃf�o�b�O */
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
    /* �L�[�{�[�h�ƃ}�E�X�̋��͏�Ɉړ� */
    /* ���z�I�Ȋ��荞�ݏ��� */

    for (;;) {
        io_cli(); /* �O�����荞�݋֎~�i���荞�ݏ������̊��荞�ݑ΍�j */
        if (fifo8_status(&keyfifo) == 0) { /* �o�b�t�@���󂩂̊m�F */
            io_stihlt(); /* �O�����荞�݂̋��ƁACPU��~����(���荞�݂̏I��) */
        } else {
            i = fifo8_get(&keyfifo); /* �L�[�R�[�h�̃A�h���X��ϐ�i�Ɋi�[ */
            io_sti(); /* IF��1���Z�b�g�i�O�����荞�݂̋��j */ 
            sprintf(s, "%02X", i); /* �������̃f�[�^�̎Q�� */
            boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31); /* ��ʂ̃��Z�b�g */
            putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s); /* �����̕\�� */
        }
    }
}
