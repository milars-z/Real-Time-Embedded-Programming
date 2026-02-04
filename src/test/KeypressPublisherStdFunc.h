#ifndef __KEYPRESS_PUBLISHER_STD_FUNC
#define __KEYPRESS_PUBLISHER_STD_FUNC

/**
 * GNU GENERAL PUBLIC LICENSE
 * Version 3, 29 June 2007
 *
 * (C) 2020-2025, Bernd Porr <mail@bernporr.me.uk>
 * 
 **/

#include <stdlib.h>
//#include <unistd.h>
#include <stdio.h>
#include <thread>
#include <functional>

class KeypressPublisherStdFunc
{

public:
    virtual void start() {
	if (uthread.joinable()) uthread.join();
	if (running) return;
	uthread = std::thread(&KeypressPublisherStdFunc::worker,this);
    }

    virtual void stop() {
	running = false;
	if (uthread.joinable()) uthread.join();
    }

    virtual ~KeypressPublisherStdFunc() {
	stop();
    }

    using CallbackFunction = std::function<void(void)>;

    void registerEventCallback(CallbackFunction cf) {
        callbackFunction = cf;
    }
    
private:
    CallbackFunction callbackFunction;
    bool running = false;
    std::thread uthread;
    void worker() {
	running = true;
	while (running) {
	    // we are blocking here
	    getchar();
	    // callback
	    if (callbackFunction)
		callbackFunction();
	}
    }
};

#endif
