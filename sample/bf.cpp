#define DEBUG 1
#include <cstdio>
#include <stack>
#include <string>

#include "RV32_asm.hpp"

using namespace std;

typedef unsigned char uchar;

static void put(int ch) { putchar(ch); }
static int getch(void) {
  while (true) {
    int ch = getchar();
    if (ch != EOF) return ch;
  }
}

class Bf : public RV32_asm::RV32GC {
  void operator=(const Bf &);

  int label_count;
  uchar mem[10000];

  string getLabel() {
    char buf[16];
    sprintf(buf, ".L%d", label_count++);
    return buf;
  }

 public:
  Bf(const char *src)
      : RV32_asm::RV32GC(RV32_asm::DEFAULT_MAX_CODE_SIZE, 0),
        label_count(0),
        mem{0} {
    // レジスタの使い方
    // a0 : メモリアクセス用一時領域・関数呼び出しの引数/戻り値
    // s1 : ポインタ
    // s2 : put 関数
    // s3 : get 関数

    // [ ] 命令の入れ子管理用スタック
    stack<string> par;

    // 保存しておく必要があるレジスタの内容をスタックに退避する
    addi(sp, sp, -32);
    sw(ra, sp[24]);
    sw(s0, sp[20]);
    sw(s1, sp[16]);
    sw(s2, sp[12]);
    sw(s3, sp[8]);
    addi(s0, sp, 32);

    li(s1, (intptr_t) & (this->mem[0]));
    li(s2, (intptr_t)put);
    li(s3, (intptr_t)getch);

    /** 現在ポインタが指しているメモリと a0
     * レジスタの内容が一致しているかを表すフラグ*/
    bool store = false;

    // 同一の命令が連続している場合の最適化処理用の変数
    /** 未処理の命令の文字コード*/
    char code = '\0';
    /** 未処理の命令の繰り返し回数 */
    int count = 0;

    // JITコンパイルメインループ
    for (const char *p = src;; ++p) {
      // 最後の命令の読み出し後にコンパイル未完了な命令がcode/countに残る可能性があるので
      // for文内でループを抜けず、未処理の命令の処理が終わるタイミングでbreakする

      // 未処理の命令を最適化したコードとして出力する
      if (*p != code && (0 < count && code != '\0')) {
        switch (code) {
          case '>':
            addi(s1, s1, count);
            store = false;
            break;
          case '<':
            addi(s1, s1, -count);
            store = false;
            break;
          case '+':
            if (!store) {
              lbu(a0, s1[0]);
            }
            addi(a0, a0, count);
            sb(a0, s1[0]);
            store = true;
            break;
          case '-':
            if (!store) {
              lbu(a0, s1[0]);
            }
            addi(a0, a0, -count);
            sb(a0, s1[0]);
            store = false;
            break;
        }
        code = '\0';
        count = 0;
      }

      // メインループの終了判定
      if (*p == '\0') {
        break;
      }

      // 命令の読み込み
      switch (*p) {
        case '<':
        case '>':
        case '+':
        case '-':
          code = *p;
          count++;
          break;
        case '[': {
          string l = getLabel();
          par.push(l);

          L(l + "B");
          lbu(a0, s1[0]);
          beqz(a0, (l + "E").c_str());

          store = false;
          break;
        }
        case ']': {
          string l = par.top();
          par.pop();
          j((l + "B").c_str());
          L(l + "E");

          store = false;
          break;
        }
        case '.':
          if (!store) {
            lbu(a0, s1[0]);
          }
          jalr(ra, s2(0));

          store = false;
          break;
        case ',':
          jalr(ra, s3(0));
          sb(a0, s1[0]);

          store = true;
          break;
        default:
          break;
      }
    }

    // 保存しておく必要があったレジスタにスタックから書き戻す
    lw(s3, sp[8]);
    lw(s2, sp[12]);
    lw(s1, sp[16]);
    lw(s0, sp[20]);
    lw(ra, sp[24]);
    addi(sp, sp, 32);
    ret();
  }

  void exec() {
    auto *func = this->generate<void (*)(void)>();
    func();
  }
};

int main(void) {
  const char *hello_world =
      "+++++++++[>++++++++>+++++++++++>+++>+<<<<-]>.>++.+++++++..+++.>+++++.<<+"
      "++++++++++++++.>.+++.------.--------.>+.>+.";
  Bf *o = new Bf(hello_world);
  o->exec();
  printf("INPUT:%s\n", hello_world);
}
