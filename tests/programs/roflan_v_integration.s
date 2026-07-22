.option norvc
.section .text
.globl _start
_start:
    lui x31, 0x1
    addi x1, x0, 5
    addi x2, x0, 7
    add x3, x1, x2
    addi x30, x0, 12
    bne x3, x30, fail_1
    addi x4, x0, 10
    addi x5, x4, 1
    addi x30, x0, 11
    bne x5, x30, fail_2
    addi x6, x0, 13
    add x7, x0, x6
    addi x30, x0, 13
    bne x7, x30, fail_3
    addi x8, x0, 1
    addi x9, x8, 1
    addi x10, x9, 1
    addi x11, x10, 1
    addi x30, x0, 4
    bne x11, x30, fail_4
    addi x0, x0, 123
    addi x12, x0, 5
    addi x30, x0, 5
    bne x12, x30, fail_5
    lui x13, 0x12345
    lui x30, 0x12345
    bne x13, x30, fail_6
auipc_site:
    auipc x14, 0x1
    lui x14, 0x2
    addi x15, x0, 42
    sw x15, 0(x14)
    lw x30, 0(x14)
    bne x15, x30, fail_7
    lui x16, 0x2
    addi x16, x16, 16
    addi x17, x0, 55
    sw x17, 0(x16)
    lw x30, 0(x16)
    bne x17, x30, fail_8
    lui x18, 0x2
    addi x19, x0, 77
    sw x19, 32(x18)
    lw x20, 32(x18)
    bne x19, x20, fail_9
    lui x21, 0x2
    addi x22, x0, 41
    sw x22, 64(x21)
    lw x23, 64(x21)
    addi x24, x23, 1
    addi x30, x0, 42
    bne x24, x30, fail_10
    addi x22, x0, 128
    sb x22, 81(x21)
    lb x23, 81(x21)
    addi x30, x0, -128
    bne x23, x30, fail_11
    lbu x24, 81(x21)
    addi x30, x0, 128
    bne x24, x30, fail_12
    sb x22, 83(x21)
    lb x23, 83(x21)
    addi x30, x0, -128
    bne x23, x30, fail_13
    lui x22, 0x8
    addi x22, x22, 1
    sh x22, 96(x21)
    lh x23, 96(x21)
    lui x30, 0xffff8
    addi x30, x30, 1
    bne x23, x30, fail_14
    lhu x24, 96(x21)
    bne x24, x22, fail_15
    sh x22, 98(x21)
    lh x23, 98(x21)
    bne x23, x30, fail_16
    lhu x24, 98(x21)
    bne x24, x22, fail_17
    addi x25, x0, 1
    beq x25, x0, fail_18
    addi x26, x0, 7
    addi x27, x0, 0
    addi x28, x0, 1
    beq x28, x28, branch_taken
    addi x27, x0, 99
branch_taken:
    addi x28, x28, 1
    bne x27, x0, fail_19
jal_site:
    jal x1, jal_target
    addi x27, x0, 88
jal_target:
    addi x2, x0, 9
    bne x27, x0, fail_20
jalr_base:
    auipc x29, 0
    addi x29, x29, 17
jalr_site:
    jalr x30, 0(x29)
    addi x27, x0, 77
jalr_target:
    addi x3, x0, 21
    bne x27, x0, fail_21
success:
    lui x30, 0x600d6
    addi x30, x30, 13
    sw x30, 0(x31)
success_loop:
    jal x0, success_loop

fail_1:  lui x30, 0xbad00; addi x30, x30, 1;  jal x0, fail_store
fail_2:  lui x30, 0xbad00; addi x30, x30, 2;  jal x0, fail_store
fail_3:  lui x30, 0xbad00; addi x30, x30, 3;  jal x0, fail_store
fail_4:  lui x30, 0xbad00; addi x30, x30, 4;  jal x0, fail_store
fail_5:  lui x30, 0xbad00; addi x30, x30, 5;  jal x0, fail_store
fail_6:  lui x30, 0xbad00; addi x30, x30, 6;  jal x0, fail_store
fail_7:  lui x30, 0xbad00; addi x30, x30, 7;  jal x0, fail_store
fail_8:  lui x30, 0xbad00; addi x30, x30, 8;  jal x0, fail_store
fail_9:  lui x30, 0xbad00; addi x30, x30, 9;  jal x0, fail_store
fail_10: lui x30, 0xbad00; addi x30, x30, 10; jal x0, fail_store
fail_11: lui x30, 0xbad00; addi x30, x30, 11; jal x0, fail_store
fail_12: lui x30, 0xbad00; addi x30, x30, 12; jal x0, fail_store
fail_13: lui x30, 0xbad00; addi x30, x30, 13; jal x0, fail_store
fail_14: lui x30, 0xbad00; addi x30, x30, 14; jal x0, fail_store
fail_15: lui x30, 0xbad00; addi x30, x30, 15; jal x0, fail_store
fail_16: lui x30, 0xbad00; addi x30, x30, 16; jal x0, fail_store
fail_17: lui x30, 0xbad00; addi x30, x30, 17; jal x0, fail_store
fail_18: lui x30, 0xbad00; addi x30, x30, 18; jal x0, fail_store
fail_19: lui x30, 0xbad00; addi x30, x30, 19; jal x0, fail_store
fail_20: lui x30, 0xbad00; addi x30, x30, 20; jal x0, fail_store
fail_21: lui x30, 0xbad00; addi x30, x30, 21; jal x0, fail_store
fail_store:
    sw x30, 0(x31)
fail_loop:
    jal x0, fail_loop
