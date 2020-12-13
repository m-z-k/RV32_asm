#ifndef RV32_ASM_C_HPP_INCLUDED
#define RV32_ASM_C_HPP_INCLUDED

#include "RV32_asm_base.hpp"

namespace RV32_asm {

////////////////////////////////////////////////////////////////////////////////
// 圧縮命令セットの定義

template <typename T = Generator>
class CodeGenerator32C : public virtual Base, public T {
 public:
  CodeGenerator32C() : T() {}

 protected:
  // 浮動小数点数命令の圧縮命令を有効化
  enum { float_mode = T::float_mode | 8 };

  // 圧縮命令生成ラムダ式の追加
  // f浮動小数点数命令の実装で使うので protected にしておく
  virtual void C(const int op, const char *msg = "") {
    env << [=](Env &e) { e.dh(op, msg); };
  }

 private:
  void CJ(const Reg &rd, const Label &label) {
    env << [=](Env &e) {
      address_offset_t imm = label.getOffset(e);
      if (-2048 <= imm && imm <= 2046 && (imm & 1) == 0 &&
          (rd == x0 || rd == x1)) {
        // 短縮命令の対象だった
        int im2 =                   //
            ((imm & 0x800) >> 1) |  //
            ((imm & 0x400) >> 4) |  //
            ((imm & 0x300) >> 1) |  //
            ((imm & 0x080) >> 3) |  //
            ((imm & 0x040) >> 1) |  //
            ((imm & 0x020) >> 5) |  //
            ((imm & 0x010) << 5) |  //
            (imm & 0x00e);
        unsigned int bits = 0;
        const char *msg = NULL;
        if (rd == x0) {
          bits = 0b101;
          msg = "C.J";
        } else if (rd == x1) {
          bits = 0b001;
          msg = "C.JAL";
        }
        unsigned int op = (bits << 13) | (im2 << 2) | 0b01;
        e.dh(op, msg);
      } else {
        // 通常命令の処理を呼ぶ
        e.dw(T::U_(0b1101111, rd, T::u2j(imm)), "JAL");
      }
    };
  }

  // 0比較の分岐系の圧縮命令の生成用
  // B compressed branch `condition` zero
  void Bcbcz(int cop, int opcode, int funct3, const Reg &rs1,
             const Label &label, const char *cmsg, const char *msg) {
    assert(rs1.isCReg());
    env << [=](Env &e) {
      address_offset_t off = label.getOffset(e);
      if (-512 <= off && off <= 511 && (off & 1) == 0) {
        off &= 0x1fe;
        unsigned int op = (cop << 13) | ((off & 0x100) << 4) |
                          ((off & 0x018) << 7) | (rs1.getCIdx() << 7) |
                          ((off & 0x0c0) >> 1) | ((off & 0x006) << 2) |
                          ((off & 0x020) >> 3) | 0b01;
        e.dh(op, cmsg);
      } else {
        e.dw(T::S_(opcode, funct3, rs1, zero, off), msg);
      }
    };
  }

  //////////////////////////////////////////////////////////////////
  // 命令の実装関数
 public:
  // c.addi4spn / c.addiw / c.li / c.addi16sp
  virtual void addi(const Reg &rd, const Reg &rs1, int32_t imm) override {
    if (rd.isCReg() && rs1 == x2 && (imm & 0x3fc) == imm) {
      int nzuimm = ((imm & 0x03c0) >> 4) | ((imm & 0x030) << 2) |
                   ((imm & 0x008) >> 3) | ((imm & 0x004) >> 1);
      unsigned int op = (nzuimm << 5) | (rd.getCIdx() << 2) | 0b00;
      C(op, "C.ADDI4SPN");
    } else if (rd == sp && rs1 == sp &&
               (-512 <= imm && imm <= 512 - 16 && (imm % 16) == 0 &&
                imm != 0)) {
      // c.addi16sp で表せる動作の一部は c.addiw でも表せるが
      // c.addi16sp を優先して使うようにしたいので先に判定する
      imm &= 0x3f8;
      unsigned int op = (0b011 << 13) | ((imm & 0x200) << 3) |
                        (sp.getIdx() << 7) | ((imm & 0x010) << 2) |
                        ((imm & 0x040) >> 1) | ((imm & 0x180) >> 4) |
                        ((imm & 0x020) >> 3) | 0b01;
      C(op, "C.ADDI16SP");
    } else if (rd == rs1 && (-32 <= imm && imm <= 31 && imm != 0)) {
      unsigned int op = (0b000 << 13) | ((imm & 0x0020) << 7) |
                        (rs1.getIdx() << 7) | ((imm & 0x001f) << 2) | 0b01;
      C(op, "C.ADDIW");
    } else if (rd != zero && rs1 == zero && (-32 <= imm && imm <= 31)) {
      imm &= 0x3f;
      unsigned int op = (0b010 << 13) | ((imm & 0x20) << 7) |
                        (rd.getIdx() << 7) | ((imm & 0x1f) << 2) | 0b01;
      C(op, "C.LI");
    } else {
      T::addi(rd, rs1, imm);
    }
  }

  // c.fld
  // →浮動小数点数命令なのでそちらで定義

  // c.lw + c.lwsp (LW)
  virtual void lw(const Reg &rd, const OffsetReg32 &or1) override {
    const address_offset_t off = or1.getOffset();
    if (rd.isCReg() && or1.getReg().isCReg() && (off & 0x7c) == off) {
      unsigned int op = (0b010 << 13) | (or1.getReg().getCIdx() << 7) |
                        (rd.getCIdx() << 2) | 0b00 |  //
                        ((off & 0x0040) >> 1) | ((off & 0x0038) << 7) |
                        ((off & 0x0004) << 4);
      C(op, "C.LW");
    } else if (rd != zero && or1.getReg() == sp && ((off & 0xfc) == off)) {
      unsigned int op = (0b010 << 13) | ((off & 0x20) << 7) |
                        (rd.getIdx() << 7) | ((off & 0x1c) << 2) |
                        ((off & 0xc0) >> 4) | 0b10;
      C(op, "C.LWSP");
    } else {
      T::lw(rd, or1);
    }
  }

  // c.flw
  // →浮動小数点数命令なのでそちらで定義

  // c.ld
  // →RV64の命令なので実装しない

  // c.fsd
  // →浮動小数点数命令なのでそちらで定義

  // c.sw + c.swsp (SW)
  virtual void sw(const Reg &rs2, const OffsetReg32 &or1) override {
    const address_offset_t off = or1.getOffset();
    if (rs2.isCReg() && or1.getReg().isCReg() && (off & 0x7c) == off) {
      unsigned int op = (0b110 << 13) | (or1.getReg().getCIdx() << 7) |
                        (rs2.getCIdx() << 2) | 0b00 |  //
                        ((off & 0x0040) >> 1) | ((off & 0x0038) << 7) |
                        ((off & 0x0004) << 4);
      C(op, "C.SW");
    } else if (or1.getReg() == sp && ((off & 0xfc) == off)) {
      unsigned int op = (0b110 << 13) | ((off & 0x3c) << 7) |
                        ((off & 0xc0) << 1) | (rs2.getIdx() << 2) | 0b10;
      C(op, "C.SWSP");
    } else {
      T::sw(rs2, or1);
    }
  }

  // c.sd
  // →RV64の命令なので実装しない

  // c.nop(NOP)
  virtual void nop() override { C(0x0001, "C.NOP"); }

  // c.jal + c.j (JAL)
  virtual void jal(const Reg &rd, const Label &label) {
    if (rd == x1 || rd == x0) {
      // オフセットが確定しないと短縮命令に出来るか確定しない
      CJ(rd, label);
    } else {
      T::jal(rd, label);
    }
  }

  // 多重定義した関数の中で1つでも仮想関数があるとうまく動かないので
  // 全て仮想関数にする。
  virtual void jal(const Label &label) { jal(x1, label); }
  virtual void jal(std::string &label) { T::jal(label); }
  virtual void jal(const char *label) { T::jal(label); }

  // c.addiw
  // →addiの圧縮タイプの1つなので↑の方に定義を追加

  // c.li
  // オペコード生成は他のaddi命令と同じ場所で定義
  virtual void li(const Reg &rd, int32_t imm) {
    // 通常のli疑似命令の処理では、c.liで表せるケースのうち
    // immが負数のケースをうまく扱えないのでオーバーライドして対処する
    if (rd != zero && (-32 <= imm && imm <= 31)) {
      addi(rd, zero, imm);
    } else {
      T::li(rd, imm);
    }
  }

  // c.lui
  virtual void lui(const Reg &rd, uint32_t imm) {
    if ((rd != zero && rd != sp) &&
        (imm <= 31 || (0xfffe0 <= imm && imm <= 0xfffff)) && imm != 0) {
      imm &= 0x0003f;
      unsigned int op = (0b011 << 13) | ((imm & 0x00020) << 7) |
                        (rd.getIdx() << 7) |  //
                        ((imm & 0x0001f) << 2) | 0b01;
      C(op, "C.LUI");
    } else {
      T::lui(rd, imm);
    }
  }

  // c.srli
  virtual void srli(const Reg &rd, const Reg &rs1, int32_t imm) {
    assert((imm & 0x1f) == imm);
    if (rd == rs1 && rd.isCReg()) {
      unsigned int op = (0b100 << 13) | ((imm & 0x00020) << 7) | (0b00 << 10) |
                        (rd.getCIdx() << 7) |  //
                        ((imm & 0x0001f) << 2) | 0b01;
      C(op, "C.SRLI");
    } else {
      T::srli(rd, rs1, imm);
    }
  }

  // c.srai
  virtual void srai(const Reg &rd, const Reg &rs1, int32_t imm) {
    assert((imm & 0x1f) == imm);
    if (rd == rs1 && rd.isCReg()) {
      unsigned int op = (0b100 << 13) | ((imm & 0x00020) << 7) | (0b01 << 10) |
                        (rd.getCIdx() << 7) |  //
                        ((imm & 0x0001f) << 2) | 0b01;
      C(op, "C.SRAI");
    } else {
      T::srai(rd, rs1, imm);
    }
  }

  // c.andi
  virtual void andi(const Reg &rd, const Reg &rs1, int32_t imm) {
    if (rd == rs1 && rd.isCReg() && (-32 <= imm && imm <= 31)) {
      unsigned int op = (0b100 << 13) | ((imm & 0x00020) << 7) | (0b10 << 10) |
                        (rd.getCIdx() << 7) |  //
                        ((imm & 0x0001f) << 2) | 0b01;
      C(op, "C.ANDI");
    } else {
      T::andi(rd, rs1, imm);
    }
  }

  // c.sub
  virtual void sub(const Reg &rd, const Reg &rs1, const Reg &rs2) {
    if (rd.isCReg() && rd == rs1 && rs2.isCReg()) {
      unsigned int op = (0b100011 << 10) | (rd.getCIdx() << 7) | (0b00 << 5) |
                        (rs2.getCIdx() << 2) | 0b01;
      C(op, "C.SUB");
    } else {
      T::sub(rd, rs1, rs2);
    }
  }

  // c.xor
  virtual void x\
or(const Reg &rd, const Reg &rs1, const Reg &rs2) {
    if (rd.isCReg() && rd == rs1 && rs2.isCReg()) {
      unsigned int op = (0b100011 << 10) | (rd.getCIdx() << 7) | (0b01 << 5) |
                        (rs2.getCIdx() << 2) | 0b01;
      C(op, "C.XOR");
    } else {
      T::x\
or(rd, rs1, rs2);
    }
  }

  // c.or
  virtual void or (const Reg &rd, const Reg &rs1, const Reg &rs2) {
    if (rd.isCReg() && rd == rs1 && rs2.isCReg()) {
      unsigned int op = (0b100011 << 10) | (rd.getCIdx() << 7) | (0b10 << 5) |
                        (rs2.getCIdx() << 2) | 0b01;
      C(op, "C.OR");
    } else {
      T:: or (rd, rs1, rs2);
    }
  }

  // c.and
  virtual void and (const Reg &rd, const Reg &rs1, const Reg &rs2) {
    if (rd.isCReg() && rd == rs1 && rs2.isCReg()) {
      unsigned int op = (0b100011 << 10) | (rd.getCIdx() << 7) | (0b11 << 5) |
                        (rs2.getCIdx() << 2) | 0b01;
      C(op, "C.AND");
    } else {
      T::and(rd, rs1, rs2);
    }
  }

  // c.subw
  // RV64の命令なのでここでは実装しない

  // c.addw
  // RV64の命令なのでここでは実装しない

  // c.beqz
  virtual void beq(const Reg &rs1, const Reg &rs2, const Label &label) {
    if (rs1.isCReg() && rs2 == zero) {
      Bcbcz(0b110, 0b1100011, 0b000, rs1, label, "C.BEQZ", "BEQZ");
    } else {
      T::beq(rs1, rs2, label);
    }
  }

  virtual void bne(const Reg &rs1, const Reg &rs2, const Label &label) {
    if (rs1.isCReg() && rs2 == zero) {
      Bcbcz(0b111, 0b1100011, 0b001, rs1, label, "C.BNEZ", "BNEZ");
    } else {
      T::bne(rs1, rs2, label);
    }
  }

  // c.slli
  virtual void slli(const Reg &rd, const Reg &rs1, int32_t imm) {
    assert((imm & 0x1f) == imm);
    if (rd == rs1) {
      unsigned int op = (0b000 << 13) | ((imm & 0x00020) << 7) |
                        (rd.getIdx() << 7) | ((imm & 0x0001f) << 2) | 0b10;
      C(op, "C.SLLI");
    } else {
      T::slli(rd, rs1, imm);
    }
  }

  // c.fldsp
  // c.flwsp
  // →浮動小数点数命令なのでそちらで定義

  // c.ldsp
  // RV64の命令なのでここでは実装しない

  // c.jr + c.jalr
  virtual void jalr(const Reg &rd, const OffsetReg32 &or1) {
    if (rd == x0 && or1.getOffset() == 0) {
      unsigned int op = (0b100 << 13) | (0b0 << 12) | (or1.getIdx() << 7) |
                        (0b00000 << 2) | 0b10;
      C(op, (or1.getIdx() == 1) ? "C.RET" : "C.JR");
    } else if (rd == x1 && or1.getOffset() == 0) {
      unsigned int op = (0b100 << 13) | (0b1 << 12) | (or1.getIdx() << 7) |
                        (0b00000 << 2) | 0b10;
      C(op, "C.JALR");
    } else {
      T::jalr(rd, or1);
    }
  }

  virtual void jr(const Reg &rs) { jalr(x0, rs[0]); }
  virtual void jalr(const Reg &rs) { jalr(x1, rs[0]); }

  // c.mv + c.add
  virtual void add(const Reg &rd, const Reg &rs1, const Reg &rs2) {
    if (rs1 == zero) {
      unsigned int op = (0b100 << 13) | (0b0 << 12) | (rd.getIdx() << 7) |
                        (rs2.getIdx() << 2) | 0b10;
      C(op, "C.MV");
    } else if (rd == rs1) {
      unsigned int op = (0b100 << 13) | (0b1 << 12) | (rs1.getIdx() << 7) |
                        (rs2.getIdx() << 2) | 0b10;
      C(op, "C.ADD");
    } else {
      T::add(rd, rs1, rs2);
    }
  }

  // c.ebreak
  // →割り込み関係なので実装しない

  // c.fsdsp
  // c.fswsp
  // →浮動小数点数命令なのでそちらで定義

  // c.sdsp
  // RV64の命令なのでここでは実装しない

};  // namespace RV32_asm

REGIST_IS('C', RV32_asm::CodeGenerator32C);
};  // namespace RV32_asm

#endif
