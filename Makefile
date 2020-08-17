cc=g++
MAKE=make
RM=rm
INTERFACE=-I Interface/inc -L Interface/lib -static
SERVER=ps u | grep "[s]erver" | tr -s ' ' | cut -d ' ' -f2
TERMINAL=konsole #change to your terminal

#targets .
all: Client/main.cpp Server/main.cpp
	kill -9 $$(ps u | grep "[s]erver" | tr -s ' ' | cut -d ' ' -f2) || true
	kill -9 $$(ps u | grep "[c]lient" | tr -s ' ' | cut -d ' ' -f2) || true

	$(cc) -I ./Interface/inc -c Interface/src/interface.cpp -o Interface/obj/interface.o
	ar rcs Interface/lib/libinterface.a Interface/obj/interface.o

	$(cc) $(INTERFACE) Client/main.cpp -linterface -o client -lssl -lcrypto
	$(cc) Server/main.cpp -o server -lssl -lcrypto
	$(TERMINAL) -e "./server" &
	sleep 1
	$(MAKE) client_target

#another target for client
client_target:
	$(TERMINAL) -e "./client" &


clean: server client
	$(RM) server
	$(RM) client
	$(RM) Interface/lib/libinterface.a Interface/obj/interface.o
