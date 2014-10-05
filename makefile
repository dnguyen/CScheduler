type = 2
input = input_1

all:
	./out $(type) p1_test/TestInputs/$(input) > output
	diff output p1_test/TestOutputs/$(type)/$(input)

compile: project1.c scheduler.c
	gcc scheduler.c project1.c -lpthread -lrt -lc -lm -g -o out
