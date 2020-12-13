#ifndef RV32_ASM_D_HPP_INCLUDED
#define RV32_ASM_D_HPP_INCLUDED

#include "RV32_asm_base.hpp"

namespace RV32_asm {

////////////////////////////////////////////////////////////////////////////////
// 倍精度小数命令セットの定義

template <typename T = Generator>
class CodeGenerator32D : public virtual Base, public T {
  typedef CodeGenerator32D<T> self_t;

 protected:
  enum { float_mode = T::float_mode | 2 };

 public:
  CodeGenerator32D<T>() : T() {}
};

REGIST_IS('D', RV32_asm::CodeGenerator32D);

};  // namespace RV32_asm
#endif
