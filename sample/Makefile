CC=riscv32-unknown-elf-gcc
CPP=riscv32-unknown-elf-g++
OBJDUMP=riscv32-unknown-elf-objdump
INCS=../RV32_asm*.hpp
SRCS=*.cpp
OUTS=$(SRCS:.cpp=.out)

.PHONY:	all clean

all: test.out bf.out ;

clean:
	-rm $(OUTS)

test: test.out
	spike --isa=rv32gc pk $^

bf: bf.out
	spike --isa=rv32gc pk $^

%.out: %.cpp
	$(CPP) $^ -o $@ -I.. -march=rv32ima -O2 -fno-operator-names


