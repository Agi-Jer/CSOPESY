#include <string>
#include <vector>
#include "SymbolTable.h" 

enum class OpCode {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP
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
};