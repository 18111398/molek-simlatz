### TODO
- Full collision detection
- Output checking (Currently it will just stop when the 120th output is received, no matter if they are correct or not)
- Implement puzzles
- Validate precursors when loading solution (available in puzzle, overlap, out of bounds)
- A lot of other stuff

### Building
Example with gcc:
```
g++ main.cpp sim.cpp -O2 -s -std=c++23 -o molek-simlatz
```

Use this for a stepping debugger, which prints all atoms to the console every cycle (cannot print bonds, I wish to know how to)

```
g++ main_trace.cpp sim.cpp -O2 -s -std=c++23 -o molek-traz
```

You can try with any compiler and any flags you like.

### Usage (currently)
```molek-simlatz [file path]```
