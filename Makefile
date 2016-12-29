default: basic status

CFLAGS = -g -Wall -Werror
BASIC_OBJ = basic.o lcd.o
STATUS_OBJ = status.o lcd.o

%.o: %.c $(HEADERS)
	gcc $(CFLAGS) -c $< -o $@

basic: $(BASIC_OBJ)
	gcc $(BASIC_OBJ) -o $@

status: $(STATUS_OBJ)
	gcc $(STATUS_OBJ) -o $@

clean:
	-rm -f $(BASIC_OBJ) $(STATUS_OBJ)
	-rm -f basic status
