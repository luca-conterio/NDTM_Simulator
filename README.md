# Turing Machine Simulator

#### Algorithms and Data Structures - Politecnico di Milano, 2017/2018

A simulator for non-deterministic single tape Turing Machine.
  
The repository contains three versions of the project:
- a recursive version implementing a DFS algorithm through dynamic arrays.
- an iterative version implementing a BFS algorithm, using dynamic arrays to represent Turing Machine's tapes.
- an iterative version implementing a BFS algorithm that uses "chunks" to represent each tape: a tape is divided in chunks, in which is contained a limited number of characters.  

To simulate the infinite machine tape, the third solution allows faster allocation of new blank characters, when the machine head tries to go to the left/right of the input string. In the first and second solutions it is necessary to reallocate the whole tape every time, adding new blank characters.
  
A basic machine loop detection is also implemented in all the three algorithms.  
