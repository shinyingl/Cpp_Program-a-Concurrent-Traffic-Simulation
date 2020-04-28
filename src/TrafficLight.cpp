#include <iostream>
#include <random>
#include <queue>
#include <chrono>
#include <thread>
#include <future>
#include <algorithm>

#include "TrafficLight.h"
/* Implementation of class "MessageQueue" */


template <class T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    
    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _condition.wait(uLock, [this] { return !_msg_queue.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_msg_queue.back());
    _msg_queue.pop_back();

    return msg; // will not be copied due to return value optimization (RVO) in C++
}

template <class T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
  
    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    // std::cout << "   Message " << msg << " has been sent to the queue" << std::endl;
    _msg_queue.push_back(std::move(msg));
    // _msg_queue.push_back(msg);
    _condition.notify_one(); // notify client after pushing new msg
}


/* Implementation of class "TrafficLight" */

 
TrafficLight::TrafficLight()
{
    _current_phase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto msg = _msg_queue.receive();
        if(msg == green){
            return;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _current_phase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this)); 
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(4000, 6000);
    int cycle_duration = dis(gen); 
    auto last_update = std::chrono::system_clock::now();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto dt_update = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last_update);
        int dt = dt_update.count();
        if(dt >= cycle_duration){
            _current_phase = _current_phase == red ? green : red;

            auto future_sent = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, &_msg_queue, std::move(_current_phase));
	        future_sent.wait();
        
            cycle_duration = dis(gen); 
            last_update = std::chrono::system_clock::now();
        } //if

    } //while
}

