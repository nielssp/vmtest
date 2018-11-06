#ifndef INS_H
#define INS_H

#define INS_NOOP  0
#define INS_EXIT  1
#define INS_PSH1  2
#define INS_PSH2  3
#define INS_PSH4  4
#define INS_PSH8  5
#define INS_LOD1  6
#define INS_LOD2  7
#define INS_LOD4  8
#define INS_LOD8  9
#define INS_STO1 10
#define INS_STO2 11
#define INS_STO4 12
#define INS_STO8 13
#define INS_OPI1 14
#define INS_OPI2 15
#define INS_OPI4 16
#define INS_OPI8 17
#define INS_OPU1 18
#define INS_OPU2 19
#define INS_OPU4 20
#define INS_OPU8 21
#define INS_OPF4 22
#define INS_OPF8 23
#define INS_INSP 24
#define INS_JUMP 25
#define INS_JMPC 26
#define INS_CALL 27
#define INS_CRET 28
#define INS_SYSC 29

#define INS_PSHN 30
#define INS_PSHA 31
#define INS_PSHS 35
#define INS_LODV 32
#define INS_STOV 33
#define INS_OPVA 34

#define INS_FORK

#define OPR_DUP 0x00
#define OPR_NEG 0x01
#define OPR_ADD 0x02
#define OPR_SUB 0x03
#define OPR_MUL 0x04
#define OPR_DIV 0x05
#define OPR_MOD 0x06
#define OPR_EQL 0x07
#define OPR_NEQ 0x08
#define OPR_LTH 0x09
#define OPR_LEQ 0x0A
#define OPR_GTH 0x0B
#define OPR_GEQ 0x0C
#define OPR_CON 0x0D
#define OPR_CHD 0x0E
#define OPR_CTL 0x0F
#define OPR_ARR 0x10
#define OPR_AGT 0x11
#define OPR_AST 0x12
#define OPR_CI1 0x13
#define OPR_CI2 0x14
#define OPR_CI4 0x15
#define OPR_CI8 0x16
#define OPR_CU1 0x17
#define OPR_CU2 0x18
#define OPR_CU4 0x19
#define OPR_CU8 0x1A
#define OPR_CF4 0x1B
#define OPR_CF8 0x1C

#define OPR_SND
#define OPR_RCV

#endif
