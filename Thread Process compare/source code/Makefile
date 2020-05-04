LIBS=-lgmp -pthread
p_main: p_main.c ./lib/primedecompose.c
	gcc -Wall -o ./exec/p_main p_main.c ./lib/primedecompose.c $(LIBS)

t_main: t_main.c ./lib/primedecompose.c
	gcc -Wall -o ./exec/t_main t_main.c ./lib/primedecompose.c $(LIBS)