HEADERS = 
OBJECTS = Utilities.o

default: Utilities

%.o: %.c $(HEADERS)
	gcc -Wall -Wextra -Werror -c $< -o $@

program: $(OBJECTS)
	gcc $(OBJECTS) -o $@

clean:
	-rm -f $(OBJECTS)
	-rm -f Utilities
