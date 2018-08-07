//
//  main.c
//  ndtm
//
//  Created by Luca Conterio on 25/05/18.
//  Copyright © 2018 Luca Conterio. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ACCEPT                 '1'
#define REJECT                 '0'
#define UNDEFINED	           'U'
#define RIGHT                   1
#define LEFT  			       -1
#define STOP  			        0
#define BLANK 			       '_'
#define DEFAULT_INPUT_DIM       200
#define INPUT_INCREMENT		    50
#define DEFAULT_STATES_DIM      25
#define STATES_INCREMENT        5
#define TAPE_CHUNK_LENGTH	    15+1
#define DEBUG 					0

typedef enum {true, false} bool;

typedef struct graph_node {       // node of the graph
	char in;
	char out;
	int move;
	int next_state;
	struct graph_node * next;     // list of all possible next states of the current one
} graph_node;

typedef struct tape_chunk {
	char * string;
	struct tape_chunk * right;
	struct tape_chunk * left;
} tape_chunk;

typedef struct stack_node {
  tape_chunk * tape;
  tape_chunk * currChunk;
  struct stack_node * next;
} stack_node;

typedef struct acc_state {
	int value;
	struct acc_state * next;
} acc_state;

void initGraph();
void readMTStructure();
void addAcceptationState(int);
void insertNodeInGraph(int, char, char, int, int);
void readInputStrings();
void addTapeChunk(char *);
void run();
void executeTM(int, int, int);
void performTransition(int *, int *, int *);
void performNonDeterministicTransition(int, int, int);
tape_chunk * copyCurrentTape(tape_chunk **);
void putInStack(tape_chunk *, tape_chunk *);
void popFromStack();
int updateIndex(int);
void prependNewTapeChunk(tape_chunk *);
void appendNewTapeChunk(tape_chunk *);
tape_chunk * createNewChunk();
int countAccessibleTransitions(int, char);
bool checkIfAccState(int);
char checkComputationResult();
void printGraph();
void printTape();
void printStack();
void freeGraph();
void freeStack();
void freeTape(tape_chunk *);
void freeAccStatesList();

//**********************************************************************************************************

int states_num = 0;						// the number of states of the TM
int states_dim = DEFAULT_STATES_DIM;    // the actual size of states array (graph)
graph_node ** graph;				    // array containing all the states read from input

stack_node * stack = NULL;              // the stack used to handle non-deterministic moves

tape_chunk * tape = NULL;			    // the tape of the Turing Machine
tape_chunk * tape_tail = NULL;          // the last chunk of the Turing Machine tape

acc_state * accStates = NULL;	     	// acceptation states list
int startingState = 0;                  // the starting state of the Turing Machine
int reject_state = -1;    				// the state of rejection
int iterationsLimit;             		// the limit to the iteration number (to avoid machine loop)

bool acceptString = false;              // true when a path accepts the input string
bool atLeastAnUndefinedPath = false;    // true when at least a path returns UNDEFINED (iteration > iterationsLimit)

/***************************************************************
 * Initializes graph (states vector) elements to NULL
 ***************************************************************/
void initGraph() {
	for(int i = 0; i < DEFAULT_STATES_DIM; i++)
		graph[i] = NULL;
}

/*******************************************************************
 * Reads the stind (MT structure) saving data in program structures
 *******************************************************************/
void readMTStructure() {
	int s = -1;
	char inputChar = ' ';
	char outputChar = ' ';
	int move = 2;
	int next_s = -1;
	char in[5];

	scanf("%s", in);
	if (strcmp(in, "tr") != 0)
		exit(0);

	scanf("%s", in);
	while (strcmp(in, "acc") != 0) { 	// cycle until the word "acc"
		sscanf(in, "%d", &s); 		 	// read state
		scanf("%s", in);
		inputChar = in[0];    		 	// read input character
		scanf("%s", in);
		outputChar = in[0];   		 	// read output character
		scanf("%s", in);
		switch(in[0]) {					// read move
			case 'R': move = RIGHT;
					  break;
			case 'L': move = LEFT;
					  break;
			case 'S': move = STOP;
					  break;
			default:  move = STOP;
					  break;
		}

		scanf("%s", in);
		sscanf(in, "%d", &next_s);   					// read next state
		if (s >= states_num || next_s >= states_num) {	 	// maintain the maximum state number
			if (s >= next_s)
				states_num = s + 1;
			else
				states_num = next_s + 1;
		}
		if (states_num >= states_dim) {
			graph = (graph_node **) realloc(graph, (states_num + STATES_INCREMENT) * sizeof(graph_node *));
			// re-initialize states vector (graph) with NULL in new positions
			for (int i = states_dim; i < states_num + STATES_INCREMENT; i++)
				graph[i] = NULL;
			states_dim = states_num + STATES_INCREMENT;
			//printf("\nRIALLOCO VETTORE STATI (dim = %d)\n\n", states_dim);
		}
		insertNodeInGraph(s, inputChar, outputChar, move, next_s);
		scanf("%s", in);
	}

	int accState;
	scanf("%s", in);
	while (strcmp(in, "max") != 0) { 	// read acceptation states
		sscanf(in, "%d", &accState);
		addAcceptationState(accState);
		scanf("%s", in);
	}

	scanf("%d", &iterationsLimit);    // read maximum number of iterations

	scanf("%s", in);                  // read the word "run" to start computations
	if (strcmp(in, "run") != 0)
		exit(0);
	else
		getchar(); // consume '\n' character after "run" string
}

/******************************************************************
 * Inserts the graph_node read from stdin in the graph_nodes graph
 ******************************************************************/

void insertNodeInGraph(int s, char in, char out, int m, int n_s) {

	graph_node * new = (graph_node *) malloc(sizeof(graph_node));
	if (new != NULL) {
		new->next = NULL;
		new->in = in;
		new->out = out;
		new->move = m;
		new->next_state = n_s;
	}
	else {
		printf("Error: not enough memory...");
		exit(0);
	}

	if (graph[s] == NULL) {   // insert when list is empty
		graph[s] = new;
		new->next = NULL;
		return;
	}

	if (new->in <= graph[s]->in) { // insert as first element
		new->next = graph[s];
		graph[s] = new;
		return;
	}

	graph_node *curr = graph[s];          // insert in order (includes insertion as last)
	graph_node *succ = curr->next;
	while (succ != NULL && new->in > succ->in) {
		curr = succ;
		succ = succ->next;
	}

	new->next = succ;
	curr->next = new;
}

/*******************************************************************
 * Insert a new integer in accStates list (acceptation states list)
 *******************************************************************/
 void addAcceptationState(int s) {
	 acc_state * new = (acc_state *) malloc(sizeof(acc_state));
	 if (new != NULL) {
		 new->value = s;
		 new->next = NULL;
	 }
	 else {
		 printf("Error: not enough memory...");
		 exit(0);
	 }
	 new->next = accStates;
	 accStates = new;
 }

/****************************************************************
 * Reads the next input string from stdin
 ****************************************************************/
void readInputStrings() {

	char inputString[TAPE_CHUNK_LENGTH]; // current input string

	int i = 0;
	char c = ' ';

	while (c != EOF) {
		c = getchar();
		if (i == TAPE_CHUNK_LENGTH-1 && c != EOF && c != '\n') {
			inputString[i] = '\0';
            addTapeChunk(inputString);
			i = 0;
		}

		if (i != 0 && (c == '\n' || (c == EOF && inputString[i-1] != '\0'))) {
			inputString[i] = '\0';
			if (strlen(inputString) != 0)
				addTapeChunk(inputString);
			run();
            tape = NULL;
            tape_tail = NULL;
			i = 0;
		}
		else if (c != EOF && c != '\n') {
			inputString[i] = c;
			i++;
		}
	}
}

//******************************************************************
void addTapeChunk(char * string) {
	tape_chunk * new = (tape_chunk *) malloc(sizeof(tape_chunk));
	new->string = (char *) malloc(TAPE_CHUNK_LENGTH * sizeof(char));

	if (new == NULL) {
		printf("Error: not enough memory...");
		exit(0);
	}

	if (tape == NULL)
		tape = new;

	if (strlen(string) < TAPE_CHUNK_LENGTH) { // add BLANK characters to fill the chunk char array
		for (int i = strlen(string); i < TAPE_CHUNK_LENGTH; i++) {
			string[i] = BLANK;
		}
		string[TAPE_CHUNK_LENGTH-1] = '\0';
	}

	strcpy(new->string, string);
	new->left = tape_tail;

	if (tape_tail != NULL)
		tape_tail->right = new;

	new->right = NULL;
	tape_tail = new;
}

//***************************************
void run() {
    //initTape();
    putInStack(tape, tape);
    acceptString = false;
    atLeastAnUndefinedPath = false;
    executeTM(0, startingState, 1);
    printf("%c\n", checkComputationResult());
    freeStack();
}

//***************************************
void executeTM(int index, int currState, int iteration) {
    if (acceptString == true)
        return;

    tape_chunk * currChunk = stack->currChunk;

    index = updateIndex(index);
	
    int accessibleTransitions = countAccessibleTransitions(currState, currChunk->string[index]);

    while (acceptString == false && iteration <= iterationsLimit && accessibleTransitions != 0) {

		if (DEBUG) {
			//sleep(1);
			printf("\n###############################\n");
			printf("current state: %d\n", currState);
			printf("accessible transitions: %d\n", accessibleTransitions);
			printf("iteration: %d\n", iteration);
		}

		if (accessibleTransitions >= 2) {
			performNonDeterministicTransition(currState, index, iteration);
			return;
		}
		else {
			performTransition(&currState, &index, &iteration);
		}

		accessibleTransitions = countAccessibleTransitions(currState, currChunk->string[index]);
	}

	if (checkIfAccState(currState) == true) { // accept string
		acceptString = true;
		if (DEBUG) printf("ACCEPT\n");
		return;
	}
	else if (iteration > iterationsLimit) {  // undefined
		atLeastAnUndefinedPath = true;
		if (DEBUG) printf("UNDEFINED\n");
		return;
	}
	else if (accessibleTransitions == 0) {  // reject string
		if (DEBUG) printf("REJECT\n");
		return;
	}
}

//***************************************
void performTransition(int * state, int * i, int * it) {
	graph_node * p = graph[*state];
    tape_chunk * currChunk = stack->currChunk;
	bool found = false;
	while (p != NULL && found == false) {
		if (p->in == currChunk->string[*i]) {
			if (DEBUG) printf("----------   Deterministic Transition   ----------\n");
			found = true;
			currChunk->string[*i] = p->out;
			if (DEBUG) printf("index: %d\n", *i);
		    *i = *i + p->move;
            *i = updateIndex(*i);
			if (DEBUG) printf("new index: %d\n", *i);
			*it = *it + 1;
			if (DEBUG) printf("iteration: %d\n", *it);
			*state = p->next_state;
			if (DEBUG) printf("new state: %d\n", *state);
		}
		p = p->next;
	}
}

//****************************************
void performNonDeterministicTransition(int state, int i, int it) {
	graph_node * p = graph[state];
    tape_chunk * currChunk = stack->currChunk;
	while (p != NULL) {
		if (p->in == currChunk->string[i]) {
			if (DEBUG) printf("**********   NON-DETERMINISTIC TRANSITION   **********\n");
			if (DEBUG) {
				printf("index: %d\n", i);
				printf("new index: %d\n", i+p->move);
				printf("iteration: %d\n", it+1);
				printf("new state: %d\n", p->next_state);
			}

            tape_chunk * newCurrChunk;
            tape_chunk * newTape = copyCurrentTape(&newCurrChunk);

			if (DEBUG) { printf("\ncurrent chunk: %s\n", currChunk->string); }
			//if (DEBUG) { printf("\nnew current chunk: %s\n", newCurrChunk->string); }
			if (DEBUG) { printf("new tape:\n"); printTape(newTape); }

			newCurrChunk->string[i] = p->out;

			putInStack(newTape, newCurrChunk);
			executeTM(i+p->move, p->next_state, it+1);
			popFromStack();
		}
		p = p->next;
	}
}

//***************************************
tape_chunk * copyCurrentTape(tape_chunk ** newCurrChunk) {
    tape_chunk * p = stack->tape;
    tape_chunk * currChunk = stack->currChunk;
    tape_chunk * newTape = NULL;
    tape_chunk * newTapeTail = NULL;
    tape_chunk * newChunk = NULL;

    while (p != NULL) {
        newChunk = (tape_chunk *) malloc(sizeof(tape_chunk));
        newChunk->string = (char *) malloc(TAPE_CHUNK_LENGTH * sizeof(char));

        if (newTape == NULL)
    		newTape = newChunk;

    	strcpy(newChunk->string, p->string);
    	newChunk->left = newTapeTail;

		if (newTapeTail != NULL)
    		newTapeTail->right = newChunk;

    	newChunk->right = NULL;
    	newTapeTail = newChunk;

        if (p == currChunk) {
            *newCurrChunk = newChunk;
			if (DEBUG) printf("\nLOL");
		}

        p = p->right;
    }

    return newTape;
}

//***************************************
void putInStack(tape_chunk * string, tape_chunk * current) {
    stack_node * new = (stack_node *) malloc(sizeof(stack_node));
    if (string == NULL || new == NULL)
        return;
    new->tape = string;
    new->currChunk = current;
    new->next = stack;
    stack = new;

    if (DEBUG) {
        printf("Added tape in stack\n");
        printStack();
    }

}

//***************************************
void popFromStack() {
    if (stack == NULL) {
        if (DEBUG) printf("Stack already empty\n");
        return;
    }
    stack_node * p = stack;
    stack = stack->next;
	freeTape(p->tape);
    free(p);
    if (DEBUG) {
        printf("Removed tape from stack\n");
        printStack();
    }
}

//****************************************************************
int updateIndex(int index) {
    tape_chunk * currChunk = stack->currChunk;

    if (index < 0) {
        if (currChunk->left == NULL)
            prependNewTapeChunk(currChunk);
        index = strlen(currChunk->string)-2;         // exclude the '\0'
        currChunk = currChunk->left;
    }
    else if (index > strlen(currChunk->string)-2) {  // exclude the '\0'
        if (currChunk->right == NULL)
            appendNewTapeChunk(currChunk);
        index = 0;
        currChunk = currChunk->right;
    }

    return index;
}

//****************************************************************
void prependNewTapeChunk(tape_chunk * current) {
    tape_chunk * newChunk = createNewChunk();
    stack->tape = newChunk;
    newChunk->right = current;
    current->left = newChunk;
}

//****************************************************************
void appendNewTapeChunk(tape_chunk * current) {
    tape_chunk * newChunk = createNewChunk();
    current->right = newChunk;
    newChunk->left = current;
}

//****************************************************************
tape_chunk * createNewChunk() {
    tape_chunk * newChunk = (tape_chunk *) malloc(sizeof(tape_chunk));
    newChunk->string = (char *) malloc(TAPE_CHUNK_LENGTH * sizeof(char));
    for (int i = 0; i < TAPE_CHUNK_LENGTH; i++)
        newChunk->string[i] = BLANK;
    newChunk->string[TAPE_CHUNK_LENGTH-1] = '\0';
    newChunk->left = NULL;
    newChunk->right = NULL;
    return newChunk;
}


//****************************************************************
int countAccessibleTransitions(int state, char c) {
	graph_node * p = graph[state];
	int i = 0;
	while (p != NULL) {
		if (p->in == c)
			i++;
		p = p->next;
	}
	return i;
}

/***************************************************************************
 * Calculates the correct computation results according to the results list
 ***************************************************************************/
char checkComputationResult() {
	if (acceptString == true)             // at least a path accpets the string ---> ACCEPT STRING
        return ACCEPT;
	if (atLeastAnUndefinedPath == true)   // at least an undefined value ---> UNDEFINED
		return UNDEFINED;
	return REJECT;                        // only rejection values ---> REJECT STRING
}

/***********************************************************************
 * Checks if the current state is an acceptation state
 ***********************************************************************/
bool checkIfAccState(int state) {
	acc_state * p = accStates;
	while (p != NULL) {
		if (p->value == state)
			return true;
		p = p->next;
	}
	return false;
}

//******************************************************************
void freeTape(tape_chunk * head) {
	tape_chunk * prec = head;
	tape_chunk * succ = head;
	while (succ != NULL) {
		succ = succ->right;
		free(prec->string);
		free(prec);
		prec = succ;
	}
}

/*****************************************************************
 * Frees the tapes stack
 *****************************************************************/
 void freeStack() {
     while(stack != NULL) {
        popFromStack();
     }
 }

 /*****************************************************************
  * Frees the tapes stack
  *****************************************************************/
  void freeAccStatesList() {
      acc_state * p;
      while(accStates != NULL) {
         p = accStates;
         accStates = accStates->next;
         free(p);
      }
  }

  /*****************************************************************
 * Frees the Turing Machine graph
 *****************************************************************/
void freeGraph() {
	int i = 0;
	graph_node * prec = NULL;
	graph_node * succ = NULL;
	while (i < states_dim) {
		if (graph[i] != NULL) {
			prec = graph[i];
			succ = graph[i];
			while (succ != NULL) {
				succ = succ->next;
				free(prec);
				prec = succ;
			}
		}
		i++;
	}
	free(graph);
}

//******************************************************************
void printTape(tape_chunk * currTape) {
	tape_chunk * p = currTape;
	int i = 0;
	while (p != NULL) {
		printf("chunk %d: %s\n", i, p->string);
		p = p->right;
		i++;
	}
}

/****************************************************************
 * Displays the current state of the tapes' stack
 ****************************************************************/
void printStack() {
    stack_node * p = stack;
    int i = 0;
    printf("STACK:\n");
    while (p != NULL) {
        printTape(p->tape);
        p = p->next;
		i++;
    }
    free(p);
    printf("\n\n");
}

/****************************************************************
 * Displays the Turing Machine graph
 ****************************************************************/
void printGraph() {
	graph_node * p;
	for (int i = 0; i < states_num; i++) {
		if (graph[i] != NULL) {
			p = graph[i];
			while (p != NULL) {
				char m;
				switch(p->move) {
					case 0: m = 'S'; break;
					case 1: m = 'R'; break;
					case -1: m = 'L'; break;
				}
				printf("\n%d %c %c %c %d", i, p->in, p->out, m, p->next_state);
				p = p->next;
			}
		}
	}
}

/**************************************************************
 * 						 Main function
 **************************************************************/
int main(int argc, char *argv[]) {
	graph = (graph_node **) malloc(DEFAULT_STATES_DIM * sizeof(graph_node *));
	initGraph();
	readMTStructure();

  if (DEBUG) {
  	printf("\nStates number: %d", states_num);
  	printf("\nAcceptation states: ");
  	acc_state * p = accStates;
  	while (p != NULL) {
  		printf("%d ", p->value);
  		p = p->next;
  	}
  	printf("\nIterations limit: %d", iterationsLimit);
  	printGraph();
  	printf("\n");
  }

	readInputStrings();

    #ifdef EVAL
        freeGraph();
        freeAccStatesList();
    #endif

	return 0;
}
