all: curlprog

curlprog: curlprog.m
	clang -arch x86_64 -arch arm64 -fobjc-arc -o curlprog curlprog.m -ldl -framework Foundation

clean:
	rm curlprog