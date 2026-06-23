#ifndef RQ_H
#define RQ_H

#include <queue>
#include <vector> // Added
#include <mutex>
#include "Process.h" 

/*
Abstraction for Global RQ

No per-core RQ

Everything is STATIC to act as a psudo global access
*/

class RQ {
private:
    inline static std::queue<Process> rQueue; 
    inline static std::vector<Process> fQueue; // Changed from queue to vector!
    inline static std::mutex queueMutex; 

public:
    static void addToReady(const Process& p) {
        std::lock_guard<std::mutex> lock(queueMutex);
        rQueue.push(p);
    }

    static void addToFinished(const Process& p) {
        std::lock_guard<std::mutex> lock(queueMutex);
        fQueue.push_back(p); // push_back for vector
    }

    //Outproccess is space for output memory adress
    static bool tryPopReady(Process& outProcess) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (rQueue.empty()) return false; 
        outProcess = rQueue.front();
        rQueue.pop(); 
        return true; 
    }

    // Return a copy of the finished vector so the screen class can read it safely
    static std::vector<Process> getFinishedProcesses() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return fQueue;
    }
};

#endif