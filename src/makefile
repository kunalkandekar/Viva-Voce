#
# Makes VivaVoce
#

CFLAGS	= -g -Wall -O3
SRC     = VivaVoce.cpp kodek.c UdpRW.cpp SyncQueue.cpp Thread.cpp ResourceManager.cpp AudioRW.cpp 
OBJ     = $(SRC:.cpp=.o)
CC	= gcc

LINK	= -lcurses -lm -lpthread # -lsocket -lnsl -lposix4 
INCLUDE	= -I./ -I/usr/include

#-I/usr/include/sys for Mac OSX

all: clean VivaVoce

SIPTest: cleanSIP
	g++ $(CFLAGS) -o SIPTest SIPTest.cpp SIP.cpp SDP.cpp $(INCLUDE) $(LINK)

cleanSIP:
	rm -f SIPTest

Common.o:
	g++ $(CFLAGS) -o Common.o -c Common.cpp  $(INCLUDE)

SIP.o:
	g++ $(CFLAGS) -o SIP.o -c SIP.cpp  $(INCLUDE)

SDP.o:
	g++ $(CFLAGS) -o SDP.o -c SDP.cpp  $(INCLUDE)

RTP.o:
	g++ $(CFLAGS) -o RTP.o -c RTP.cpp $(INCLUDE)

UserAgent.o:
	g++ $(CFLAGS) -o UserAgent.o -c UserAgent.cpp $(INCLUDE)

Thread.o:
	g++ $(CFLAGS) -o Thread.o -c Thread.cpp $(INCLUDE)

SyncQueue.o:
	g++ $(CFLAGS) -o SyncQueue.o -c SyncQueue.cpp

RTPManager.o:
	g++ $(CFLAGS) -o RTPManager.o -c RTPManager.cpp $(INCLUDE)

ResourceManager.o:
	g++ $(CFLAGS) -o ResourceManager.o -c ResourceManager.cpp $(INCLUDE)

AudioRW.o:
	g++ $(CFLAGS) -o AudioRW.o -c AudioRW.cpp $(INCLUDE)

AudioTest.o:
	g++ $(CFLAGS) -o AudioTest.o -c AudioTest.cpp $(INCLUDE)

AdpcmCodec.o:
	g++ $(CFLAGS) -o AdpcmCodec.o -c AdpcmCodec.cpp	 $(INCLUDE)

UdpRW.o:
	g++ $(CFLAGS) -o UdpRW.o -c UdpRW.cpp $(INCLUDE)

kodek.o: 
	g++ $(CFLAGS) -o kodek.o -c kodek.c $(INCLUDE)

ulaw.o: 
	g++ $(CFLAGS) -o ulaw.o -c ulaw.c $(INCLUDE)

VivaVoce.o:
	g++ $(CFLAGS) -o VivaVoce.o -c VivaVoce.cpp $(INCLUDE)

adpcm.o:
	g++ $(CFLAGS) -o adpcm.o -c adpcm.c  $(INCLUDE)

VivaVoce: Common.o SIP.o SDP.o RTP.o Thread.o UserAgent.o AdpcmCodec.o AudioRW.o AudioTest.o UdpRW.o SyncQueue.o ResourceManager.o kodek.o ulaw.o VivaVoce.o RTPManager.o
	g++ $(CFLAGS) -o VivaVoce VivaVoce.o Common.o SIP.o SDP.o RTP.o kodek.o ulaw.o UserAgent.o AdpcmCodec.o AudioRW.o AudioTest.o UdpRW.o SyncQueue.o Thread.o ResourceManager.o RTPManager.o $(LINK) $(INCLUDE) -L./
	#rm -f *.o 

cleanobj:
	rm -f *.o

clean:
	rm -f *.o VivaVoce
