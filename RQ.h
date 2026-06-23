#ifndef RQ_H
#define RQ_H

#include <queue>
#include <vector> 
#include <mutex>
#include "Process.h" 

/*
Abstraction for Global RQ

No per-core RQ

Everything is STATIC to act as a psudo global access
*/

class RQ {
private:
    inline static std::queue<int> rQueue;      // Ready Queue (PIDs)
    inline static std::vector<int> runQueue;   // New Running Queue tracking active PIDs on Cores
    inline static std::vector<int> wQueue;     // Waiting Queue tracking blocked/sleeping PIDs
    inline static std::vector<int> fQueue;     // Finished Queue tracking completed PIDs
    inline static std::mutex queueMutex; 

public:
    static void addToReady(int pid) {
        std::lock_guard<std::mutex> lock(queueMutex);
        rQueue.push(pid);
    }

    static void addToFinished(int pid) {
        std::lock_guard<std::mutex> lock(queueMutex);
        fQueue.push_back(pid); 
    }

    static void addToWaiting(int pid) {
        std::lock_guard<std::mutex> lock(queueMutex);
        wQueue.push_back(pid);
    }

    static void addToRunning(int pid) {
        std::lock_guard<std::mutex> lock(queueMutex);
        runQueue.push_back(pid);
    }

    // Outprocess is space for output PID integer
    static bool tryPopReady(int& outPid) {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (rQueue.empty()) return false; 
        outPid = rQueue.front();
        rQueue.pop(); 
        return true; 
    }

    // Return a copy of the running vector so the screen/scheduler can read it safely
    static std::vector<int> getRunningProcesses() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return runQueue;
    }

    // Return a copy of the finished vector so the screen class can read it safely
    static std::vector<int> getFinishedProcesses() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return fQueue;
    }

    // Return a copy of the waiting vector so the screen/scheduler can read it safely
    static std::vector<int> getWaitingProcesses() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return wQueue;
    }

    // Removes a PID from the running queue (called when a process yields, sleeps, or finishes)
    static void removeFromRunning(int pid) {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto it = runQueue.begin(); it != runQueue.end(); ++it) {
            if (*it == pid) {
                runQueue.erase(it);
                break;
            }
        }
    }

    // Removes a PID from the waiting queue (called when a process wakes up)
    static void removeFromWaiting(int pid) {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (auto it = wQueue.begin(); it != wQueue.end(); ++it) {
            if (*it == pid) {
                wQueue.erase(it);
                break;
            }
        }
    }
};

#endif