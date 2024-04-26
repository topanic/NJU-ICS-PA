/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

extern void display_call_func(word_t pc, word_t func_addr);
extern void display_ret_func(word_t pc);


#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
    TYPE_I, TYPE_U, TYPE_S, TYPE_R,
    TYPE_J, TYPE_B,
    TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)

/*
 * There is a point that is easily overlooked here.
 * The lowest bits of B and U are 1, while the others are all 0,
 * so in the end you need to shift left by 1.
 * It may be difficult to pay attention here.
 * */
//#define immR() do { /* R-type instructions do not have an immediate value */ } while(0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immB() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 12) | BITS(i, 7, 7) << 11 | \
                          BITS(i, 30, 25) << 5 | \
                          BITS(i, 11, 8) << 1; } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | BITS(i, 19, 12) << 12 | \
                          BITS(i, 20, 20) << 11 | \
                          BITS(i, 30, 21) << 1; } while(0)


static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
    uint32_t i = s->isa.inst.val;
    int rs1 = BITS(i, 19, 15);
    int rs2 = BITS(i, 24, 20);
    *rd = BITS(i, 11, 7);
    switch (type) {
        case TYPE_R:
            src1R();
            src2R();
            break;
        case TYPE_I:
            src1R();
            immI();
            break;
        case TYPE_S:
            src1R();
            src2R();
            immS();
            break;
        case TYPE_B:
            src1R();
            src2R();
            immB();
            break;
        case TYPE_U:
            immU();
            break;
        case TYPE_J:
            immJ();
            break;
        case TYPE_N:
            break;
        default:
            Assert(0, "Don't match instruction formats, something wrong.");
    }
}

// for 'trap'
static vaddr_t *csr_register(word_t imm) {
    switch (imm) {
        case 0x341:
            return &(cpu.csr.mepc);
        case 0x342:
            return &(cpu.csr.mcause);
        case 0x300:
            return &(cpu.csr.mstatus);
        case 0x305:
            return &(cpu.csr.mtvec);
        default:
            panic("Unknown csr");
    }
}

//
// this part below is for TRAP and CSR.
//

static void etrace_info(Decode *s) {
#ifdef CONFIG_ETRACE
    bool success;
    printf("\n" ANSI_FMT("[ETRACE]", ANSI_FG_YELLOW) " ecall at %#x, cause: %d", s->pc, isa_reg_str2val("a7", &success));
    assert(success == true);
#endif
}

#define ECALL(dnpc) {bool success; dnpc = (isa_raise_intr(isa_reg_str2val("a7", &success), s->pc)); assert(success == true);}
#define CSR(i) *csr_register(i)


static int decode_exec(Decode *s) {
    int rd = 0;
    word_t src1 = 0, src2 = 0, imm = 0;
    s->dnpc = s->snpc;

    // Both macros are used by the following macros(INSTPAT) defined in decode.h
#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

    INSTPAT_START();
        // pattern | key | mask | shift
        /* R */
        INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra, R,
                R(rd) = (SEXT(BITS(src1, 31, 31), 1) << (32 - src2)) | (src1 >> src2));
        INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl, R, R(rd) = src1 >> src2);
        INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem, R, R(rd) = (int32_t) src1 % (int32_t) src2);
        INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu, R, R(rd) = src1 % src2);
        INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu, R, R(rd) = (uint32_t) src1 / (uint32_t) src2);
        INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div, R, R(rd) = (int32_t) src1 / (int32_t) src2);
        INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul, R, R(rd) = src1 * src2);
        INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh, R,
                int32_t a = src1; int32_t b = src2; int64_t tmp = (int64_t) a * (int64_t) b; R(rd) = BITS(tmp, 63, 32));
        INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu, R,
                uint64_t tmp = (uint64_t) src1 * (uint64_t) src2; R(rd) = BITS(tmp, 63, 32));
        INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and, R, R(rd) = src1 & src2);
        INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll, R, R(rd) = src1 << src2);
        INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, R(rd) = src1 + src2);
        INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub, R, R(rd) = src1 - src2);
        INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu, R, R(rd) = (uint32_t) src1 < (uint32_t) src2 ? 1 : 0;);
        INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt, R, R(rd) = (int) src1 < (int) src2 ? 1 : 0);
        INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor, R, R(rd) = src1 ^ src2);
        INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or, R, R(rd) = src1 | src2);

        /* I */
        INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, R(rd) = Mr(src1 + imm, 1));
        INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, R(rd) = SEXT(Mr(src1 + imm, 1), 16));
        INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, R(rd) = SEXT(Mr(src1 + imm, 2), 16));
        INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, R(rd) = Mr(src1 + imm, 2));
        INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, R(rd) = Mr(src1 + imm, 4));
        INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi, I, R(rd) = imm & src1);
        INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori, I, R(rd) = src1 ^ imm);
        INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori, I, R(rd) = src1 | imm);
        INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, R(rd) = src1 + imm);
        INSTPAT("??????? ????? ????? 000 ????? 00100 11", li, I, R(rd) = src1 + imm);
        INSTPAT("0000000 ????? ????? 001 ????? 00100 11", elli, I, R(rd) = src1 << imm);
        INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu, I, R(rd) = (uint32_t) src1 < (uint32_t) imm ? 1 : 0);
        INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti, I, R(rd) = (int32_t) src1 < (int32_t) imm ? 1 : 0);
        INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli, I, R(rd) = src1 >> imm);
        INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai, I,
                imm = BITS(imm, 4, 0); R(rd) = (SEXT(BITS(src1, 31, 31), 1) << (32 - imm)) | (src1 >> imm));
        INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I,
                R(rd) = s->pc + 4; s->dnpc = (src1 + imm) & ~1;
                        IFDEF(CONFIG_FTRACE,
                              if(rd == 1){
                                  display_call_func(s->pc, s->dnpc);
                              } else if (rd == 0 && src1 == R(1)) {
                                  display_ret_func(s->pc);
                              });
                        );
        INSTPAT("??????? ????? ????? 000 ????? 11001 11", ret, I,
                R(rd) = s->pc + 4; s->dnpc = (src1 + imm) & ~1;
                        IFDEF(CONFIG_FTRACE,
                              if(rd == 1){
                                  display_call_func(s->pc, s->dnpc);
                              } else if (rd == 0 && src1 == R(1)) {
                                  display_ret_func(s->pc);
                              });
                        ); // jalr(ret)

        /* S */
        INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, Mw(src1 + imm, 1, src2));
        INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh, S, Mw(src1 + imm, 2, src2));
        INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, Mw(src1 + imm, 4, src2));
        INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw  , I, R(rd) = CSR(imm); CSR(imm) = src1);
        INSTPAT("??????? ????? ????? 010 ????? 11100 11", csrrs  , I, R(rd) = CSR(imm); CSR(imm) |= src1);


        /* B */
        INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt, B,
                s->dnpc += (int) src1 < (int) src2 ? imm - 4 : 0);
        INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu, B,
                s->dnpc += (uint32_t) src1 < (uint32_t) src2 ? imm - 4 : 0);
        INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge, B,
                s->dnpc += (int) src1 >= (int) src2 ? imm - 4 : 0);
        INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu, B,
                s->dnpc += src1 >= src2 ? imm - 4 : 0;);
        INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq, B,
                s->dnpc += src1 == src2 ? imm - 4 : 0;);
        INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne, B,
                s->dnpc += src1 != src2 ? imm - 4 : 0;);

        /* U */
        INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, R(rd) = s->pc + imm);
        INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, R(rd) = imm);

        /* J */
        // JAL stores the address of the instruction following the jump (pc+4) into register rd.
        INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, R(rd) = s->pc + 4; s->dnpc += imm - 4;
                IFDEF(CONFIG_FTRACE, if (rd == 1) {
                    display_call_func(s->pc, s->dnpc);
                });
        );

        /* N */
        INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
        INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall, N, etrace_info(s); ECALL(s->dnpc));
        INSTPAT("0011000 00010 00000 000 00000 11100 11", met, N, s->dnpc = cpu.csr.mepc);
        INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));


    INSTPAT_END();

    R(0) = 0; // reset $zero to 0

    return 0;
}

int isa_exec_once(Decode *s) {
    s->isa.inst.val = inst_fetch(&s->snpc, 4);
    return decode_exec(s);
}
