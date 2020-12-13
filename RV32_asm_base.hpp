#ifndef RV32_ASM_BASE_HPP_INCLUDED
#define RV32_ASM_BASE_HPP_INCLUDED

////////////////////////////////////////////////////////////////////////////////
// デバッグモードの判定

// デバッグモードの設定
// マクロ DEBUG が存在すればその値で判定、
// 存在しない場合は、マクロ NDEBUG が定義されていたら
// デバッグモードにする。それら以外の状態では通常モードになる。
#ifdef DEBUG
#define IN_DEBUG_MODE DEBUG
#endif
#ifndef IN_DEBUG_MODE
#if NDEBUG
#define IN_DEBUG_MODE 0
#else
#define IN_DEBUG_MODE 1
#endif
#endif

////////////////////////////////////////////////////////////////////////////////
// コンパイル環境の確認

// ターゲットCPU
#define TARGET_RISCV 1
#define TARGET_OTHER 2

#ifdef __GNUC__
#if __riscv
#define TARGET TARGET_RISCV
#else
#define TARGET TARGET_OTHER
#endif
#endif
#ifndef TARGET
#define TARGET TARGET_OTHER
#endif

// コンパイラ種別
#define COMPILER_GCC 1
#define COMPILER_MS 2
#define COMPILER_UNKNOWN 3

#ifdef __GNUC__
#define COMPILER COMPILER_GCC
#else
#if _MSC_VER
#define COMPILER COMPILER_MS
#endif
#endif
#ifndef COMPILER
#define COMPILER COMPILER_UNKNOWN 3

#endif

#if not +0
// and or not を関数名として使用できる設定になっているか、
// 本家と同じ手法でエラー判定する
#error "use -fno-operator-names"
#endif


////////////////////////////////////////////////////////////////////////////////
// ヘッダのインクルード

#if IN_DEBUG_MODE
#include <cstdio>  // デバッグ用
#endif
#include <assert.h>

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <string>

// xx.y のような名前の命令を定義するためのマクロ定義
// 手法として、 xx という変数に y
// という名前のメンバー関数を持たせる形で実装する。 xx
// に該当するクラスの用意が若干面倒なので、
// マクロですこし簡単に書けるようにした。
// 使用例は↓のコメントを参照。
#define DOT_CLASS_SETUP(klass)              \
  friend self_t;                            \
  self_t &parent;                           \
  klass(self_t &parent) : parent(parent) {} \
                                            \
 public:


namespace RV32_asm {
// 整数型の定義
typedef int_least32_t int32_t;
typedef uint_least32_t uint32_t;
typedef uint_least16_t uint16_t;
typedef int32_t address_offset_t;

// 定数の定義
enum {
  DEFAULT_MAX_CODE_SIZE = 4096,
  VERSION = 0x0100 /* 0xABCD = A.BC(D) */
};

// クラスの前方宣言
class Base;
class Generator;

namespace internal {
class Operand;
class Reg;
class OffsetReg32;
class FReg;

class Env;
class Label;
class Allocator;

////////////////////////////////////////////////////////////////////////////////

class Operand {
  unsigned int idx_ : 6;

 protected:
  void setIdx(int idx) { idx_ = idx; }

 public:
  constexpr int getIdx() const { return idx_; }
  Operand(int idx = 0) : idx_(idx) {}
};

class OffsetReg32 : public Operand {
  const Reg &reg;
  const address_offset_t offset;

 public:
  OffsetReg32(const Reg &reg, address_offset_t offset)
      : reg(reg), offset(offset) {}

  constexpr int getIdx()
      const;  // 実装の都合でReg32クラスの定義の後で関数を定義する

  constexpr const Reg &getReg() const { return reg; }
  constexpr address_offset_t getOffset() const { return offset; }
};

class Reg : public Operand {
  unsigned int cidx : 4;  // C命令セットで使用するレジスタ番号
 public:
  Reg() : Operand(), cidx(15) {}
  Reg(int idx) : Operand(idx), cidx(15) {}
  Reg(int idx, int cidx) : Operand(idx), cidx(cidx) {}
  OffsetReg32 operator[](address_offset_t offset) const {
    return OffsetReg32(*this, offset);
  }
  OffsetReg32 operator()(address_offset_t offset) const {
    return OffsetReg32(*this, offset);
  }
  Reg operator()() const { return *this; }

  bool operator==(const Reg &o) const { return this->getIdx() == o.getIdx(); }
  bool operator!=(const Reg &o) const { return this->getIdx() != o.getIdx(); }
  bool isCReg() const { return cidx != 15; }
  int getCIdx() const { return cidx; }
};

constexpr int OffsetReg32::getIdx() const { return reg.getIdx(); }

class FReg : public Operand {
  unsigned int cidx : 4;  // C命令セットで使用するレジスタ番号
 public:
  FReg(int idx) : Operand(idx), cidx(15) {}
  FReg(int idx, int cidx) : Operand(idx), cidx(cidx) {}

  bool operator==(const FReg &o) const { return this->getIdx() == o.getIdx(); }
  bool operator!=(const FReg &o) const { return this->getIdx() != o.getIdx(); }
  bool isCReg() const { return cidx != 15; }
  int getCIdx() const { return cidx; }
};

class Env {
  typedef std::map<std::string, address_offset_t> LabelMap;
  typedef std::function<void(Env &)> InsnGen_type;
  address_offset_t offset;
  LabelMap labels;
  std::list<InsnGen_type> insns;
  bool inGenerate;
  Base *pGen;

  // 生成したコードを入れる領域
  unsigned char *code;
  size_t remining;

  void write32(uint32_t dw) {
    assert(4 <= this->remining);
    this->remining -= 4;
    *(this->code++) = dw;
    *(this->code++) = dw >> 8;
    *(this->code++) = dw >> 16;
    *(this->code++) = dw >> 24;
  }

  void write16(uint16_t hw) {
    assert(2 <= this->remining);
    this->remining -= 2;
    *(this->code++) = hw;
    *(this->code++) = hw >> 8;
  }

  void write8(unsigned char b) {
    assert(1 <= this->remining);
    this->remining -= 1;
    *(this->code++) = b;
  }

 public:
  Env(Base *pGen)
      : offset(0),
        labels(),
        insns(),
        inGenerate(false),
        pGen(pGen),
        code(NULL),
        remining(0) {}

  void AddLabel(const std::string &s) {
#if IN_DEBUG_MODE
    printf("%s: %+d\n", s.c_str(), int(offset));
#endif
    labels[s] = offset;
  }
  address_offset_t getOffset(const Label &label) const;
  void operator<<(InsnGen_type ig) {
    ig(*this);
    insns.push_back(ig);
  }

  // コードを生成して、codeに書き込み、書き込んだバイト数を返す
  size_t generate(unsigned char *code, size_t code_size) {
    this->code = code;
    this->remining = code_size;

    offset = 0;
    inGenerate = true;
    for (auto ig : insns) {
      ig(*this);
    }
    inGenerate = false;
    return code_size - remining;
  }

  void dh(unsigned int op, const char *msg = "") {
    offset += 2;

    if (inGenerate) {
      write16(op);
#if IN_DEBUG_MODE
      printf(
          "                                                        0x%04x %s\n",
          op, msg);
#endif
    }
  }

  void dw(uint32_t op, const char *msg = "") {
    offset += 4;

    if (inGenerate) {
      write32(op);

#if IN_DEBUG_MODE
      static const char *tbl[16] = {
          "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
          "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111",
      };
      char buf[64] = {0};
      sprintf(buf, "%s%s%s%s%s%s%s%s",                     //
              tbl[(op >> 28) & 15], tbl[(op >> 24) & 15],  //
              tbl[(op >> 20) & 15], tbl[(op >> 16) & 15],  //
              tbl[(op >> 12) & 15], tbl[(op >> 8) & 15],   //
              tbl[(op >> 4) & 15], tbl[(op >> 0) & 15]);
      char buf2[] = "....... 2[.....] 1[.....] ... D[.....] OP[.......]";
      char *in = buf;
      char *out = buf2;
      while (*out != '\0' && *in != '\0') {
        if (*out == '.') {
          *out = *in++;
        }
        ++out;
      }
      printf("%s  0x%08x %s\n", buf2, op, msg);
#endif
    }
  }
};

class Label {
  std::string label;
  address_offset_t force_offset;

 public:
  Label(const char *label) : label(label), force_offset(0) {
    if (label == 0) {
      label = "";
      force_offset = 0;
    }
  }
  explicit Label(const std::string &label) : label(label), force_offset(0) {}
  explicit Label(address_offset_t offset) : label(""), force_offset(offset) {}

  address_offset_t getOffset(const Env &e) const {
    if (label.empty()) {
      return force_offset;
    } else {
      return e.getOffset(*this);
    }
  }
  std::string getLabel() const { return label; }
};  // namespace RV32_asm

address_offset_t Env::getOffset(const Label &label) const {
  auto itr = labels.find(label.getLabel());
  if (inGenerate) {
    assert(itr != labels.end());
    return itr->second - this->offset;
  } else {
    return 0;
  }
}

class Allocator {
  // 根本の原因は判らないが、spike で動作確認を行っていると
  // メモリに書き込んだ命令をうまく読みだせず落ちる。
  // 試行錯誤した結果、new したメモリ領域から2048バイトにアライメント
  // したメモリを使用するとうまく動いたので対症療法として
  // アライメント処理を追加した。
  const size_t ALIGN = 2048;

  size_t size;
  void *ptr;
  bool self_allocated;  // メモリ確保を自前でやったフラグ
 public:
  Allocator() : size(0), ptr(NULL), self_allocated(false) {}

  virtual ~Allocator() {
    if (self_allocated) {
      delete[](unsigned char *) this->ptr;
    }
  }

  void allocate(size_t size, void *ptr) {
    this->size = size;
    this->ptr = ptr;

    if (this->ptr == NULL) {
      this->ptr = new unsigned char[size + ALIGN];
      self_allocated = true;
    }
    assert(this->ptr != NULL);
  }

  unsigned char *getMemory() const {
    assert(ptr != NULL);
    // アライメント調整
    intptr_t p = (intptr_t)ptr;
    p = (p + ALIGN - 1) & (~(ALIGN - 1));
    return (unsigned char *)p;
  }
  size_t getSize() const { return size; }
};  // namespace internal
};  // namespace internal
using namespace internal;

class Base {
 protected:
  Allocator alloc;
  Env env;
  enum { float_mode = 0 };

  virtual void C(const int op, const char *msg = "") { assert(false); }

 public:
  // レジスタ
  const Reg x0, x1, x2, x3, x4, x5, x6, x7,    //
      x8, x9, x10, x11, x12, x13, x14, x15,    //
      x16, x17, x18, x19, x20, x21, x22, x23,  //
      x24, x25, x26, x27, x28, x29, x30, x31;

  //レジスタの別名
  const Reg zero, ra, sp, gp, tp, t0, t1,      //
      t2, s0, fp, s1, a0, a1, a2, a3, a4, a5,  //
      a6, a7, s2, s3, s4, s5, s6, s7,          //
      s8, s9, s10, s11, t3, t4, t5, t6;

  // 浮動小数点レジスタ
  const FReg f0, f1, f2, f3, f4, f5, f6, f7,   //
      f8, f9, f10, f11, f12, f13, f14, f15,    //
      f16, f17, f18, f19, f20, f21, f22, f23,  //
      f24, f25, f26, f27, f28, f29, f30, f31;

  const FReg ft0, ft1, ft2, ft3, ft4, ft5, ft6, ft7,  //
      fs0, fs1, fa0, fa1, fa2, fa3, fa4, fa5,         //
      fa6, fa7, fs2, fs3, fs4, fs5, fs6, fs7,         //
      fs8, fs9, fs10, fs11, ft8, ft9, ft10, ft11;

  Base()
      : alloc(),
        env(this),
        x0(0),
        x1(1),
        x2(2),
        x3(3),
        x4(4),
        x5(5),
        x6(6),
        x7(7),
        x8(8, 0),
        x9(9, 1),
        x10(10, 2),
        x11(11, 3),
        x12(12, 4),
        x13(13, 5),
        x14(14, 6),
        x15(15, 7),
        x16(16),
        x17(17),
        x18(18),
        x19(19),
        x20(20),
        x21(21),
        x22(22),
        x23(23),
        x24(24),
        x25(25),
        x26(26),
        x27(27),
        x28(28),
        x29(29),
        x30(30),
        x31(31),  //
        zero(0),
        ra(1),
        sp(2),
        gp(3),
        tp(4),
        t0(5),
        t1(6),
        t2(7),
        s0(8, 0),
        fp(8, 0),
        s1(9, 1),
        a0(10, 2),
        a1(11, 3),
        a2(12, 4),
        a3(13, 5),
        a4(14, 6),
        a5(15, 7),
        a6(16),
        a7(17),
        s2(18),
        s3(19),
        s4(20),
        s5(21),
        s6(22),
        s7(23),
        s8(24),
        s9(25),
        s10(26),
        s11(27),
        t3(28),
        t4(29),
        t5(30),
        t6(31),  //
        f0(0),
        f1(1),
        f2(2),
        f3(3),
        f4(4),
        f5(5),
        f6(6),
        f7(7),
        f8(8, 0),
        f9(9, 1),
        f10(10, 2),
        f11(11, 3),
        f12(12, 4),
        f13(13, 5),
        f14(14, 6),
        f15(15, 7),
        f16(16),
        f17(17),
        f18(18),
        f19(19),
        f20(20),
        f21(21),
        f22(22),
        f23(23),
        f24(24),
        f25(25),
        f26(26),
        f27(27),
        f28(28),
        f29(29),
        f30(30),
        f31(31),  //
        ft0(0),
        ft1(1),
        ft2(2),
        ft3(3),
        ft4(4),
        ft5(5),
        ft6(6),
        ft7(7),
        fs0(8, 0),
        fs1(9, 1),
        fa0(10, 2),
        fa1(11, 3),
        fa2(12, 4),
        fa3(13, 5),
        fa4(14, 6),
        fa5(15, 7),
        fa6(16),
        fa7(17),
        fs2(18),
        fs3(19),
        fs4(20),
        fs5(21),
        fs6(22),
        fs7(23),
        fs8(24),
        fs9(25),
        fs10(26),
        fs11(27),
        ft8(28),
        ft9(29),
        ft10(30),
        ft11(31) {}

  unsigned int getVersion() const { return VERSION; }

  //////////////////////////////////////////////////////////////////
  // ラベル関係の関数

  void L(const std::string &label) { env.AddLabel(label); }
  void inLocalLabel() {}
  void outLocalLabel() {}
};

////////////////////////////////////////////////////////////////////////////////
// 命令セット毎のクラスを生成するためのテンプレートの定義

/*
head = 'RV32I' | 'RV32E' | 'RV64I' | 'RV128I'
IMAFD(G)QLCBJTPVNXabcSdefSXghi
*/

namespace /* anonymous */ {
// 命令セットのアルファベットを実際のクラスに変換しながら再帰的にクラスを構築する
// namespaceの外から使用できないようにするため、無名名前空間の中で定義する
template <char CAR, char... CDR>
struct RV32 {
  typedef void type;
};

// REGIST Instraction Set
#define REGIST_IS(ch, klass)                         \
  template <char... CDR>                             \
  struct RV32<ch, CDR...> {                          \
    typedef klass<typename RV32<CDR...>::type> type; \
  }
template <>
struct RV32<'$'> {
  typedef RV32_asm::Generator type;
};

};  // namespace

};  // namespace RV32_asm

#endif
