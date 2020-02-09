[INSTRSET "i486p"]
[BITS 32]
    CLI   ; フリーズ攻撃
fin:
    HLT
    JMP   fin
