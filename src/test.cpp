#include <stdio.h>
#include "UserAgent.h"

//g++ -o uatest test.cpp UserAgent.o SIP.o SDP.o SyncQueue.o Thread.o ResourceManager.o UdpRW.o RTPManager.o RTP.o Common.o

int main(void) {
    SDPPacket *sdpPackSend = new SDPPacket();
    delete sdpPackSend; 
    //UserAgent *ua = new UserAgent(NULL);
    //ua->setThreadsToSignal(NULL);
    //#delete ua;
    printf("done\n");
}
