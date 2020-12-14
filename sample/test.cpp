#define DEBUG 1
#include "RV32_asm.hpp"

class Test : public RV32_asm::RV32GC {
  void operator=(const Test &);

 public:
  Test(size_t size = RV32_asm::DEFAULT_MAX_CODE_SIZE, void *userPtr = 0)
      : RV32_asm::RV32GC(size, userPtr) {
    // memcpy
    add(a2, a0, a2);
    mv(a5, a0);
    L(".L59");
    bne(a5, a2, ".L60");
    ret();
    L(".L60");
    addi(a1, a1, 1);
    lbu(a4, a1[-1]);
    addi(a5, a5, 1);
    sb(a4, a5[-1]);
    j(".L59");
  }
};

int main(void) {
  Test t;
  printf("Version=%04x\n", t.getVersion());
  auto *func = t.generate<void (*)(void *, void *, size_t)>();

  char a[] = "0123456789ABCDEF";
  char b[] = "abcdefghijklmnopqrstuvwxyz";
  printf("a[]=%s\nb[]=%s\n", a, b);
#if TARGET == TARGET_RISCV
  printf("Execute generated code.\n");
  func(b, a, sizeof(a));
#else
  printf("Skip execution %p.\n", func);
#endif
  printf("a[]=%s\nb[]=%s\n", a, b);
}
