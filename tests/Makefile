
# Copyright SM.

CFLAGS = -g -Wall -Wextra -I ..
WTPLIB = ../WTP.a

all :  wtptest sitest serialtest errtest

wtptest : wtptest.cpp Logger.o $(WTPLIB)
	g++ $(CFLAGS)  wtptest.cpp -o wtptest  Logger.o $(WTPLIB) -lpthread 

sitest : sitest.cpp  Logger.o $(WTPLIB)
	g++ $(CFLAGS)  sitest.cpp -o sitest  Logger.o $(WTPLIB) -lpthread 

serialtest : serialtest.cpp  $(WTPLIB)
	g++ $(CFLAGS)  serialtest.cpp -o serialtest $(WTPLIB) -lpthread 

errtest : errtest.cpp  $(WTPLIB)
	g++ $(CFLAGS)  errtest.cpp -o errtest $(WTPLIB) -lpthread 

Logger.o : Logger.cpp Logger.h
	g++ $(CFLAGS)  -c Logger.cpp

clean:
	rm -f *.o wtptest sitest serialtest errtest
