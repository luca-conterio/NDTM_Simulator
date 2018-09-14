# Turing Machine simulator

Algorithms and Data Structures  
Politecnico di Milano, 2018

A simulator for non-deterministic single tape Turing Machine.
  
The repository contains three versions of the project:
- a recursive version implementing a DFS algorithm through dynamic arrays.
- an iterative version implementing a BFS algorithm, using dynamic arrays to represent Turing Machine's tapes.
- an iterative version implementing a BFS algorithm that uses "chunks" to represent each tape: a tape is divided in chunks, in which is contained a limited number of characters. This allows faster allocation of new blank characters, simulating an infinite tape.  
  
All algorithm also implement a basic machine loop detection.  
