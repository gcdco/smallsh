CXX = gcc
CXXFLAGS = -std=gnu99 -g

ODIR=obj

OBJS = main.o smallsh.o

SRCS = main.c smallsh.c

HEADERS = smallsh.h

smallsh:
	${CXX} ${CXXFLAGS} ${SRCS} -lm -o smallsh
	
.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ smallsh

run:
	./smallsh
	
memcheck:
	valgrind --leak-check=yes --track-origins=yes --show-reachable=yes ./smallsh