# Turing Machine Simulator

Algorithms and Data Structures - Politecnico di Milano, 2017/2018

## Overview
A simulator for non-deterministic single tape acceptor Turing Machine.
  
The repository contains three versions of the project: 
- a recursive version implementing a DFS algorithm through dynamic arrays that uses a stack structure to store machine tapes.  
- an iterative version implementing a BFS algorithm, using dynamic arrays to represent Turing Machine tapes. It uses a queue implemented through a linked list to store possible transitions of the machine's computation tree at each machine step.  
- an iterative version implementing a BFS algorithm that uses "chunks" to represent each tape: a tape is divided in chunks, in which is contained a limited number of characters. The queue of possible transitions is implemented as a queue through a static array, that avoids calls to malloc function, saving a great amount of time during computation.

To simulate the infinite machine tape, the third solution allows faster allocation of new blank characters, when the machine head tries to go to the left/right of the input string. In the first and second solutions it is necessary to reallocate the whole tape every time, adding new blank characters.
  
A basic machine loop detection is also implemented in all the three algorithms.  

## Conventions
- tape symbols are chars, while states are integers.
- the char '_' indicates the blank character.
- the machine always starts from state 0 and from the first character on the input string.
- if state N exists, all states from 0 to N-1 exist too.
- acceptation states doesn't have any outgoing transition.
- the tape has infinite length in both left and right directions and contains the blank character in every position.
- characters 'R', 'L' and 'S' are used for machine head's movements.

## Input
The input that the simulator needs has 4 parts:
1. the first part, preceded by "tr", contains the list of transitions, one on each line (es: 0 a b R 1 indicates that this transition starts from state 0 and goes to state 2 reading 'a' on the tape, writing 'b' and moving the head on the right). 
2. the next part, starting with "acc", contains the list of acceptation states, one on each line.
3. to avoid infinite computations, after the word "max", a maximum number of machine steps is given.
4. finally, after the word "run" there's the list of strings to be computed, separated by a new line.

## Output
The simulator has three possible outputs:
- if the machine reaches an acceptance state, the output for that string will be '1'.
- the machine will give a '0' as a result if the computation ends without reaching an acceptation state.
- 'U' will be the result if the computation exceeds the maximun number of steps without reaching an acceptation state. 

## Example
Here is given an example of Turing Machine that accepts strings composed by a substring and the same substring written from the last letter to the first one, for example "abaaba":  
```  
tr  
0 a a R 0  
0 b b R 0  
0 a c R 1  
0 b c R 2  
1 a c L 3  
2 b c L 3  
3 c c L 3  
3 a c R 4  
3 b c R 5  
4 c c R 4  
4 a c L 3  
5 c c R 5  
5 b c L 3  
3 _ _ R 6  
6 c c R 6  
6 _ _ S 7  
acc  
7  
max  
800  
run  
aababbabaa  
aababbabaaaababbabaa  
aababbabaaaababbabaab  
aababbabaaaababbabaabbaababbabaaaababbabaa  
aababbabbaaababbabaabbaababbabaaaababbabaa  
```
The output will be:
```
1  
1  
0  
U  
0
```
