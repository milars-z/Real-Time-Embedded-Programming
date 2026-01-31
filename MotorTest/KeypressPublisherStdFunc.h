#ifndef __KEYPRESS_PUBLISHER_STD_FUNC
#define __KEYPRESS_PUBLISHER_STD_FUNC

/**
 * GNU GENERAL PUBLIC LICENSE
 * Version 3, 29 June 2007
 *
 * (C) 2020-2025, Bernd Porr <mail@bernporr.me.uk>
 * 
 **/

/**
 * ServoEngine - raspi5 ServoEngine
 * 
 * basic reference : berndporr
 * rebuild for raspi5 : Ziyin Zeng
 * License ：MIT
 * Time : 01,30,2026
 * 
 **/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <thread>
#include <functional>

#include <termios.h>

class KeypressPublisherStdFunc
{

public:

    using CallbackFunction = std::function<void(int)>;
    
    virtual void start() {
	if (uthread.joinable()) uthread.join();
	if (running) return;
    
    // add running setting
    running = true;

	uthread = std::thread(&KeypressPublisherStdFunc::worker,this);
    }

    virtual void stop() {
	running = false;
	if (uthread.joinable()) uthread.join();
    }

    virtual ~KeypressPublisherStdFunc() {
	stop();
    }

    //using CallbackFunction = std::function<void(void)>;

    void registerEventCallback(CallbackFunction cf) {
        callbackFunction = cf;
    }
    
private:
    CallbackFunction callbackFunction;
    bool running = false;
    std::thread uthread;
    void worker() {

    // set mode no need for ENTER
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	while (running) {
	    // we are blocking here
	    //getchar();
        // get a command from user
        int ch = getchar();

	    // callback
	    if (callbackFunction && running)
		callbackFunction(ch);
	}
    // set back
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
    
};

#endif
