#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

// Dummy structure for Instruction assumed from your context
struct Instruction {
    std::string commandType;
    std::string argument;    
};

class Process {
private:
    // Static counter initialized to 0 to manage auto-incrementing process IDs
    inline static int globalPidCounter = 0;

    int pid;                    // Unique process ID
    std::string name;           // Process name
    int assignedCore;           // CPU core currently assigned
    bool finished;              // Is this process finished?

    int currentInstruction;     // Current instruction index
    int totalInstructions;      // Total instructions in the program

    std::vector<Instruction> instructions; // The actual instructions to be processed

    // TIME TRACKING FIELDS
    std::chrono::system_clock::time_point dateCreated;
    std::chrono::system_clock::time_point dateLastInstruction;

    // Helper method to get any time point formatted as (MM/DD/YYYY HH:MM:SSAM/PM)
    std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) const {
        std::time_t currentTime = std::chrono::system_clock::to_time_t(tp);
        std::tm* localTime = std::localtime(&currentTime);
        if (!localTime) return "(00/00/0000 00:00:00AM)";

        char buffer[40];
        // Format the core time units: %m = Month, %d = Day, %Y = Year, %I = 12-hour clock, %M = Minute, %S = Second, %p = AM/PM
        std::strftime(buffer, sizeof(buffer), "(%m/%d/%Y %I:%M:%S%p)", localTime);
        return std::string(buffer);
    }

    // Handles the execution log file creation and writing
    void logPrintCommand(const std::string& message) {
        std::string filename = name + ".txt";
        
        // Check if file already exists to decide whether to write the header headers
        std::ifstream checkFile(filename);
        bool isNewFile = !checkFile.good();
        checkFile.close();

        // Open in append mode
        std::ofstream logFile(filename, std::ios::app);
        
        if (logFile.is_open()) {
            if (isNewFile) {
                logFile << "Process name: " << name << "\n";
                logFile << "Logs:\n\n";
            }
            
            // Log matching the exact structure from your screenshot using the instruction time
            logFile << formatTimestamp(dateLastInstruction) << " Core:" << assignedCore 
                    << " \"" << message << "\"\n";
            
            logFile.close();
        } else {
            std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        }
    }

    // Instruction interpreter
    void run(const Instruction& instr) {
        if (instr.commandType == "PRINT") {
            logPrintCommand(instr.argument);
        }
        // Handle other commands here in the future
    }

public:
    // Safe default constructor for creating clean display/extraction containers without ticking the counter
    Process() 
        : pid(-1), name("unassigned"), assignedCore(-1), finished(false), 
          currentInstruction(0), totalInstructions(0) {
        dateCreated = std::chrono::system_clock::now();
        dateLastInstruction = dateCreated;
    }

    // New constructor handling auto-increment PID, targeted string name naming conventions, and automatic vector padding
    Process(std::string commandType, int totalInst = 100) 
        : currentInstruction(0), totalInstructions(totalInst), finished(false), assignedCore(-1) {
        
        // 1. Assign the current static ID index value and increment it for the next process
        this->pid = globalPidCounter++;

        // 2. Set process naming conditionally based on command type string
        if (commandType == "PRINT") {
            this->name = "screen_" + std::to_string(this->pid);
        } else {
            this->name = "process_" + std::to_string(this->pid);
        }

        // 3. Pre-fill the instruction vector with totalInst entries of this commandType
        for (int i = 0; i < totalInstructions; ++i) {
            Instruction instr;
            instr.commandType = commandType;
            // Setup a default print message using its own programmatic process title
            instr.argument = "Hello world from " + this->name + "!";
            this->instructions.push_back(instr);
        }

        // 4. Set the creation baseline timestamps
        dateCreated = std::chrono::system_clock::now();
        dateLastInstruction = dateCreated;
    }

    // Simulation for processing one instruction line
    void executeInstruction() {
        if (!finished) {
            // Update timestamp precisely when the execution begins
            dateLastInstruction = std::chrono::system_clock::now();

            // Execute current instruction
            run(instructions[currentInstruction]);

            currentInstruction++;

            // Check if process is done
            if (currentInstruction >= totalInstructions) {
                finished = true;
            }
        }
    }

    // Setters & Getters
    void setAssignedCore(int coreId) { this->assignedCore = coreId; }
    std::string getName() const { return name; }
    int getPid() const { return pid; }
    int getCurrentInstruction() const { return currentInstruction; }
    int getTotalInstructions() const { return totalInstructions; }
    bool isFinished() const { return finished; }
    int getAssignedCore() const { return assignedCore; }

    // Human-readable string timestamps exposed for your UI Screen class
    std::string getCreationTime() const { 
        return formatTimestamp(dateCreated); 
    }
    
    std::string getLastInstructionTime() const { 
        return formatTimestamp(dateLastInstruction); 
    }
};

#endif