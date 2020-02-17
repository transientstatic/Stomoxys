#pragma once

#include <memory>
#include <oscpkt.hh>	// neccesary for definition of oscpkt::Message in callback
#include <udp.hh>
#include <thread>       //thread

// forward declarations to speed up compilation
#define OSCRECEIVER_POLL_MS 10
#define OSCRECEIVER_BUFFERSIZE 65536

/**
 * \brief OscReceiver provides functions for receiving OSC messages in Bela.
 *
 * When an OSC message is received over UDP on the port number passed to 
 * OscReceiver::setup() it is passed in the form of an oscpkt::Message
 * to the onreceive callback. This callback, which must be passed to
 * OscReceiver::setup() by the user, is run off the audio thread at 
 * non-realtime priority.
 *
 * For documentation of oscpkt see http://gruntthepeon.free.fr/oscpkt/
 */
class OscReceiver{
    public:
        OscReceiver();
        OscReceiver(int port, std::function<void(oscpkt::Message* msg, void* arg)> on_receive, void* callbackArg = nullptr);
        ~OscReceiver();
	
        /**
		 * \brief Initiliases OscReceiver
		 *
		 * Must be called once during setup()
		 *
		 * @param port the port number used to receive OSC messages
		 * @param onreceive the callback function which recevied OSC messages are passed to
		 * @param callbackArg an argument to pass to the callback
		 *
		 */
        bool setup(int port, std::function<void(oscpkt::Message* msg, void* arg)> _on_receive, void* callbackArg = nullptr);
        
    private:
    	bool lShouldStop = false;
    	
    	volatile bool waitingForMessage = false;
    	int waitForMessage(int timeout_ms);
    	
        oscpkt::UdpSocket socket;

        std::thread* receive_task;
        void receive_task_func();
		
        oscpkt::PacketReader pr;
        char* inBuffer[OSCRECEIVER_BUFFERSIZE];
        
        std::function<void(oscpkt::Message* msg, void* arg)> on_receive;
        void* onReceiveArg = nullptr;
};
