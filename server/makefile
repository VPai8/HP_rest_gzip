INC=-I/home/pai/cpprestsdk/Release/include/cpprest
fin : Main.o MyServer.o stdafx.o
	g++ -o fin Main.o MyServer.o stdafx.o -lboost_system -lcrypto -lssl -lcpprest -pthread
Main.o : Main.cpp stdafx.h MyServer.h
	g++ -c Main.cpp
MyServer.o : MyServer.cpp stdafx.h messagetypes.h MyServer.h
	g++ -c MyServer.cpp
stdafx.o : stdafx.cpp stdafx.h
	g++ -c stdafx.cpp
