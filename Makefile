TARGETS=hw2

hw2: hw2.c
	gcc -g -o hw2 hw2.c -lpthread

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

