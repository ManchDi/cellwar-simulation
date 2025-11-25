# Multithreaded Cell-War Simulation

## Description
This is an interactive **multithreaded simulation** of a battle between two teams of soldiers on a grid.  
Each soldier acts in a separate thread, shooting at random positions and updating the map in real-time.  
The simulation demonstrates multithreading, atomic operations, and concurrency control in C++.

## Features
- Multi-threaded soldiers using **POSIX threads (`pthread`)**
- Real-time grid updates with a **majority rule** for neighboring cells
- Logging of all actions with timestamps
- Configurable grid size and number of soldiers for each team
- Simple command-line interface

## Technologies
- C++
- POSIX Threads (`pthread`)
- C++ Standard Library (`iostream`, `fstream`, `atomic`, `unordered_map`, `chrono`, `random`)

## Usage
# Compile
g++ -std=c++17 -pthread -o cellwar main.cpp

# Run
./cellwar <team1_soldiers> <team2_soldiers> <rows> <columns>

# Example
./cellwar 5 5 10 10
