#ifndef RV32_ASM_HPP_INCLUDED
#define RV32_ASM_HPP_INCLUDED

////////////////////////////////////////////////////////////////////////////////
// 実際の命令セットを定義しているヘッダファイルのインクルード

#include "RV32_asm_A.hpp"
#include "RV32_asm_C.hpp"
#include "RV32_asm_D.hpp"
#include "RV32_asm_F.hpp"
#include "RV32_asm_I.hpp"
#include "RV32_asm_M.hpp"
#include "RV32_asm_float.hpp"

////////////////////////////////////////////////////////////////////////////////
// ライブラリの定義

namespace RV32_asm {

class Generator : public virtual Base {
  static void clear_cache() {
#if TARGET == TARGET_RISCV
#if COMPILER == COMPILER_GCC
    asm volatile("fence.i" ::: "memory");
#endif
#endif
  }

 public:
  Generator() : Base() {}

  //////////////////////////////////////////////////////////////////
  // コード生成関数

  void dw(uint32_t word) {
    env << [=](Env &e) { e.dw(word, ".long"); };
  }

  void dh(unsigned int hw) {
    env << [=](Env &e) { e.dh(hw, ".word"); };
  }

  // 生成したコードをテンプレートで指定された関数ポインタとして返す
  template <typename T>
  T generate() {
    auto p = alloc.getMemory();
    size_t code_size = env.generate(p, alloc.getSize());
    clear_cache();
#if IN_DEBUG_MODE
    printf("%d byte code generated at %p.\n", (int)code_size, p);
    for (size_t i = 0; i < code_size; ++i) {
      printf("%02x%s", p[i], (i % 16) == 15 ? "\n" : " ");
    }
    puts("");
#endif
    return (T)p;
  }

  // 生成したコードを返す
  const unsigned char *getCode(size_t *pSize) {
    auto p = alloc.getMemory();
    size_t code_size = env.generate(p, alloc.getSize());
    clear_cache();
#if IN_DEBUG_MODE
    printf("%d byte code generated at %p.\n", (int)code_size, p);
    for (size_t i = 0; i < code_size; ++i) {
      printf("%02x%s", p[i], (i % 16) == 15 ? "\n" : " ");
    }
    puts("");
#endif
    if (pSize != NULL) {
      *pSize = code_size;
    }
    return p;
  }
};

// 命令セットに応じたコード生成クラスを定義するテンプレート
template <char... Cs>
struct ISA32 : virtual public Base,
               public CodeGenerator32Float<typename RV32<Cs...>::type> {
  ISA32(size_t size = DEFAULT_MAX_CODE_SIZE, void *ptr = NULL) {
    alloc.allocate(size, ptr);
  }
};
// よく使われそうな命令セットの組み合わせのクラスの定義
typedef ISA32</**********************/ 'I', '$'> RV32I;
typedef ISA32</*****************/ 'M', 'I', '$'> RV32IM;
typedef ISA32</************/ 'A', 'M', 'I', '$'> RV32IMA;
typedef ISA32</*******/ 'F', 'A', 'M', 'I', '$'> RV32IMAF;
typedef ISA32</**/ 'D', 'F', 'A', 'M', 'I', '$'> RV32IMAFD;
typedef ISA32<'C', 'D', 'F', 'A', 'M', 'I', '$'> RV32IMAFDC;

typedef RV32IMAFD RV32G;
typedef RV32IMAFDC RV32GC;
};  // namespace RV32_asm

#endif
