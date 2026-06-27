#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <unordered_map>
#include <variant>
#include <sstream>
#include <vector>
#include <iostream>
#include <stdexcept>

// Enum of C primitive data types
enum class PrimitiveType {
    CHAR,
    SIGNED_CHAR,
    UNSIGNED_CHAR,
    SHORT,
    UNSIGNED_SHORT, // Maps directly to uint16
    INT,
    UNSIGNED_INT,
    LONG,
    UNSIGNED_LONG,
    LONG_LONG,
    UNSIGNED_LONG_LONG,
    FLOAT,
    DOUBLE,
    LONG_DOUBLE,
    BOOL
};


// Value container for all supported primitive types
using Value = std::variant<
    char,
    signed char,
    unsigned char,
    short,
    unsigned short,
    int,
    unsigned int,
    long,
    unsigned long,
    long long,
    unsigned long long,
    float,
    double,
    long double,
    bool
>;

// Symbol structure that represents stored variables
// Name is managed as the map key, so it's not needed here
struct Symbol {
    PrimitiveType type;
    Value value;
};


class SymbolTable {
private:
    /* Symbol table using an unordered map where the key is the variable name
    and the value is the Symbol struct containing the type and stored value.*/
    std::unordered_map<std::string, Symbol> table;

    //Turn String input into Enum 
    PrimitiveType stringToType(const std::string& typeStr) {
        if (typeStr == "int") return PrimitiveType::INT;
        if (typeStr == "unsigned int") return PrimitiveType::UNSIGNED_INT;
        if (typeStr == "unsigned short") return PrimitiveType::UNSIGNED_SHORT; // Added mapping for uint16 support
        if (typeStr == "float") return PrimitiveType::FLOAT;
        if (typeStr == "double") return PrimitiveType::DOUBLE;
        if (typeStr == "char") return PrimitiveType::CHAR;
        if (typeStr == "long") return PrimitiveType::LONG;
        if (typeStr == "long long") return PrimitiveType::LONG_LONG;
        if (typeStr == "bool") return PrimitiveType::BOOL;

        throw std::invalid_argument("Unknown type: " + typeStr);
    }

    //Turn String and Enum type into correct Value type
    Value convertValue(PrimitiveType type, const std::string& rawValue) {

        if (type == PrimitiveType::INT)
            return std::stoi(rawValue);

        if (type == PrimitiveType::UNSIGNED_INT) {
            long temp = std::stol(rawValue);
            if (temp < 0)
                throw std::invalid_argument("Negative unsigned int");
            return static_cast<unsigned int>(temp);
        }

        if (type == PrimitiveType::UNSIGNED_SHORT) { // Added handling for uint16 support
            int temp = std::stoi(rawValue);
            if (temp < 0)
                throw std::invalid_argument("Negative unsigned short");
            return static_cast<unsigned short>(temp);
        }

        if (type == PrimitiveType::FLOAT)
            return std::stof(rawValue);

        if (type == PrimitiveType::DOUBLE)
            return std::stod(rawValue);

        if (type == PrimitiveType::CHAR)
            return rawValue[0];

        if (type == PrimitiveType::LONG)
            return std::stol(rawValue);

        if (type == PrimitiveType::LONG_LONG)
            return std::stoll(rawValue);

        if (type == PrimitiveType::BOOL)
            return (rawValue == "true" || rawValue == "1");

        throw std::invalid_argument("Unsupported type conversion");
    }

public:

    //Insert a new Variable based on exact values
    void insert(const std::string& name, PrimitiveType type, const std::string& rawValue) {
        try {
            Value value = convertValue(type, rawValue);
            // FIX: Removed 'name' parameter to match Symbol struct signature
            table[name] = Symbol{type, value};
        }
        catch (const std::exception& e) {
            std::cerr << "Insert error for '" << name << "': " << e.what() << std::endl;
        }
    }

    //Overload: accept Enum as string
    //Insert a new Variable based on 3 separate strings
    //This overload converts string type to enum before inserting into map
    void insert(const std::string& name, const std::string& typeStr, const std::string& rawValue) {
        try {
            PrimitiveType type = stringToType(typeStr);
            Value value = convertValue(type, rawValue);
            // FIX: Removed 'name' parameter to match Symbol struct signature
            table[name] = Symbol{type, value};
        }
        catch (const std::exception& e) {
            std::cerr << "Insert error for " << name << ": " << e.what() << std::endl;
        }
    }

    //Overload: Insert using 1 long String
    //Decomposed the String into name, enum, and value
    void insert(const std::string& declaration) {

        std::istringstream iss(declaration);
        std::vector<std::string> tokens;
        std::string token;

        //Get all String tokens.
        while (iss >> token) {
            tokens.push_back(token);
        }

        // find "=" position
        int eqIndex = -1;
        for (int i = 0; i < (int)tokens.size(); i++) {
            if (tokens[i] == "=") {
                eqIndex = i;
                break;
            }
        }

        //Check for correct formatting
        if (eqIndex == -1 || eqIndex < 2) {
            throw std::invalid_argument("Invalid declaration format");
        }

        // Name is right before "="
        std::string name = tokens[eqIndex - 1];

        // Value is after "="
        std::string valueStr = tokens[eqIndex + 1];

        // Type is everything before name
        std::string typeStr;
        for (int i = 0; i < eqIndex - 1; i++) {
            if (i > 0) typeStr += " ";
            typeStr += tokens[i];
        }

        //Call Overloaded function that accepts 3 string inputs
        insert(name, typeStr, valueStr);
    }

    void update(const std::string& name, const std::string& rawValue) {

        try {
            if (!exists(name)) {
                throw std::invalid_argument("Variable not found");
            }

            PrimitiveType type = table[name].type;
            Value newValue = convertValue(type, rawValue);

            table[name].value = newValue;
        }
        catch (const std::exception& e) {
            std::cerr << "Update error for '" << name << "': " << e.what() << std::endl;
        }
    }

    // Retrieve full symbol entry (Type, and Value) from the symbol table
    Symbol getSymbol(const std::string& name) {

        if (!exists(name)) {
            throw std::invalid_argument("Variable not found: " + name);
        }

        return table.at(name);
    }

    // Retrieve the data type of a stored variable
    PrimitiveType getType(const std::string& name) {

        if (!exists(name)) {
            throw std::invalid_argument("Variable not found: " + name);
        }

        return table.at(name).type;
    }

    // Retrieve the stored value of a variable from the symbol table
    Value getValue(const std::string& name) {

        if (!exists(name)) {
            throw std::invalid_argument("Variable not found: " + name);
        }

        return table.at(name).value;
    }

    // Check if variable exists
    bool exists(const std::string& name) {
        return table.find(name) != table.end();
    }

    // Check if a token matches the C++ variable naming scheme
    static bool isValidIdentifier(const std::string& token) {
        // An empty token is not a valid variable name
        if (token.empty()) {
            return false;
        }

        // Cannot start with a digit
        if (std::isdigit(static_cast<unsigned char>(token[0]))) {
            return false;
        }

        // Every character must be a letter, a digit, or an underscore
        for (char c : token) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
                return false;
            }
        }

        return true;
    }
};

#endif