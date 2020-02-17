/***** OscSender.cpp *****/
#include "OscSender.h"
#include <iostream>

#define OSCSENDER_MAX_ARGS 1024
#define OSCSENDER_MAX_BYTES 65536

OscSender::OscSender(){}
OscSender::OscSender(int port, std::string ip_address){
	setup(port, ip_address);
}
OscSender::~OscSender(){}

void OscSender::send_task_func(char * buf, int size){
//    std::cout << buf << " Size: " << size << "\n";
	socket.sendPacket(buf, size);
}

void OscSender::setup(int port, std::string ip_address){
    
    socket.connectTo(ip_address, port);
    if (!socket.isOk()){
        fprintf(stderr, "OscSender: Unable to connect to UDP socket: %d %s\n", errno, strerror(errno));
        return;
    }
    
    msg = std::unique_ptr<oscpkt::Message>(new oscpkt::Message());
//    msg->reserve(OSCSENDER_MAX_ARGS, OSCSENDER_MAX_BYTES);
//  This is a Bela special I suspect to avoid allocations during runtime
	
}

OscSender &OscSender::newMessage(std::string address){
	msg->init(address);
	return *this;
}

OscSender &OscSender::add(int payload){
	msg->pushInt32(payload);
	return *this;
}
OscSender &OscSender::add(float payload){
	msg->pushFloat(payload);
	return *this;
}
OscSender &OscSender::add(std::string payload){
	msg->pushStr(payload);
	return *this;
}
OscSender &OscSender::add(bool payload){
	msg->pushBool(payload);
	return *this;
}
OscSender &OscSender::add(void *ptr, size_t num_bytes){
	msg->pushBlob(ptr, num_bytes);
	return *this;
}

void OscSender::send(){
//    int iping = 1;
//    oscpkt::Message ping("/ping");
//    ping.pushInt32(iping);
//    pw.startBundle().startBundle().addMessage(ping).endBundle().endBundle();

    if (enabled){
    
        pw.init().startBundle().addMessage(*msg).endBundle();

    //    std::cout << "Pack: " << pw.packetData() << " Size: " << pw.packetSize() << std::endl;
        bool ok = socket.sendPacket(pw.packetData(), pw.packetSize());
        
        //    std::thread(&OscSender::send_task_func, this, pw.packetData(), pw.packetSize()).detach(); //launch a send task and let it go...

    } else {
        msg->clear();
    }
    
}

