[BITS 32]
  MOV   AL,'A'
  CALL    2*8:0xbe3 ; セグメントの壁を超えるのでfarなCALL
fin:
  HLT
  JMP   fin
