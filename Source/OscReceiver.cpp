#include "OscReceiver.h"

#include<chrono>
#include<thread>

#include <iostream> //tmp for testing


OscReceiver::OscReceiver(){receive_task = NULL;}
OscReceiver::OscReceiver(int port, std::function<void(oscpkt::Message* msg, void* arg)> on_receive, void* callbackArg){
    receive_task = NULL;
	setup(port, on_receive, callbackArg);
}
OscReceiver::~OscReceiver(){
	lShouldStop = true;
	// allow in-progress read to complete before destructing
	while(waitingForMessage){
        std::this_thread::sleep_for(std::chrono::milliseconds(OSCRECEIVER_POLL_MS/2));
	}
    if (receive_task != NULL) receive_task->join();
}

void OscReceiver::receive_task_func(){
	OscReceiver* instance = this;
	while(!instance->lShouldStop){
		instance->waitingForMessage = true;
		instance->waitForMessage(0);
		instance->waitingForMessage = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(OSCRECEIVER_POLL_MS));
	}
}

bool OscReceiver::setup(int port, std::function<void(oscpkt::Message* msg, void* arg)> _on_receive, void* callbackArg){

    onReceiveArg = callbackArg;
    on_receive = _on_receive;
    
    socket.bindTo(port);
    if(!socket.isOk()){
        fprintf(stderr, "OscReceiver: Unable to initialise UDP socket: %d %s\n", errno, strerror(errno));
        return false;
    }
    
    std::cout << "Server started, will listen to packets on port " << port << std::endl;
    
    //unique_ptr will kill any already running task when getting a new pointer
    //Have to be explicit before recreating I think.  Or else it gets unhappy.
    if (receive_task != NULL) receive_task->join();
    receive_task = new std::thread(&OscReceiver::receive_task_func, this);

    return true;
}

int OscReceiver::waitForMessage(int timeout){
    int ret = -1;
    if (!socket.isOk()) {
		fprintf(stderr, "OscReceiver: Error polling UDP socket: %d %s\n", errno, strerror(errno));
		return -1;
	} else if (socket.receiveNextPacket(timeout)){
		int msgLength = socket.packetSize();
		if (msgLength < 0) {
			fprintf(stderr, "OscReceiver: Error reading UDP socket: %d %s\n", errno, strerror(errno));
			return -1;
        }
        if (msgLength > OSCRECEIVER_BUFFERSIZE) msgLength = OSCRECEIVER_BUFFERSIZE;
        pr.init(socket.packetData(), socket.packetSize());
        if (!pr.isOk()){
        	fprintf(stderr, "OscReceiver: oscpkt error parsing received message: %i", pr.getErr());
        	return -1;
        }
		on_receive(pr.popMessage(), onReceiveArg);
        ret = msgLength;
	}
	return ret;
}


