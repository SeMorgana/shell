all: kinda-sh

kinda-sh: kinda-sh.o linkedList.o tokenizer.o
	gcc kinda-sh.o linkedList.o tokenizer.o -o kinda-sh

kinda-sh.o: kinda-sh.c
	gcc -c kinda-sh.c -o kinda-sh.o

tokenizer.o: tokenizer.c
	gcc -c tokenizer.c -o tokenizer.o

linkedList.o: linkedList.c
	gcc -c linkedList.c -o linkedList.o

clean:
	rm *.o kinda-sh
