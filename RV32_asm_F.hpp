#ifndef RV32_ASM_F_HPP_INCLUDED
#define RV32_ASM_F_HPP_INCLUDED

#include "RV32_asm_base.hpp"

namespace RV32_asm {

////////////////////////////////////////////////////////////////////////////////
// 単精度小数命令セットの定義

template <typename T = Generator>
class CodeGenerator32F : public virtual Base, public T {
  typedef CodeGenerator32F<T> self_t;

 protected:
  enum { float_mode = T::float_mode | 1 };

 public:
  CodeGenerator32F<T>() : T() {}
};

REGIST_IS('F', RV32_asm::CodeGenerator32F);

};  // namespace RV32_asm
#endif
