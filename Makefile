all: curlprog

curlprog: curlprog.c
	gcc -o curlprog curlprog.c -ldl

clean:
	rm curlprog