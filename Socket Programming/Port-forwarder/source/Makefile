OUTPUT 	= server# executable
DEP		= ./lib/port_forward.c ./lib/connect_setup.c

$(OUTPUT):
	gcc -Wall $(OUTPUT).c -o $(OUTPUT) $(DEP) -fopenmp

all: clean $(OUTPUT) run

run:
	./$(OUTPUT)

clean:
	rm -f $(OUTPUT)