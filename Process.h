#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <atomic> // Include atomic for thread synchronization
#include "Instruction.h"
#include "SymbolTable.h"
#include "GenerateProcessInstructions.h" // Include factory class

/*
Abstraction for Process

Handles all the necessary load for keeping track of processing time and
running the low level logic for processing the instructions
*/
class Process {
private:
    // New process execution states (Added explicit int backing type for atomic usage compatibility)
    enum class ProcessState : int {
        READY,
        RUNNING,
        WAITING,
        FINISHED
    };

    inline static int globalPidCounter = 0;

    int pid;                    // Unique process ID
    std::string name;           // Process name
    int assignedCore;           // CPU core currently assigned
    std::atomic<bool> finished; // Is this process finished? (Thread-safe status checking)

    int currentInstruction;     // Current instruction index
    int totalInstructions;      // Total instructions in the program

    std::vector<Instruction> instructions; // The actual instructions to be processed

    // TIME TRACKING FIELDS
    std::chrono::system_clock::time_point dateCreated;
    std::chrono::system_clock::time_point dateLastInstruction;

    // RUNTIME ENGINE FIELDS
    SymbolTable sTable;         // Private memory bank for THIS process context
    std::atomic<unsigned int> sleepTimer{0}; // Handles process sleep cycles atomically across threads
    std::atomic<ProcessState> state{ProcessState::READY}; // Current scheduling lifecycle state handled atomically

    //PROCESS SMI
    std::vector<std::string> screenLogBuffer;    //Log Buffer for process-smi

    // DELAYS PER EXEC
    // You know i could have min and max be a static variable initialized on startup as well
    // Oh Well too late
    // Loaded once from config.txt during the "initialize" command
    inline static unsigned int globalDelayPerExec = 0; 
    // Tracks how many more cycles this specific process must busy-wait on a core
    unsigned int currentDelayCounter = 0;

    // Check if String args is number or variable
    // If Variable, return the value in symbol table or 0
    // If it doesn't exist in symbol table but matches naming scheme, declare as 0
    // Otherwise return the token as int literal
    int32_t resolveValue(const std::string& token) {
        // If it's an already tracked variable, pull its current value
        if (sTable.exists(token)) {
            return std::get<unsigned short>(sTable.getValue(token));
        }

        // Use static helper to verify C++ naming rules for implicit declaration
        if (SymbolTable::isValidIdentifier(token)) {
            // Inserts variable "token" as a default uint16 set to 0
            sTable.insert(token, "unsigned short", "0");
            return 0;
        }

        // 3. Otherwise, fall back to parsing it as a raw literal number token
        return std::stoi(token);
    }

    std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) const {
        std::time_t currentTime = std::chrono::system_clock::to_time_t(tp);
        std::tm* localTime = std::localtime(&currentTime);
        if (!localTime) return "(00/00/0000 00:00:00AM)";

        char buffer[40];
        std::strftime(buffer, sizeof(buffer), "(%m/%d/%Y %I:%M:%S%p)", localTime);
        return std::string(buffer);
    }

    //---------------------------
    //Execution
    //---------------------------

    //Wrapper for all the switch cases
    void executeInstruction(const Instruction& current) {
        OpCode op = current.getOpCode();
        const auto& args = current.getArgs();

        switch (op) {
            case OpCode::ADD:       executeAdd(args);      break;
            case OpCode::SUBTRACT:  executeSubtract(args); break;
            case OpCode::SLEEP:     executeSleep(args);    break;
            case OpCode::DECLARE:   executeDeclare(args);  break;
            case OpCode::PRINT:     executePrint(args);    break;
        }
    }

    
    void executeAdd(const std::vector<std::string>& args) {
        //Check if it exists, if not declate the variable
        if (!sTable.exists(args[0])) {
            sTable.insert(args[0], "unsigned short", "0");
        }

        int32_t val2 = resolveValue(args[1]);
        int32_t val3 = resolveValue(args[2]);
        int32_t result = val2 + val3;
        
        if (result > 65535) result = 65535;
        if (result < 0) result = 0;

        sTable.update(args[0], std::to_string(result));
    }

    void executeSubtract(const std::vector<std::string>& args) {
        //Check if it exists, if not declate the variable
        if (!sTable.exists(args[0])) {
            sTable.insert(args[0], "unsigned short", "0");
        }
        int32_t val2 = resolveValue(args[1]);
        int32_t val3 = resolveValue(args[2]);
        int32_t result = val2 - val3;

        if (result > 65535) result = 65535;
        if (result < 0) result = 0;

        sTable.update(args[0], std::to_string(result));
    }

    void executeSleep(const std::vector<std::string>& args) {
        sleepTimer = std::stoi(args[0]);
        state = ProcessState::WAITING; // Automatically switch to WAITING on SLEEP evaluation
    }

    void executeDeclare(const std::vector<std::string>& args) {
        if (sTable.exists(args[0])) {
            sTable.update(args[0], args[1]);
        } else {
            sTable.insert(args[0], "unsigned short", args[1]);
        }
    }

    void executePrint(const std::vector<std::string>& args) {
        std::string rawMessage = args[0];

        // If the blueprint passed a second argument, it's our variable name to append
        if (args.size() > 1) {
            std::string varName = args[1];
            // FIX: Using resolveValue instead of the non-existent function
            rawMessage += std::to_string(resolveValue(varName));
        }

        // Get current timestamp and core details at this exact instruction tick
        std::string timestamp = formatTimestamp(dateLastInstruction);
        std::string coreStr = "Core:" + (assignedCore != -1 ? std::to_string(assignedCore) : "N/A");

        // Format matches mockup: (MM/DD/YYYY hh:mm:ssAM/PM) Core:X "Message"
        std::string formattedLog = timestamp + " " + coreStr + " \"" + rawMessage + "\"";

        // Push right into our screen history buffer!
        screenLogBuffer.push_back(formattedLog);
    }
    
public:
    // Safe default constructor for display slots
    Process() 
        : pid(-1), name("unassigned"), assignedCore(-1), finished(false), 
          currentInstruction(0), totalInstructions(0), state(ProcessState::READY) {
        dateCreated = std::chrono::system_clock::now();
        dateLastInstruction = dateCreated;
    }

    // Standard constructor managing random generation ranges
    Process(std::string processName, int minIns, int maxIns)
        : pid(globalPidCounter++), name(processName), assignedCore(-1), finished(false),
          currentInstruction(0), state(ProcessState::READY) {
        
        // Call the generation offshoot factory once to build the instruction list
        instructions = GenerateProcessInstructions::createProgram(name, minIns, maxIns);

        totalInstructions = static_cast<int>(instructions.size());
        dateCreated = std::chrono::system_clock::now();
        dateLastInstruction = dateCreated;
    }

    // Overloaded constructor that derives the name automatically using the next PID
    Process(int minIns, int maxIns) 
        : Process("process_" + std::to_string(globalPidCounter), minIns, maxIns) {
    }

    // Main function that begin actually running the instructions
    // Call this to execute one line of code
    // Wrapper for execute instruction
    void runCycle() {
        //Check if Process if Asleep or Over 
        if (finished || state == ProcessState::FINISHED || sleepTimer > 0 || state == ProcessState::WAITING) return;
        if (currentInstruction >= totalInstructions) return;
        // New Check if process is being delayed
        if (currentDelayCounter > 0) {
            currentDelayCounter--;
            return; // Burn this CPU cycle doing absolutely nothing
        }

        //Reset State
        dateLastInstruction = std::chrono::system_clock::now();  
        
        // Route the current active line to the execution handler
        executeInstruction(instructions[currentInstruction]);

        currentInstruction++;
        
        // FIX: Verify completion limit threshold bounds immediately before allocating execution delays
        if (currentInstruction >= totalInstructions) {
            finished = true;
            state = ProcessState::FINISHED;
            currentDelayCounter = 0;
        } else {
            currentDelayCounter = globalDelayPerExec; // If globalDelayPerExec is 0, this sets it to 0, running 1 instruction per cycle perfectly.
        }
    }

    void decrementSleep() { 
        if (sleepTimer > 0) {
            sleepTimer--; 
            if (sleepTimer == 0 && state == ProcessState::WAITING) {
                state = ProcessState::READY; // Back to ready queue once sleep expires
            }
        }
    }
    
    bool isSleeping() const { return sleepTimer > 0 || state == ProcessState::WAITING; }

    // State Modifiers
    void setReady()    { if (state != ProcessState::FINISHED) state = ProcessState::READY; }
    void setRunning()  { if (state != ProcessState::FINISHED) state = ProcessState::RUNNING; }
    void setWaiting()  { if (state != ProcessState::FINISHED) state = ProcessState::WAITING; }
    void setFinished() { state = ProcessState::FINISHED; finished = true; }

    // Setters & Getters
    void setAssignedCore(int coreId) { this->assignedCore = coreId; }
    std::string getName() const { return name; }
    int getPid() const { return pid; }
    int getCurrentInstruction() const { return currentInstruction; }
    int getTotalInstructions() const { return totalInstructions; }
    
    // Checks both the legacy boolean and the localized enum state safely
    bool isFinished() const { return finished || state == ProcessState::FINISHED; }
    int getAssignedCore() const { return assignedCore; }

    // Public setter called by the configuration parser during initialization
    static void setGlobalDelayPerExec(unsigned int delay) {
        globalDelayPerExec = delay;
    }

    // Converts enum to human-readable text for visual terminal dumps
    std::string getStateString() const {
        switch (state.load()) {
            case ProcessState::READY:    return "READY";
            case ProcessState::RUNNING:  return "RUNNING";
            case ProcessState::WAITING:  return "WAITING";
            case ProcessState::FINISHED: return "FINISHED";
        }
        return "UNKNOWN";
    }

    const std::vector<std::string>& getScreenLogBuffer() const {
        return screenLogBuffer;
    }

    std::string getCreationTime() const { return formatTimestamp(dateCreated); }
    std::string getLastInstructionTime() const { return formatTimestamp(dateLastInstruction); }
};

#endif