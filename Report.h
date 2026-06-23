#ifndef REPORT_H
#define REPORT_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include "RQ.h"
#include "ProcessMap.h"
#include "Process.h"

class Report {
private:
    // Globally configured total CPU cores set via "initialize" in main
    inline static int totalCores = 0;

    // Internal helper to build the unified ASCII snapshot string block
    static std::string buildSnapshotString() {
        std::vector<int> runningPids = RQ::getRunningProcesses();
        std::vector<int> finishedPids = RQ::getFinishedProcesses();

        int coresUsed = static_cast<int>(runningPids.size());
        if (coresUsed > totalCores) coresUsed = totalCores; // Upper bound guard
        int coresAvailable = totalCores - coresUsed;

        // Calculate utilization: (Cores Used / Total Cores) * 100
        int cpuUtilization = (totalCores > 0) ? static_cast<int>((static_cast<double>(coresUsed) / totalCores) * 100) : 0;

        std::string output = "";
        output += "CPU utilization: " + std::to_string(cpuUtilization) + "%\n";
        output += "Cores used: " + std::to_string(coresUsed) + "\n";
        output += "Cores available: " + std::to_string(coresAvailable) + "\n";
        output += "-------------------------------------------------------\n\n";

        // 1. Running Processes Segment
        output += "Running processes:\n";
        for (int pid : runningPids) {
            Process* proc = ProcessMap::getProcess(pid);
            if (proc != nullptr) {
                // Formatting tabs to make properties columns match line 44 mockup layout
                output += proc->getName() + "\t" 
                       + proc->getCreationTime() + "\t"
                       + "Core: " + std::to_string(proc->getAssignedCore()) + "\t"
                       + std::to_string(proc->getCurrentInstruction()) + " / " 
                       + std::to_string(proc->getTotalInstructions()) + "\n";
            }
        }
        if (runningPids.empty()) output += "(No active running processes)\n";

        output += "\nFinished processes:\n";
        // 2. Finished Processes Segment
        for (int pid : finishedPids) {
            Process* proc = ProcessMap::getProcess(pid);
            if (proc != nullptr) {
                output += proc->getName() + "\t" 
                       + proc->getCreationTime() + "\t"
                       + "Finished\t"
                       + std::to_string(proc->getCurrentInstruction()) + " / " 
                       + std::to_string(proc->getTotalInstructions()) + "\n";
            }
        }
        if (finishedPids.empty()) output += "(No finished processes)\n";
        
        output += "-------------------------------------------------------\n";
        return output;
    }

public:
    // Pure static implementation
    Report() = delete;

    // Setter invoked during the "initialize" step to cache total core num
    static void setTotalCores(int cores) {
        if (cores > 0) totalCores = cores;
    }

    // Handles the live display for the "screen -ls" command workflow
    static void printScreenList() {
        std::cout << buildSnapshotString();
    }

    // Handles the file export system for the "report-util" command workflow
    static bool generateUtilReport() {
        std::ofstream logFile("csopesy-log.txt", std::ios::out | std::ios::trunc);
        if (!logFile.is_open()) {
            return false;
        }

        logFile << buildSnapshotString();
        logFile.close();
        return true;
    }
};

#endif