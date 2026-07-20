all: bin/ekf_test
	./bin/ekf_test

C_SRCS = ../AP_BattEkfImpl.c ../AP_BattCagi.c ../AP_BattRemTimeCalc.c
C_OBJS = $(C_SRCS:.c=.o)

%.o: %.c
	clang -ggdb -fsanitize=address -c -o $@ $< -I../ -Imock/

bin/ekf_test: ekf_test.cpp $(C_OBJS)
	clang++ -ggdb -fsanitize=address -o bin/ekf_test ekf_test.cpp $(C_OBJS) -I../ -Imock/
