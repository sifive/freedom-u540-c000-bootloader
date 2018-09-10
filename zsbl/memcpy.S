.section .text.memcpy
.global memcpy
.type memcpy, %function
.func memcpy
memcpy:
/* 108e6 */ xor     a5,a1,a0
/* 108ea */ andi    a5,a5,7
/* 108ec */ add     a7,a0,a2
/* 108f0 */ bnez    a5,6f    # 10938 <memcpy+0x52>
/* 108f2 */ li      a5,7
/* 108f4 */ bleu    a2,a5,6f # 10938 <memcpy+0x52>
/* 108f8 */ andi    a4,a0,7
/* 108fc */ mv      a5,a0
/* 108fe */ bnez    a4,8f    # 10950 <memcpy+0x6a>
1:
/* 10900 */ andi    a6,a7,-8
/* 10904 */ addi    a4,a6,-64
/* 10908 */ bltu    a5,a4,9f # 10976 <memcpy+0x90>
2:
/* 1090c */ mv      a3,a1
/* 1090e */ mv      a4,a5
/* 10910 */ bleu    a6,a5,4f # 10932 <memcpy+0x4c>
3:
/* 10914 */ ld      a2,0(a3)
/* 10916 */ addi    a4,a4,8
/* 10918 */ addi    a3,a3,8
/* 1091a */ sd      a2,-8(a4)
/* 1091e */ bltu    a4,a6,3b # 10914 <memcpy+0x2e>
/* 10922 */ not     a4,a5
/* 10926 */ add     a6,a6,a4
/* 10928 */ andi    a6,a6,-8
/* 1092c */ addi    a6,a6,8
/* 1092e */ add     a5,a5,a6
/* 10930 */ add     a1,a1,a6
4:
/* 10932 */ bltu    a5,a7,7f # 1093e <memcpy+0x58>
5:
/* 10936 */ ret
6:
/* 10938 */ mv      a5,a0
/* 1093a */ bleu    a7,a0,5b # 10936 <memcpy+0x50>
7:
/* 1093e */ lbu     a4,0(a1)
/* 10942 */ addi    a5,a5,1
/* 10944 */ addi    a1,a1,1
/* 10946 */ sb      a4,-1(a5)
/* 1094a */ bltu    a5,a7,7b # 1093e <memcpy+0x58>
/* 1094e */ ret
8:
/* 10950 */ lbu     a3,0(a1)
/* 10954 */ addi    a5,a5,1
/* 10956 */ andi    a4,a5,7
/* 1095a */ sb      a3,-1(a5)
/* 1095e */ addi    a1,a1,1
/* 10960 */ beqz    a4,1b    # 10900 <memcpy+0x1a>
/* 10962 */ lbu     a3,0(a1)
/* 10966 */ addi    a5,a5,1
/* 10968 */ andi    a4,a5,7
/* 1096c */ sb      a3,-1(a5)
/* 10970 */ addi    a1,a1,1
/* 10972 */ bnez    a4,8b    # 10950 <memcpy+0x6a>
/* 10974 */ j       1b       # 10900 <memcpy+0x1a>
9:
/* 10976 */ ld      t2,0(a1)
/* 1097a */ ld      t0,8(a1)
/* 1097e */ ld      t6,16(a1)
/* 10982 */ ld      t5,24(a1)
/* 10986 */ ld      t4,32(a1)
/* 1098a */ ld      t3,40(a1)
/* 1098e */ ld      t1,48(a1)
/* 10992 */ ld      a2,56(a1)
/* 10994 */ addi    a1,a1,72
/* 10998 */ addi    a5,a5,72
/* 1099c */ ld      a3,-8(a1)
/* 109a0 */ sd      t2,-72(a5)
/* 109a4 */ sd      t0,-64(a5)
/* 109a8 */ sd      t6,-56(a5)
/* 109ac */ sd      t5,-48(a5)
/* 109b0 */ sd      t4,-40(a5)
/* 109b4 */ sd      t3,-32(a5)
/* 109b8 */ sd      t1,-24(a5)
/* 109bc */ sd      a2,-16(a5)
/* 109c0 */ sd      a3,-8(a5)
/* 109c4 */ bltu    a5,a4,9b # 10976 <memcpy+0x90>
/* 109c8 */ j       2b       # 1090c <memcpy+0x26>
.short 0
.size memcpy, . - memcpy
.endfunc
