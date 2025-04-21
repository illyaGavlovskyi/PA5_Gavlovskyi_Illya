make: myfs.o
	gcc  -Wall myfs.c -o myfs
clean:
	rm *o myfs