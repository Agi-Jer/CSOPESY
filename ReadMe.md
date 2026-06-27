"""# CSOPESY - OS Simulation: Process Multiplexer and CLI

## Configuration Setup

The simulation reads its environmental constraints from a config.txt file located in the root directory. This configuration dictates the behavior of the hardware threads and the scheduling mechanics.

Configuration Parameters:
* num-cpu: The absolute number of simulated CPU Core instances to initialize. Each core spawns its own autonomous worker thread (std::thread).
* scheduler: The algorithm type used by the queues ("rr" for Round Robin, or "fcfs" for First-Come, First-Served).
* quantum-cycles: The maximum number of consecutive clock cycles a process is allowed to run on a core before preemption under the Round Robin scheduler.
* batch-process-freq: The metered frequency defining exactly how many global clock cycles must pass before the scheduler-test thread spawns and injects a new batch process into the system.
* min-ins: The minimum boundary limit for the randomized number of assembly-like instruction lines assigned to a newly generated process.
* max-ins: The maximum boundary limit for the randomized number of assembly-like instruction lines assigned to a newly generated process.
* delay-per-exec: A simulated delay metric determining how many global clock cycles are required to execute a single atomic instruction line.

Sample config.txt Format:
num-cpu 1
scheduler "rr"
quantum-cycles 20
batch-process-freq 1
min-ins 1000
max-ins 3000
delay-per-exec 0

---

## How to Compile and Run

This project relies heavily on C++ native multi-threading libraries (<thread>, <mutex>, <atomic>, <condition_variable>). It requires a compiler supporting C++11 or higher.

1. Compilation

Using GCC Compiler (Linux / macOS Terminal / Windows MinGW):
Navigate to your project root folder and execute the following command to compile all source files into a unified executable:

g++ -std=c++11 -pthread *.cpp -o csopesy_sim

*Note: The -pthread flag tells the compiler to link the POSIX threads library, which is strictly mandatory on Unix-based systems.*

2. Execution

Once compilation completes successfully, invoke the binary directly from your terminal interface.

On Linux / macOS:
./csopesy_sim

On Windows (Command Prompt / PowerShell):
csopesy_sim.exe
"""
