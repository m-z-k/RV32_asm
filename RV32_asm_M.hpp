#ifndef RV32_ASM_M_HPP_INCLUDED
#define RV32_ASM_M_HPP_INCLUDED

#include "RV32_asm_base.hpp"

namespace RV32_asm {

////////////////////////////////////////////////////////////////////////////////
// 圧縮命令セットの定義
template <typename T = Generator>
class CodeGenerator32M : public virtual Base, public T {
 public:
  CodeGenerator32M() : T() {}
  // mul
  void mul(const Reg& rd, const Reg& rs1, const Reg& rs2) {
    T::R(0b0110011, 0b0000001, 0b000, rd, rs1, rs2, "MUL");
  }
  // mulh
  void mulh(const Reg& rd, const Reg& rs1, const Reg& rs2) {
    T::R(0b0110011, 0b0000001, 0b001, rd, rs1, rs2, "MULH");
  }
  // mulhsu
  void mulhsu(const Reg& rd, const Reg& rs1, const Reg& rs2) {
    T::R(0b0110011, 0b0000001, 0b010, rd, rs1, rs2, "MULHSU");
  }
  // mulhu
  void mulhu(const Reg& rd, const Reg& rs1, const Reg& rs2) {
    T::R(0b0110011, 0b0000001, 0b011, rd, rs1, rs2, "MULHU");
  }
  // div
  void div(const Reg& rd, const Reg& rs1, const Reg& rs2) {
    T::R(0b0110011, 0b0000001, 0b100, rd, rs1, rs2, "DIV");
  }
  // divu
  void divu(const Reg& rd, const Reg& rs1, const Reg& rs2) {
    T::R(0b0110011, 0b0000001, 0b101, rd, rs1, rs2, "DIVU");
  }
  // rem
  void rem(const Reg& rd, const Reg& rs1, const Reg& rs2) {
    T::R(0b0110011, 0b0000001, 0b110, rd, rs1, rs2, "REM");
  }
  // remu
  void remu(const Reg& rd, const Reg& rs1, const Reg& rs2) {
    T::R(0b0110011, 0b0000001, 0b111, rd, rs1, rs2, "REMU");
  }
 private:
};

REGIST_IS('M', RV32_asm::CodeGenerator32M);

};  // namespace RV32_asm

#endif
