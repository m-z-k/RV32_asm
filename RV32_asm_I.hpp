#ifndef RV32_ASM_I_HPP_INCLUDED
#define RV32_ASM_I_HPP_INCLUDED

#include "RV32_asm_base.hpp"

namespace RV32_asm {

////////////////////////////////////////////////////////////////////////////////
// 整数命令の定義

template <typename T = Generator>
class CodeGenerator32I : public virtual Base, public T {
 public:
  CodeGenerator32I() : T() {}

  //////////////////////////////////////////////////////////////////
  // 各命令タイプのオペコード組み立て用補助関数
  
 protected:
  constexpr static uint32_t R_(int opcode, int funct7, int funct3,
                               const Reg &rd, const Reg &rs1, const Reg &rs2) {
    uint32_t op = (uint32_t(funct7 & 0x7f) << 25) | (rs2.getIdx() << 20) |
                  (rs1.getIdx() << 15) | ((funct3 & 0x7) << 12) |
                  (rd.getIdx() << 7) | (opcode & 0x7f);
    return op;
  }
  constexpr static uint32_t I_(int opcode, int funct3, const Reg &rd,
                               const Reg &rs1, address_offset_t imm) {
    assert(-4096 <= imm && imm <= 4095);
    uint32_t op = (uint32_t(imm & 0xfff) << 20) | (rs1.getIdx() << 15) |
                  ((funct3 & 0x7) << 12) | (rd.getIdx() << 7) | (opcode & 0x7f);
    return op;
  }

  constexpr static uint32_t S_(int opcode, int funct3, const Reg &rs1,
                               const Reg &rs2, address_offset_t imm) {
    assert(-4096 <= imm && imm <= 4095);
    uint32_t op = (uint32_t(imm & 0xfe0) << 20) | (rs2.getIdx() << 20) |
                  (rs1.getIdx() << 15) | ((funct3 & 0x7) << 12) |
                  ((imm & 0x1f) << 7) | (opcode & 0x7f);
    return op;
  }

  constexpr static uint32_t B_(int opcode, int funct3, const Reg &rs1,
                               const Reg &rs2, address_offset_t imm) {
    assert((imm & 1) == 0 && -8192 <= imm && imm <= 8190);
    const uint32_t tmp = imm & 0x00001fff;
    uint32_t op = ((tmp >> 12) & 1) << 31 | ((tmp >> 5) & 0x3f) << 25 |
                  (rs2.getIdx() << 20) | (rs1.getIdx() << 15) |
                  ((funct3 & 0x7) << 12) | ((tmp >> 1) & 0xf) << 8 |
                  ((tmp >> 11) & 1) << 7 | (opcode & 0x7f);
    return op;
  }

  constexpr static uint32_t U_(int opcode, const Reg &rd,
                               address_offset_t imm) {
    assert((imm & 0xfff) == 0);
    uint32_t op =
        uint32_t(imm & 0xfffff000) | (rd.getIdx() << 7) | (opcode & 0x7f);
    return op;
  }

  //////////////////////////////////////////////////////////////////
  // 各命令タイプのオペコード組み立て・登録関数

  static address_offset_t u2j(address_offset_t imm) {
    uint32_t tmp = (uint32_t)imm;
    return (((tmp >> 20) & 0x1) << 19    // 1
            | ((tmp >> 1) & 0x3ff) << 9  // 10
            | ((tmp >> 11) & 0x1) << 8   // 1
            | ((tmp >> 12) & 0xff)       // 8
            )
           << 12;
    return address_offset_t(tmp);
  }

  void R(int opcode, int funct7, int funct3, const Reg &rd, const Reg &rs1,
         const Reg &rs2, const char *msg = "") {
    env << [=](Env &e) { e.dw(R_(opcode, funct7, funct3, rd, rs1, rs2), msg); };
  }
  void I(int opcode, int funct3, const Reg &rd, const Reg &rs1,
         address_offset_t imm, const char *msg = "") {
    env << [=](Env &e) { e.dw(I_(opcode, funct3, rd, rs1, imm), msg); };
  }
  void S(int opcode, int funct3, const Reg &rs1, const Reg &rs2,
         address_offset_t imm, const char *msg = "") {
    env << [=](Env &e) { e.dw(S_(opcode, funct3, rs1, rs2, imm), msg); };
  }
  void B(int opcode, int funct3, const Reg &rs1, const Reg &rs2,
         address_offset_t imm, const char *msg = "") {
    env << [=](Env &e) { e.dw(S_(opcode, funct3, rs1, rs2, imm), msg); };
  }
  void B(int opcode, int funct3, const Reg &rs1, const Reg &rs2,
         const Label &label, const char *msg = "") {
    env << [=](Env &e) {
      address_offset_t imm = label.getOffset(e);
      e.dw(S_(opcode, funct3, rs1, rs2, imm), msg);
    };
  }
  void U(int opcode, const Reg &rd, address_offset_t imm,
         const char *msg = "") {
    env << [=](Env &e) { e.dw(U_(opcode, rd, imm), msg); };
  }
  void U(int opcode, const Reg &rd, const Label &label, const char *msg = "") {
    env << [=](Env &e) { e.dw(U_(opcode, rd, label.getOffset(e)), msg); };
  }
  void J(int opcode, const Reg &rd, address_offset_t imm,
         const char *msg = "") {
    env << [=](Env &e) { e.dw(U_(opcode, rd, u2j(imm)), msg); };
  }
  void J(int opcode, const Reg &rd, const Label &label, const char *msg = "") {
    env << [=](Env &e) {
      address_offset_t imm = label.getOffset(e);
      e.dw(U_(opcode, rd, u2j(imm)), msg);
    };
  }

  // 疑似命令callの実装
  void pi_call(const Label &label, const char *msg = "") {
    // ラベルのオフセット値が確定しないと命令が生成できないので
    // 他の疑似命令と異なりこのレベルで実装している
    env << [=](Env &e) {
      address_offset_t offset = label.getOffset(e);
      address_offset_t hi = (offset & 0xfffff000) + ((offset & 0x0800) << 1);
      address_offset_t lo = offset & 0x00000fff;
      if (offset & 0x00000800) {  //符号拡張
        lo |= 0xfffff000;
      }
      assert(hi + lo == offset);
#if IN_DEBUG_MODE
      printf("off:%08x(%d)\n HI:%08x\n LO:%08x(%d)\n+++:%08x\n", offset, offset,
             hi, lo, lo, hi + lo);
#endif
      e.dw(U_(0b0010111, x1, hi), "CALL(AUIPC)");
      e.dw(I_(0b1100111, 0b000, x1, x1, lo), "CALL(JALR)");
    };
  }

  // 疑似命令tailの実装
  void pi_tail(const Label &label, const char *msg = "") {
    // ラベルのオフセット値が確定しないと命令が生成できないので
    // 他の疑似命令と異なりこのレベルで実装している
    env << [=](Env &e) {
      address_offset_t offset = label.getOffset(e);
      address_offset_t hi = (offset & 0xfffff000) + ((offset & 0x0800) << 1);
      address_offset_t lo = offset & 0x00000fff;
      if (offset & 0x00000800) {  //符号拡張
        lo |= 0xfffff000;
      }
      assert(hi + lo == offset);
#if IN_DEBUG_MODE
      printf("off:%08x(%d)\n HI:%08x\n LO:%08x(%d)\n+++:%08x\n", offset, offset,
             hi, lo, lo, hi + lo);
#endif
      e.dw(U_(0b0010111, x6, hi), "TAIL(AUIPC)");
      e.dw(I_(0b1100111, 0b000, x0, x6, lo), "TAIL(JALR)");
    };
  }

  //////////////////////////////////////////////////////////////////
  // 命令の実装関数
 public:
  // LUI
  virtual void lui(const Reg &rd, uint32_t imm) {
    assert(imm <= 1048575);
    U(0b0110111, rd, imm << 12, "LUI");
  }

  // AUIPC
  void auipc(const Reg &rd, const Label &label) {
    const char *msg = "AUIPC";
    U(0b0010111, rd, label, msg);
  }
  void auipc(const Reg &rd, address_offset_t offset) {
    const char *msg = "AUIPC";
    Label l(offset);
    U(0b0010111, rd, l, msg);
  }

  // JAL
  virtual void jal(const Reg &rd, const Label &label) {
    const char *msg = "JAL";
    if (rd.getIdx() == 0) {
      msg = "J";
    }
    J(0b1101111, rd, label, msg);
  }

  // JALR
  virtual void jalr(const Reg &rd, const OffsetReg32 &or1) {
    const char *msg = "JALR";
    if (rd.getIdx() == 0 && or1.getIdx() == 1 && or1.getOffset() == 0) {
      msg = "RET";
    } else if (rd.getIdx() == 0 && or1.getOffset() == 0) {
      msg = "JR";
    } else if (rd.getIdx() == 1 && or1.getOffset() == 0) {
      msg = "JALR";
    }
    I(0b1100111, 0b000, rd, or1.getReg(), or1.getOffset(), msg);
  }

  // BEQ
  virtual void beq(const Reg &rs1, const Reg &rs2, const Label &label) {
    const char *msg = "BEQ";
    if (rs2 == zero) {
      msg = "BEQZ";
    }
    B(0b1100011, 0b000, rs1, rs2, label, msg);
  }

  // BNE
  virtual void bne(const Reg &rs1, const Reg &rs2, const Label &label) {
    const char *msg = "BNE";
    if (rs2 == zero) {
      msg = "BNEZ";
    }
    B(0b1100011, 0b001, rs1, rs2, label, msg);
  }

  // BLT
  void blt(const Reg &rs1, const Reg &rs2, const Label &label) {
    const char *msg = "BLT";
    if (rs1 == zero) {
      msg = "BLTZ";
    } else if (rs2 == zero) {
      msg = "BGTZ";
    }
    B(0b1100011, 0b100, rs1, rs2, label, msg);
  }

  // BGE
  void bge(const Reg &rs1, const Reg &rs2, const Label &label) {
    const char *msg = "BGE";
    if (rs1 == zero) {
      msg = "BLEZ";
    } else if (rs2 == zero) {
      msg = "BGEZ";
    }
    B(0b1100011, 0b101, rs1, rs2, label, msg);
  }

  // BLTU
  void bltu(const Reg &rs1, const Reg &rs2, const Label &label) {
    B(0b1100011, 0b110, rs1, rs2, label, "BLTU");
  }

  // BGEU
  void bgeu(const Reg &rs1, const Reg &rs2, const Label &label) {
    B(0b1100011, 0b111, rs1, rs2, label, "BGEU");
  }

  // LB
  void lb(const Reg &rd, const OffsetReg32 &or1) {
    I(0b0000011, 0b000, rd, or1.getReg(), or1.getOffset(), "LB");
  }

  // LH
  void lh(const Reg &rd, const OffsetReg32 &or1) {
    I(0b0000011, 0b001, rd, or1.getReg(), or1.getOffset(), "LH");
  }

  // LW
  virtual void lw(const Reg &rd, const OffsetReg32 &or1) {
    I(0b0000011, 0b010, rd, or1.getReg(), or1.getOffset(), "LW");
  }

  // LBU
  void lbu(const Reg &rd, const OffsetReg32 &or1) {
    I(0b0000011, 0b100, rd, or1.getReg(), or1.getOffset(), "LBU");
  }

  // LHU
  void lhu(const Reg &rd, const OffsetReg32 &or1) {
    I(0b0000011, 0b100, rd, or1.getReg(), or1.getOffset(), "LHU");
  }

  // SB
  void sb(const Reg &rs2, const OffsetReg32 &or1) {
    S(0b0100011, 0b000, or1.getReg(), rs2, or1.getOffset(), "SB");
  }

  // SH
  void sh(const Reg &rs2, const OffsetReg32 &or1) {
    S(0b0100011, 0b001, or1.getReg(), rs2, or1.getOffset(), "SH");
  }

  // SW
  virtual void sw(const Reg &rs2, const OffsetReg32 &or1) {
    S(0b0100011, 0b010, or1.getReg(), rs2, or1.getOffset(), "SW");
  }

  // ADDI
  virtual void addi(const Reg &rd, const Reg &rs1, int32_t imm) {
    const char *msg = "ADDI";
    if (imm == 0) {
      if (rd == zero && rs1 == zero) {
        msg = "NOP";
      } else {
        msg = "MV";
      }
    }
    I(0b0010011, 0b000, rd, rs1, imm, msg);
  }

  // SLTI
  void slti(const Reg &rd, const Reg &rs1, int32_t imm) {
    I(0b0010011, 0b010, rd, rs1, imm, "SLTI");
  }

  // SLTIU
  void sltiu(const Reg &rd, const Reg &rs1, int32_t imm) {
    const char *msg = "SLTIU";
    if (imm == 1) {
      msg = "SEQZ";
    }
    I(0b0010011, 0b011, rd, rs1, imm, msg);
  }

  // XORI
  void xori(const Reg &rd, const Reg &rs1, int32_t imm) {
    const char *msg = "XORI";
    if (imm == -1) {
      msg = "NOT";
    }
    I(0b0010011, 0b100, rd, rs1, imm, msg);
  }

  // ORI
  void ori(const Reg &rd, const Reg &rs1, int32_t imm) {
    I(0b0010011, 0b110, rd, rs1, imm, "ORI");
  }

  // ANDI
  virtual void andi(const Reg &rd, const Reg &rs1, int32_t imm) {
    I(0b0010011, 0b111, rd, rs1, imm, "ANDI");
  }

  // SLLI
  virtual void slli(const Reg &rd, const Reg &rs1, int32_t imm) {
    assert((imm & 0x1f) == imm);
    I(0b0010011, 0b001, rd, rs1, imm, "SLLI");
  }

  // SRLI
  virtual void srli(const Reg &rd, const Reg &rs1, int32_t imm) {
    assert((imm & 0x1f) == imm);
    I(0b0010011, 0b101, rd, rs1, imm, "SRLI");
  }

  // SRAI
  virtual void srai(const Reg &rd, const Reg &rs1, int32_t imm) {
    assert((imm & 0x1f) == imm);
    I(0b0010011, 0b101, rd, rs1, 0b010000000000 | imm, "SRAI");
  }

  // ADD
  virtual void add(const Reg &rd, const Reg &rs1, const Reg &rs2) {
    R(0b0110011, 0b0000000, 0b000, rd, rs1, rs2, "ADD");
  }

  // SUB
  virtual void sub(const Reg &rd, const Reg &rs1, const Reg &rs2) {
    const char *msg = "SUB";
    if (rs1 == zero) {
      msg = "NEG";
    }
    R(0b0110011, 0b0100000, 0b000, rd, rs1, rs2, msg);
  }

  // SLL
  void sll(const Reg &rd, const Reg &rs1, const Reg &rs2) {
    R(0b0110011, 0b0000000, 0b001, rd, rs1, rs2, "SLL");
  }

  // SLT
  void slt(const Reg &rd, const Reg &rs1, const Reg &rs2) {
    const char *msg = "SLT";
    if (rs1 == zero) {
      msg = "SGTZ";
    } else if (rs2 == zero) {
      msg = "SLTZ";
    }
    R(0b0110011, 0b0000000, 0b010, rd, rs1, rs2, msg);
  }

  // SLTU
  void sltu(const Reg &rd, const Reg &rs1, const Reg &rs2) {
    const char *msg = "SLTU";
    if (rs1 == zero) {
      msg = "SNEZ";
    }
    R(0b0110011, 0b0000000, 0b011, rd, rs1, rs2, msg);
  }

  // XOR
  // ソースフォーマッタが"xor"を関数名として扱ってくれないケースがあり、
  // インデントが崩れるので関数名の途中に \+改行 を挟むことで回避している
  virtual void x\
or(const Reg &rd, const Reg &rs1, const Reg &rs2) {
    R(0b0110011, 0b0000000, 0b100, rd, rs1, rs2, "XOR");
  }

  // SRL
  void srl(const Reg &rd, const Reg &rs1, const Reg &rs2) {
    R(0b0110011, 0b0000000, 0b101, rd, rs1, rs2, "SRL");
  }

  // SRA
  void sra(const Reg &rd, const Reg &rs1, const Reg &rs2) {
    R(0b0110011, 0b0100000, 0b101, rd, rs1, rs2, "SRA");
  }

  // OR
  virtual void or (const Reg &rd, const Reg &rs1, const Reg &rs2) {
    R(0b0110011, 0b0000000, 0b110, rd, rs1, rs2, "OR");
  }

  // AND
  virtual void and (const Reg &rd, const Reg &rs1, const Reg &rs2) {
    R(0b0110011, 0b0000000, 0b111, rd, rs1, rs2, "AND");
  }

  // FENCE
  // 命令の仕様自体をほとんど把握できていないので未実装

  // ECALL
  void ecall() { I(0b1110011, 0b000, zero, zero, 0, "ECALL"); }

  // EBREAK
  void ebreak() { I(0b1110011, 0b000, zero, zero, 1, "EBREAK"); }

  //////////////////////////////////////////////////////////////////
  // 疑似命令の実装関数

  // nop
  virtual void nop() { addi(zero, zero, 0); }

  // li
  virtual void li(const Reg &rd, uint32_t imm) {
    // addi命令は即値を12ビットの「符号付き」整数として扱うので注意
    address_offset_t hi = (imm & 0xfffff000) + ((imm & 0x0800) << 1);
    address_offset_t lo = imm & 0x00000fff;

    // immの上位20ビットが非0ならluiで上位20ビットをセット
    if (hi != 0) {
      lui(rd, hi >> 12);
      // immの下位12ビットが非0ならaddiで下位12ビットをセット
      if (lo != 0) {
        addi(rd, rd, lo);
      }
    } else {
      // 上位20ビットは0だったので下位12ビットをaddiでセット
      addi(rd, zero, lo);
    }
  }

  // mv
  void mv(const Reg &rd, const Reg &rs1) { addi(rd, rs1, 0); }

  // not
  void not(const Reg &rd, const Reg &rs1) { xori(rd, rs1, -1); }

  // neg
  void neg(const Reg &rd, const Reg &rs1) { sub(rd, zero, rs1); }

  // seqz
  void seqz(const Reg &rd, const Reg &rs1) { sltiu(rd, rs1, 1); }

  // snez
  void snez(const Reg &rd, const Reg &rs1) { sltu(rd, zero, rs1); }

  // sltz
  void sltz(const Reg &rd, const Reg &rs1) { slt(rd, rs1, zero); }

  // sltz
  void sgtz(const Reg &rd, const Reg &rs1) { slt(rd, zero, rs1); }

  // beqz
  void beqz(const Reg &rs, const Label &label) { beq(rs, zero, label); }

  // bnez
  void bnez(const Reg &rs, const Label &label) { bne(rs, zero, label); }

  // blez
  void blez(const Reg &rs, const Label &label) { bge(zero, rs, label); }

  // bgez
  void bgez(const Reg &rs, const Label &label) { bge(rs, zero, label); }

  // bltz
  void bltz(const Reg &rs, const Label &label) { blt(zero, rs, label); }

  // bgtz
  void bgtz(const Reg &rs, const Label &label) { blt(rs, zero, label); }

  // bgt
  void bgt(const Reg &rs1, const Reg &rs2, const Label &label) {
    blt(rs2, rs1, label);
  }

  // ble
  void ble(const Reg &rs1, const Reg &rs2, const Label &label) {
    bge(rs2, rs1, label);
  }

  // bgtu
  void bgtu(const Reg &rs1, const Reg &rs2, const Label &label) {
    bltu(rs2, rs1, label);
  }

  // bleu
  void bleu(const Reg &rs1, const Reg &rs2, const Label &label) {
    bgeu(rs2, rs1, label);
  }

  // j offset
  void j(const Label &label) { jal(x0, label); }
  void j(std::string &label) {
    Label l(label);
    j(l);
  }
  void j(const char *label) {
    Label l(label);
    j(l);
  }

  // jal offset
  virtual void jal(const Label &label) { jal(x1, label); }
  virtual void jal(std::string &label) {
    Label l(label);
    jal(l);
  }
  virtual void jal(const char *label) {
    Label l(label);
    jal(l);
  }

  // jr rs
  virtual void jr(const Reg &rs) { jalr(x0, rs[0]); }

  // jalr rs
  virtual void jalr(const Reg &rs) { jalr(x1, rs[0]); }

  // ret
  void ret() { jalr(x0, x1[0]); }

  // call
  void call(const Label &label) { pi_call(label); }
  void call(const char *label) {
    Label l(label);
    call(l);
  }

  // tail
  void tail(const Label &label) { pi_tail(label); }
  void tail(const char *label) {
    Label l(label);
    tail(l);
  }
};

REGIST_IS('I', RV32_asm::CodeGenerator32I);

};  // namespace RV32_asm

#endif
