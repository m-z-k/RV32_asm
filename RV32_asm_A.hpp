#ifndef RV32_ASM_A_HPP_INCLUDED
#define RV32_ASM_A_HPP_INCLUDED

#include "RV32_asm_base.hpp"

namespace RV32_asm {

////////////////////////////////////////////////////////////////////////////////
// アトミック命令セットの定義

template <typename T = Generator>
class CodeGenerator32A : public virtual Base, public T {
  typedef CodeGenerator32A<T> self_t;

 private:
  void A(unsigned int funct5, bool aq, bool rl, const Reg &rs2, const Reg &rs1,
         const Reg &rd, const char *s) {
    uint32_t op = (funct5 << 27) | ((aq ? 1 : 0) << 26) | ((rl ? 1 : 0) << 25) |
                  (rs2.getIdx() << 20) | (rs1.getIdx() << 15) | (0b010 << 12) |
                  (rd.getIdx() << 7) | 0b0101111;
    env << [=](Env &e) { e.dw(op, s); };
  }

  //////////////////////////////////////////////////////////////////////////////
  // xx.y 型の名前を持つ命令の実装用のクラスの定義

  class LR {
    DOT_CLASS_SETUP(LR)
    void w(const Reg &rd, const Reg &rs1) {
      parent.A(0b00010, false, false, parent.zero, rs1, rd, "LR.W");
    }
  };

  class SC {
    DOT_CLASS_SETUP(SC)
    void w(const Reg &rd, const Reg &rs2, const Reg &rs1) {
      parent.A(0b00011, false, false, rs2, rs1, rd, "SC.W");
    }
  };

  class AMOSWAP {
    DOT_CLASS_SETUP(AMOSWAP)
    void w(const Reg &rd, const Reg &rs2, const Reg &rs1) {
      parent.A(0b00001, false, false, rs2, rs1, rd, "AMOSWAP.W");
    }
  };

  class AMOADD {
    DOT_CLASS_SETUP(AMOADD)
    void w(const Reg &rd, const Reg &rs2, const Reg &rs1) {
      parent.A(0b00000, false, false, rs2, rs1, rd, "AMOADD.W");
    }
  };

  class AMOXOR {
    DOT_CLASS_SETUP(AMOXOR)
    void w(const Reg &rd, const Reg &rs2, const Reg &rs1) {
      parent.A(0b00100, false, false, rs2, rs1, rd, "AMOXOR.W");
    }
  };

  class AMOAND {
    DOT_CLASS_SETUP(AMOAND)
    void w(const Reg &rd, const Reg &rs2, const Reg &rs1) {
      parent.A(0b01100, false, false, rs2, rs1, rd, "AMOAND.W");
    }
  };

  class AMOOR {
    DOT_CLASS_SETUP(AMOOR)
    void w(const Reg &rd, const Reg &rs2, const Reg &rs1) {
      parent.A(0b01000, false, false, rs2, rs1, rd, "AMOOR.W");
    }
  };

  class AMOMIN {
    DOT_CLASS_SETUP(AMOMIN)
    void w(const Reg &rd, const Reg &rs2, const Reg &rs1) {
      parent.A(0b10000, false, false, rs2, rs1, rd, "AMOMIN.W");
    }
  };

  class AMOMAX {
    DOT_CLASS_SETUP(AMOMAX)
    void w(const Reg &rd, const Reg &rs2, const Reg &rs1) {
      parent.A(0b10100, false, false, rs2, rs1, rd, "AMOMAX.W");
    }
  };

  class AMOMINU {
    DOT_CLASS_SETUP(AMOMINU)
    void w(const Reg &rd, const Reg &rs2, const Reg &rs1) {
      parent.A(0b11000, false, false, rs2, rs1, rd, "AMOMINU.W");
    }
  };

  class AMOMAXU {
    DOT_CLASS_SETUP(AMOMAXU)
    void w(const Reg &rd, const Reg &rs2, const Reg &rs1) {
      parent.A(0b11100, false, false, rs2, rs1, rd, "AMOMAXU.W");
    }
  };

 public:
  LR lr;
  SC sc;
  AMOSWAP amoswap;
  AMOADD amoadd;
  AMOXOR amoxor;
  AMOAND amoand;
  AMOOR amoor;
  AMOMIN amomin;
  AMOMAX amomax;
  AMOMINU amominu;
  AMOMAXU amomaxu;

  CodeGenerator32A<T>()
      : T(),
        lr(*this),
        sc(*this),
        amoswap(*this),
        amoadd(*this),
        amoxor(*this),
        amoand(*this),
        amoor(*this),
        amomin(*this),
        amomax(*this),
        amominu(*this),
        amomaxu(*this) {}
};  // namespace RV32_asm

REGIST_IS('A', RV32_asm::CodeGenerator32A);

};  // namespace RV32_asm

#endif
