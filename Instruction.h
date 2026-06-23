#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <vector>
#include "SymbolTable.h" 

enum class OpCode {
    ADD,
    SUBTRACT,
    SLEEP,
    DECLARE,
    PRINT,
    FOR       
};

/*
This is a wrapper for what each Instruction Line contains
*/
class Instruction {
private:
    OpCode op;
    std::vector<std::string> args; // Pre-parsed tokens (e.g., {"var1", "var2", "5"})

public:
    Instruction(OpCode operation, std::vector<std::string> arguments) 
        : op(operation), args(std::move(arguments)) {}

    // Public getters to allow Process.h and execution engines to read the internals
    OpCode getOpCode() const { 
        return op; 
    }

    const std::vector<std::string>& getArgs() const { 
        return args; 
    }
};

#endif