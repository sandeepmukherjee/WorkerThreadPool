
CFLAGS = -g -I ..
WTPLIB = ../WTP.a -lpthread

all : treebuild

treebuild : treebuild.o $(WTPLIB)
	g++ $(CFLAGS) -o treebuild treebuild.o $(WTPLIB)

treebuild.o : treebuild.cpp 
	g++ -c $(CFLAGS) treebuild.cpp

