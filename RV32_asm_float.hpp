#ifndef RV32_ASM_FLOAT_HPP_INCLUDED
#define RV32_ASM_FLOAT_HPP_INCLUDED

#include "RV32_asm_base.hpp"

namespace RV32_asm {

////////////////////////////////////////////////////////////////////////////////
// 浮動小数点数命令セットの定義

template <typename T = Generator>
class CodeGenerator32Float : public virtual Base, public T {
  typedef CodeGenerator32Float<T> self_t;

 protected:
  enum {
    float_mode = T::float_mode,
    enable_single_precision = 1,
    enable_double_precision = 2,
    enable_quadruple_precision = 4,
    enable_compressed_instruction = 8,
  };

#define IS_FLOAT_ONLY                                                          \
  do {                                                                         \
    static_assert((float_mode & enable_single_precision) != 0,                 \
                  "The single precision floating point number instruction(F) " \
                  "is disabled.");                                             \
  } while (0)

#define IS_DOUBLE_ONLY                                                         \
  do {                                                                         \
    static_assert((float_mode & enable_double_precision) != 0,                 \
                  "The double precision floating point number instruction(D) " \
                  "is disabled.");                                             \
  } while (0)

#define IS_QUADRUPLE_ONLY                                               \
  do {                                                                  \
    static_assert(                                                      \
        (float_mode & enable_quadruple_precision) != 0,                 \
        "The quadruple precision floating point number instruction(Q) " \
        "is disabled.");                                                \
  } while (0)

#define SUPPORT_COMP ((float_mode & enable_compressed_instruction) != 0)

 public:
  /// 丸めモード
  enum RoundingMode {
    rne = 0,  ///<最近の偶数へ丸める
    rtz = 1,  ///<ゼロに向かって丸める
    rdn = 2,  ///<切り下げ(-∞方向)
    rup = 3,  ///<切り上げ(+∞方向)
    rmm = 4,  ///<最も近い絶対値が大きい方向に丸める
    invalid_rounding_mode5 = 5,  ///< 不正な値。将来使用するため予約。
    invalid_rounding_mode6 = 6,  ///< 不正な値。将来使用するため予約。
    dyn = 7,  ///<動的丸めモード(丸めモードレジスタでは使用できない値)
  };

 private:
  // メモリ->レジスタ
  void LD(address_offset_t imm, int rs1, unsigned int funct3, int rd,
          const char *s) {
    assert(-2048 <= imm && imm <= 2047);
    uint32_t op =
        (imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | 0b0000111;
    env << [=](Env &e) { e.dw(op, s); };
  }

  // メモリ<-レジスタ
  void ST(address_offset_t imm, int rs2, int rs1, unsigned int funct3,
          const char *s) {
    assert(-2048 <= imm && imm <= 2047);
    uint32_t op = ((imm & 0xfe0) << 20) | (rs2 << 20) | (rs1 << 15) |
                  (funct3 << 12) | ((imm & 0x01f) << 7) | 0b0100111;
    env << [=](Env &e) { e.dw(op, s); };
  }

  // 浮動小数点系の演算命令
  void FL(unsigned int opcode, const int pr, const int rd, const int rs1,
          const int rs2, const int rs3, RoundingMode rm, const char *s) {
    uint32_t op = (rs3 << 27) | (pr << 25) | (rs2 << 20) | (rs1 << 15) |
                  (rm & 0b111) << 12 | (rd << 7) | opcode;
    env << [=](Env &e) { e.dw(op, s); };
  }

  // rdが浮動小数点数レジスタのケース
  void F(unsigned int opcode, const FReg &rd, const FReg &rs1, const FReg &rs2,
         const FReg &rs3, RoundingMode rm, const char *s) {
    FL(opcode, 0b00, rd.getIdx(), rs1.getIdx(), rs2.getIdx(), rs3.getIdx(), rm,
       s);
  }

  void D(unsigned int opcode, const FReg &rd, const FReg &rs1, const FReg &rs2,
         const FReg &rs3, RoundingMode rm, const char *s) {
    FL(opcode, 0b01, rd.getIdx(), rs1.getIdx(), rs2.getIdx(), rs3.getIdx(), rm,
       s);
  }

  // rdがレジスタのケース
  void Fr(unsigned int opcode, const Reg &rd, const FReg &rs1, const FReg &rs2,
          const FReg &rs3, RoundingMode rm, const char *s) {
    IS_FLOAT_ONLY;
    FL(opcode, 0b00, rd.getIdx(), rs1.getIdx(), rs2.getIdx(), rs3.getIdx(), rm,
       s);
  };

  void Dr(unsigned int opcode, const Reg &rd, const FReg &rs1, const FReg &rs2,
          const FReg &rs3, RoundingMode rm, const char *s) {
    IS_DOUBLE_ONLY;
    FL(opcode, 0b01, rd.getIdx(), rs1.getIdx(), rs2.getIdx(), rs3.getIdx(), rm,
       s);
  };

  // rs1がレジスタのケース
  void Ff(unsigned int opcode, const FReg &rd, const Reg &rs1, const FReg &rs2,
          const FReg &rs3, RoundingMode rm, const char *s) {
    IS_FLOAT_ONLY;
    FL(opcode, 0b00, rd.getIdx(), rs1.getIdx(), rs2.getIdx(), rs3.getIdx(), rm,
       s);
  }
  void Df(unsigned int opcode, const FReg &rd, const Reg &rs1, const FReg &rs2,
          const FReg &rs3, RoundingMode rm, const char *s) {
    IS_DOUBLE_ONLY;
    FL(opcode, 0b01, rd.getIdx(), rs1.getIdx(), rs2.getIdx(), rs3.getIdx(), rm,
       s);
  }

  //////////////////////////////////////////////////////////////////////////////
  // 命令の定義

  // fmadd.*
  class FMADD {
    DOT_CLASS_SETUP(FMADD);

    // fmadd.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2, const FReg &rs3,
           RoundingMode rm = RoundingMode::dyn) {
      IS_FLOAT_ONLY;
      parent.F(0b1000011, rd, rs1, rs2, rs3, rm, "FMADD.S");
    }

    // fmadd.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2, const FReg &rs3,
           RoundingMode rm = RoundingMode::dyn) {
      IS_DOUBLE_ONLY;
      parent.D(0b1000011, rd, rs1, rs2, rs3, rm, "FMADD.D");
    }
  };

  // fmsub.*
  class FMSUB {
    DOT_CLASS_SETUP(FMSUB);

    // fmsub.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2, const FReg &rs3,
           RoundingMode rm = RoundingMode::dyn) {
      IS_FLOAT_ONLY;
      parent.F(0b1000111, rd, rs1, rs2, rs3, rm, "FMSUB.S");
    }

    // fmsub.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2, const FReg &rs3,
           RoundingMode rm = RoundingMode::dyn) {
      IS_DOUBLE_ONLY;
      parent.D(0b1000111, rd, rs1, rs2, rs3, rm, "FMSUB.D");
    }
  };

  // fnmsub.*
  class FNMSUB {
    DOT_CLASS_SETUP(FNMSUB);

    // fnmsub.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2, const FReg &rs3,
           RoundingMode rm = RoundingMode::dyn) {
      IS_FLOAT_ONLY;
      parent.F(0b1001011, rd, rs1, rs2, rs3, rm, "FNMSUB.S");
    }

    // fnmsub.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2, const FReg &rs3,
           RoundingMode rm = RoundingMode::dyn) {
      IS_DOUBLE_ONLY;
      parent.D(0b1001011, rd, rs1, rs2, rs3, rm, "FNMSUB.D");
    }
  };

  // fnmadd.*
  class FNMADD {
    DOT_CLASS_SETUP(FNMADD);

    // fnmadd.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2, const FReg &rs3,
           RoundingMode rm = RoundingMode::dyn) {
      IS_FLOAT_ONLY;
      parent.F(0b1001111, rd, rs1, rs2, rs3, rm, "FNMADD.S");
    }

    // fnmadd.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2, const FReg &rs3,
           RoundingMode rm = RoundingMode::dyn) {
      IS_DOUBLE_ONLY;
      parent.D(0b1001111, rd, rs1, rs2, rs3, rm, "FNMADD.D");
    }
  };

  // fadd.*
  class FADD {
    DOT_CLASS_SETUP(FADD);

    // fadd.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2,
           RoundingMode rm = RoundingMode::dyn) {
      IS_FLOAT_ONLY;
      parent.F(0b1010011, rd, rs1, rs2, parent.f0, rm, "FADD.S");
    }

    // fadd.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2,
           RoundingMode rm = RoundingMode::dyn) {
      IS_DOUBLE_ONLY;
      parent.D(0b1010011, rd, rs1, rs2, parent.f0, rm, "FADD.D");
    }
  };

  // fsub.*
  class FSUB {
    DOT_CLASS_SETUP(FSUB);

    // fsub.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2,
           RoundingMode rm = RoundingMode::dyn) {
      IS_FLOAT_ONLY;
      parent.F(0b1010011, rd, rs1, rs2, parent.f1, rm, "FSUB.S");
    }

    // fsub.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2,
           RoundingMode rm = RoundingMode::dyn) {
      IS_DOUBLE_ONLY;
      parent.D(0b1010011, rd, rs1, rs2, parent.f1, rm, "FSUB.D");
    }
  };

  // fmul.*
  class FMUL {
    DOT_CLASS_SETUP(FMUL);

    // fmul.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2,
           RoundingMode rm = RoundingMode::dyn) {
      IS_FLOAT_ONLY;
      parent.F(0b1010011, rd, rs1, rs2, parent.f2, rm, "FMUL.S");
    }

    // fmul.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2,
           RoundingMode rm = RoundingMode::dyn) {
      IS_DOUBLE_ONLY;
      parent.D(0b1010011, rd, rs1, rs2, parent.f2, rm, "FMUL.D");
    }
  };

  // fdiv.*
  class FDIV {
    DOT_CLASS_SETUP(FDIV);

    // fdiv.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2,
           RoundingMode rm = RoundingMode::dyn) {
      IS_FLOAT_ONLY;
      parent.F(0b1010011, rd, rs1, rs2, parent.f3, rm, "FDIV.S");
    }

    // fdiv.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2,
           RoundingMode rm = RoundingMode::dyn) {
      IS_DOUBLE_ONLY;
      parent.D(0b1010011, rd, rs1, rs2, parent.f3, rm, "FDIV.D");
    }
  };

  // fsqrt.*
  class FSQRT {
    DOT_CLASS_SETUP(FSQRT);

    // fsqrt.s
    void s(const FReg &rd, const FReg &rs1,
           RoundingMode rm = RoundingMode::dyn) {
      IS_FLOAT_ONLY;
      parent.F(0b1010011, rd, rs1, parent.f0, parent.f11, rm, "FSQRT.S");
    }

    // fsqrt.d
    void d(const FReg &rd, const FReg &rs1,
           RoundingMode rm = RoundingMode::dyn) {
      IS_DOUBLE_ONLY;
      parent.D(0b1010011, rd, rs1, parent.f0, parent.f11, rm, "FSQRT.D");
    }
  };

  // fsgnj.*
  class FSGNJ {
    DOT_CLASS_SETUP(FSGNJ);
    // fsgnj.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2,
           RoundingMode rm = RoundingMode::dyn) {
      IS_FLOAT_ONLY;
      const char *msg = "FSGNJ.S";
      if (rs1 == rs2) {
        msg = "FMV.S";
      }
      parent.F(0b1010011, rd, rs1, rs2, parent.f4, rne, msg);
    }

    // fsgnj.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2,
           RoundingMode rm = RoundingMode::dyn) {
      IS_DOUBLE_ONLY;
      const char *msg = "FSGNJ.D";
      if (rs1 == rs2) {
        msg = "FMV.D";
      }
      parent.D(0b1010011, rd, rs1, rs2, parent.f4, rne, msg);
    }
  };

  // fsgnjn.*
  class FSGNJN {
    DOT_CLASS_SETUP(FSGNJN);

    // fsgnjn.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2) {
      IS_FLOAT_ONLY;
      const char *msg = "FSGNJN.S";
      if (rs1 == rs2) {
        msg = "FNEG.S";
      }
      parent.F(0b1010011, rd, rs1, rs2, parent.f4, rtz, msg);
    }

    // fsgnjn.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2) {
      IS_DOUBLE_ONLY;
      const char *msg = "FSGNJN.D";
      if (rs1 == rs2) {
        msg = "FNEG.D";
      }
      parent.D(0b1010011, rd, rs1, rs2, parent.f4, rtz, msg);
    }
  };

  // fsgnjx.*
  class FSGNJX {
    DOT_CLASS_SETUP(FSGNJX);

    // fsgnjx.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2) {
      IS_FLOAT_ONLY;
      const char *msg = "FSGNJX.S";
      if (rs1 == rs2) {
        msg = "FABS.S";
      }
      parent.F(0b1010011, rd, rs1, rs2, parent.f4, rdn, msg);
    }

    // fsgnjx.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2) {
      IS_DOUBLE_ONLY;
      const char *msg = "FSGNJX.D";
      if (rs1 == rs2) {
        msg = "FABS.D";
      }
      parent.D(0b1010011, rd, rs1, rs2, parent.f4, rdn, msg);
    }
  };

  // fmin.*
  class FMIN {
    DOT_CLASS_SETUP(FMIN);

    // fmin.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2) {
      IS_FLOAT_ONLY;
      parent.F(0b1010011, rd, rs1, rs2, parent.f5, rne, "FMIN.S");
    }

    // fmin.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2) {
      IS_DOUBLE_ONLY;
      parent.D(0b1010011, rd, rs1, rs2, parent.f5, rne, "FMIN.D");
    }
  };

  // fmax.*
  class FMAX {
    DOT_CLASS_SETUP(FMAX);

    // fmax.s
    void s(const FReg &rd, const FReg &rs1, const FReg &rs2) {
      IS_FLOAT_ONLY;
      parent.F(0b1010011, rd, rs1, rs2, parent.f5, rtz, "FMAX.S");
    }

    // fmax.d
    void d(const FReg &rd, const FReg &rs1, const FReg &rs2) {
      IS_DOUBLE_ONLY;
      parent.D(0b1010011, rd, rs1, rs2, parent.f5, rtz, "FMAX.D");
    }
  };

  // fcvt.**
  class FCVT {
    friend self_t;

    // fcvt.w.*
    class FCVT_W {
      DOT_CLASS_SETUP(FCVT_W);

      // fcvt.w.s
      void s(const Reg &rd, const FReg &rs1,
             RoundingMode rm = RoundingMode::dyn) {
        IS_FLOAT_ONLY;
        parent.Fr(0b1010011, rd, rs1, parent.f0, parent.f24, rm, "FCVT.W.S");
      }

      // fcvt.w.d
      void d(const Reg &rd, const FReg &rs1,
             RoundingMode rm = RoundingMode::dyn) {
        IS_DOUBLE_ONLY;
        parent.Dr(0b1010011, rd, rs1, parent.f0, parent.f24, rm, "FCVT.W.D");
      }
    };

    // fcvt.wu.*
    class FCVT_WU {
      DOT_CLASS_SETUP(FCVT_WU);

      // fcvt.wu.s
      void s(const Reg &rd, const FReg &rs1,
             RoundingMode rm = RoundingMode::dyn) {
        IS_FLOAT_ONLY;
        parent.Fr(0b1010011, rd, rs1, parent.f1, parent.f24, rm, "FCVT.WU.S");
      }

      // fcvt.wu.d
      void d(const Reg &rd, const FReg &rs1,
             RoundingMode rm = RoundingMode::dyn) {
        IS_DOUBLE_ONLY;
        parent.Dr(0b1010011, rd, rs1, parent.f1, parent.f24, rm, "FCVT.WU.D");
      }
    };

    // fcvt.s.*
    class FCVT_S {
      DOT_CLASS_SETUP(FCVT_S);

      // fcvt.s.w
      void w(const FReg &rd, const Reg &rs1,
             RoundingMode rm = RoundingMode::dyn) {
        IS_FLOAT_ONLY;
        parent.Ff(0b1010011, rd, rs1, parent.f0, parent.f26, rm, "FCVT.S.W");
      }

      // fcvt.s.wu
      void wu(const FReg &rd, const Reg &rs1,
              RoundingMode rm = RoundingMode::dyn) {
        IS_FLOAT_ONLY;
        parent.Ff(0b1010011, rd, rs1, parent.f1, parent.f26, rm, "FCVT.S.WU");
      }

      // fcvt.s.d
      void d(const FReg &rd, const FReg &rs1,
             RoundingMode rm = RoundingMode::dyn) {
        IS_DOUBLE_ONLY;
        parent.F(0b1010011, rd, rs1, parent.f1, parent.f8, rm, "FCVT.S.D");
      }
    };

    // fcvt.d.*
    // 注意
    // "The RISC-V Instruction Set Manual Volume I: Unprivileged ISA Document
    // Version 20191213" の "12.5 Double-Precision Floating-Point Conversion and
    // Move Instructions" の最初の段落の末尾の以下の文より、 fcvt.d.*
    // 命令は丸めモードを引数に取らない実装とした。
    //
    // Note FCVT.D.W[U] always produces an exact result and is unaffected by
    // rounding mode.
    //
    // つまり、IEEE 754の倍精度浮動小数点数の仮数部が52ビットあるため、
    // 32bit整数からの変換時に情報の欠落は発生しないので、丸める必要が無い。
    // また、単精度から倍精度への変換も同様に丸める必要はない。
    // また、gasで逆アセンブルすると、fcvt.d.* 命令は、丸めモードが
    // rne（最近の偶数へ丸める） でないと逆アセンブルに失敗する。そのため、
    // 無条件で rne を指定したオペコードを生成するように実装している。
    class FCVT_D {
      DOT_CLASS_SETUP(FCVT_D);

      // fcvt.d.w
      void w(const FReg &rd, const Reg &rs1) {
        IS_DOUBLE_ONLY;
        parent.Df(0b1010011, rd, rs1, parent.f0, parent.f26, parent.rne,
                  "FCVT.D.W");
      }

      // fcvt.d.wu
      void wu(const FReg &rd, const Reg &rs1) {
        IS_DOUBLE_ONLY;
        parent.Df(0b1010011, rd, rs1, parent.f1, parent.f26, parent.rne,
                  "FCVT.D.WU");
      }

      // fcvt.d.s
      void s(const FReg &rd, const FReg &rs1) {
        IS_DOUBLE_ONLY;
        parent.D(0b1010011, rd, rs1, parent.f0, parent.f8, parent.rne,
                 "FCVT.D.S");
      }
    };

   public:
    FCVT_W w;
    FCVT_WU wu;
    FCVT_S s;
    FCVT_D d;

   private:
    FCVT(self_t &parent) : w(parent), wu(parent), s(parent), d(parent) {}
  };

  // fmv.**
  class FMV {
    friend self_t;
    self_t &parent;
    // fmv.x.w
    class FMV_X {
      DOT_CLASS_SETUP(FMV_X);
      void w(const Reg &rd, const FReg &rs1) {
        IS_FLOAT_ONLY;
        parent.Fr(0b1010011, rd, rs1, parent.f0, parent.f28, rne, "FMV.X.W");
      }
    };

    // fmv.w.x
    class FMV_W {
      DOT_CLASS_SETUP(FMV_W);
      void x(const FReg &rd, const Reg &rs1) {
        IS_FLOAT_ONLY;
        parent.Ff(0b1010011, rd, rs1, parent.f0, parent.f30, rne, "FMV.W.X");
      }
    };

   public:
    FMV_X x;
    FMV_W w;

    // fmv.s 疑似命令
    void s(const FReg &rd, const FReg &rs1) { parent.fsgnj.s(rd, rs1, rs1); }

    // fmv.d 疑似命令
    void d(const FReg &rd, const FReg &rs1) { parent.fsgnj.d(rd, rs1, rs1); }

   private:
    FMV(self_t &parent) : parent(parent), x(parent), w(parent) {}
  };

  // feq.*
  class FEQ {
    DOT_CLASS_SETUP(FEQ);

    // feq.s
    void s(const Reg &rd, const FReg &rs1, const FReg &rs2) {
      IS_FLOAT_ONLY;
      parent.Fr(0b1010011, rd, rs1, rs2, parent.f20, rdn, "FEQ.S");
    }

    // feq.d
    void d(const Reg &rd, const FReg &rs1, const FReg &rs2) {
      IS_DOUBLE_ONLY;
      parent.Dr(0b1010011, rd, rs1, rs2, parent.f20, rdn, "FEQ.D");
    }
  };

  // flt.*
  class FLT {
    DOT_CLASS_SETUP(FLT);

    // flt.s
    void s(const Reg &rd, const FReg &rs1, const FReg &rs2) {
      IS_FLOAT_ONLY;
      parent.Fr(0b1010011, rd, rs1, rs2, parent.f20, rtz, "FLT.S");
    }

    // flt.d
    void d(const Reg &rd, const FReg &rs1, const FReg &rs2) {
      IS_DOUBLE_ONLY;
      parent.Dr(0b1010011, rd, rs1, rs2, parent.f20, rtz, "FLT.D");
    }
  };

  // fle.*
  class FLE {
    DOT_CLASS_SETUP(FLE);

    // fle.s
    void s(const Reg &rd, const FReg &rs1, const FReg &rs2) {
      IS_FLOAT_ONLY;
      parent.Fr(0b1010011, rd, rs1, rs2, parent.f20, rne, "FLE.S");
    }

    // fle.d
    void d(const Reg &rd, const FReg &rs1, const FReg &rs2) {
      IS_DOUBLE_ONLY;
      parent.Dr(0b1010011, rd, rs1, rs2, parent.f20, rne, "FLE.D");
    }
  };

  // flcss.*
  class FCLASS {
    DOT_CLASS_SETUP(FCLASS);

    // flcss.s
    void s(const Reg &rd, const FReg &rs1) {
      IS_FLOAT_ONLY;
      parent.Fr(0b1010011, rd, rs1, parent.f0, parent.f28, rtz, "FCLASS.S");
    }

    // flcss.d
    void d(const Reg &rd, const FReg &rs1) {
      IS_DOUBLE_ONLY;
      parent.Dr(0b1010011, rd, rs1, parent.f0, parent.f28, rtz, "FCLASS.D");
    }
  };

  //////////////////////////////////////////////////////////////////////////////
  // 疑似命令の実装専用のクラスの定義

  // fabs.*
  class FABS {
    DOT_CLASS_SETUP(FABS);

    // fabs.s 疑似命令
    void s(const FReg &rd, const FReg &rs1) { parent.fsgnjx.s(rd, rs1, rs1); }

    // fabs.d 疑似命令
    void d(const FReg &rd, const FReg &rs1) { parent.fsgnjx.d(rd, rs1, rs1); }
  };

  // fneg.*
  class FNEG {
    DOT_CLASS_SETUP(FNEG);

    // fneg.s 疑似命令
    void s(const FReg &rd, const FReg &rs1) { parent.fsgnjn.s(rd, rs1, rs1); }

    // fneg.d 疑似命令
    void d(const FReg &rd, const FReg &rs1) { parent.fsgnjn.d(rd, rs1, rs1); }
  };

 public:
  FMADD fmadd;
  FMSUB fmsub;
  FNMSUB fnmsub;
  FNMADD fnmadd;
  FADD fadd;
  FSUB fsub;
  FMUL fmul;
  FDIV fdiv;
  FSQRT fsqrt;
  FSGNJ fsgnj;
  FSGNJN fsgnjn;
  FSGNJX fsgnjx;
  FMIN fmin;
  FMAX fmax;
  FCVT fcvt;
  FMV fmv;
  FEQ feq;
  FLT flt;
  FLE fle;
  FCLASS fclass;
  //
  FABS fabs;
  FNEG fneg;

  CodeGenerator32Float<T>()
      : T(),
        fmadd(*this),
        fmsub(*this),
        fnmsub(*this),
        fnmadd(*this),
        fadd(*this),
        fsub(*this),
        fmul(*this),
        fdiv(*this),
        fsqrt(*this),
        fsgnj(*this),
        fsgnjn(*this),
        fsgnjx(*this),
        fmin(*this),
        fmax(*this),
        fcvt(*this),
        fmv(*this),
        feq(*this),
        flt(*this),
        fle(*this),
        fclass(*this),
        //
        fabs(*this),
        fneg(*this) {}

  // flw + c.flw + c.flwsp
  void flw(const FReg &rd, const OffsetReg32 &or1) {
    IS_FLOAT_ONLY;
    auto imm = or1.getOffset();
    if (SUPPORT_COMP && rd.isCReg() && or1.getReg().isCReg() &&
        (imm & 0x7c) == imm) {
      imm &= 0x7c;
      unsigned int op = (0b011 << 13) | ((imm & 0x38) << 7) |
                        (or1.getReg().getCIdx() << 7) |  //
                        ((imm & 0x04) << 4) | ((imm & 0x40) >> 1) |
                        (rd.getCIdx()) << 2;
      T::C(op, "C.FLW");
    } else if (SUPPORT_COMP && or1.getReg() == sp && ((imm & 0x0fc) == imm)) {
      imm &= 0x0fc;
      unsigned int op = (0b011 << 13) | ((imm & 0x020) << 7) |
                        (rd.getIdx() << 7) |  //
                        ((imm & 0x01c) << 2) | ((imm & 0x0c0) >> 4) | 0b10;
      T::C(op, "C.FLWSP");
    } else {
      LD(or1.getOffset(), or1.getIdx(), 0b010, rd.getIdx(), "FLW");
    }
  }

  // fsw + c.fsw + c.fswsp
  void fsw(const FReg &rs2, const OffsetReg32 &or1) {
    IS_FLOAT_ONLY;
    auto imm = or1.getOffset();
    if (SUPPORT_COMP && rs2.isCReg() && or1.getReg().isCReg() &&
        (imm & 0x7c) == imm) {
      imm &= 0x7c;
      unsigned int op = (0b111 << 13) | ((imm & 0x38) << 7) |
                        (or1.getReg().getCIdx() << 7) |  //
                        ((imm & 0x04) << 4) | ((imm & 0x40) >> 1) |
                        (rs2.getCIdx()) << 2;
      T::C(op, "C.FSW");
    } else if (SUPPORT_COMP && or1.getReg() == sp && ((imm & 0x0fc) == imm)) {
      imm &= 0x0fc;
      unsigned int op = (0b111 << 13) | ((imm & 0x3c) << 7) |
                        ((imm & 0xc0) << 1) | (rs2.getCIdx()) << 2 | 0b10;
      T::C(op, "C.FSWSP");
    } else {
      ST(or1.getOffset(), rs2.getIdx(), or1.getIdx(), 0b010, "FSW");
    }
  }

  // fld + c.fld + c.fldsp
  void fld(const FReg &rd, const OffsetReg32 &or1) {
    IS_DOUBLE_ONLY;
    auto imm = or1.getOffset();
    if (SUPPORT_COMP && rd.isCReg() && or1.getReg().isCReg() &&
        (imm & 0xf8) == imm) {
      imm &= 0xf8;
      unsigned int op = (0b001 << 13) | ((imm & 0x38) << 7) |
                        (or1.getReg().getCIdx() << 7) |  //
                        ((imm & 0xc0) >> 1) | (rd.getCIdx()) << 2;
      T::C(op, "C.FLD");
    } else if (SUPPORT_COMP && or1.getReg() == sp && ((imm & 0x1f8) == imm)) {
      imm &= 0x1f8;
      unsigned int op = (0b001 << 13) | ((imm & 0x020) << 7) |
                        (rd.getIdx() << 7) |  //
                        ((imm & 0x018) << 2) | ((imm & 0x1c0) >> 4) | 0b10;
      T::C(op, "C.FLDSP");
    } else {
      LD(or1.getOffset(), or1.getIdx(), 0b011, rd.getIdx(), "FLD");
    }
  }

  // fsd + c.fsd + c.fsdsp
  void fsd(const FReg &rs2, const OffsetReg32 &or1) {
    IS_DOUBLE_ONLY;
    auto imm = or1.getOffset();
    if (SUPPORT_COMP && rs2.isCReg() && or1.getReg().isCReg() &&
        (imm & 0xf8) == imm) {
      imm &= 0xf8;
      unsigned int op = (0b101 << 13) | ((imm & 0x38) << 7) |
                        (or1.getReg().getCIdx() << 7) |  //
                        ((imm & 0xc0) >> 1) | (rs2.getCIdx()) << 2;
      T::C(op, "C.FSD");
    } else if (SUPPORT_COMP && or1.getReg() == sp && ((imm & 0x1f8) == imm)) {
      imm &= 0x1f8;
      unsigned int op = (0b101 << 13) | ((imm & 0x020) << 7) |
                        ((imm & 0x038) << 7) | ((imm & 0x1c0) << 1) |
                        (rs2.getIdx() << 2) | 0b10;
      T::C(op, "C.FSDSP");
    } else {
      ST(or1.getOffset(), rs2.getIdx(), or1.getIdx(), 0b011, "FSD");
    }
  }

#undef IS_FLOAT_ONLY
#undef IS_DOUBLE_ONLY
#undef IS_QUADRUPLE_ONLY
#undef SUPPORT_COMP
};
};  // namespace RV32_asm

#endif
