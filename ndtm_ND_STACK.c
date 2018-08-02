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

#define OK     	               '1'
#define NO                     '0'
#define UNDEF	               'U'
#define RIGHT                  'R'
#define LEFT  			       'L'
#define STOP  			       'S'
#define BLANK 			       '_'
#define DEFAULT_PADDING_DIM     5
#define DEFAULT_INPUT_DIM       50
#define INPUT_INCREMENT		    50
#define DEFAULT_STATES_DIM      25
#define STATES_INCREMENT        5
#define DEFAULT_TAPE_STACK_DIM  100
#define TAPE_STACK_INCREMENT    10
#define DEBUG 					0

typedef enum {true, false} bool;

typedef struct tr {     // node of the graph
	char in;
	char out;
	char move;
	int next_state;
	struct tr * next;   // list of all possible next states of the current one
} transition;

typedef struct r {
	int value;
	struct r * next;
} int_node;

void initGraph();
void initTape();
void readMTStructure();
void readInputStrings();
void run();
void addAcceptationState(int);
void insertTransitionInGraph(int, char, char, char, int);
void printGraph();
void printTape(int, char *);
void execute(int, int, int, int);
int countAccessibleTransitions(int, char);
void initTapeStack();
int putTapeInStack(char *);
void popTapeFromStack(int);
bool checkIfAccState(int);
int nextIndex(int, char);
char * reallocTape(int *, char *, char);
void freeTapeStack();
void freeResultsList();
void insertResult(int);
void printTapeStack();
void printResults();
int checkComputationResult();

int states_num = 0;											// the number of states of the TM
int states_dim = DEFAULT_STATES_DIM;    // the actual size of states array (graph)
transition ** graph;				    				// array containing all the states read from input

int_node * accStates = NULL;	     	    // acceptation states list
int startingState = 0;
int reject_state = -1;    							// the state of rejection
int iterationsLimit;            				// the limit to the iteration number (to avoid machine loop)

//int tape_dim = DEFAULT_TAPE_DIM;        // the current tape length
char * tape;                            // the tape of the Turing Machine

char ** tape_stack;						     // the stack to trace non-deterministic transition
//int current_tape_stack_index = 0;			 // the current index on the stack
int tape_stack_dim = DEFAULT_TAPE_STACK_DIM; // the current length of the stack array;

int input_dim;							// the current length of the inputString array
char * inputString;						// the string read from input

int_node * resList = NULL;							// list containing all computation results (OK, NO or UNDEF)

bool acceptString = false;            // true when a path accepts the input string

/**************************************************************
 * 										Main function
 **************************************************************/
int main(int argc, char *argv[]) {
	graph = (transition **) malloc(DEFAULT_STATES_DIM * sizeof(transition *));
	initGraph();
	readMTStructure();

/*	printf("\nStates number: %d", states_num);
	printf("\nAcceptation states: ");
	int_node * p = accStates;
	while (p != NULL) {
		printf("%d ", p->value);
		p = p->next;
	}
	printf("\nIterations limit: %d", iterationsLimit);
	printGraph();
	printf("\n"); */
	readInputStrings();
	free(graph);
	return 0;
}

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
	char move = ' ';
	int next_s = -1;
	char in[5];

	scanf("%s", in);
	if (strcmp(in, "tr") != 0)
		exit(0);

	scanf("%s", in);
	while (strcmp(in, "acc") != 0) { 	// cycle until find the word "acc"
		sscanf(in, "%d", &s); 		 			// read state
		scanf("%s", in);
		inputChar = in[0];    		 			// read input character
		scanf("%s", in);
		outputChar = in[0];   		 			// read output character
		scanf("%s", in);
		move = in[0];		  		 					// read move
		scanf("%s", in);
		sscanf(in, "%d", &next_s);   		// read next state
		if (s >= states_num || next_s >= states_num) {	 				// maintain the maximum state number
			if (s >= next_s)
				states_num = s + 1;
			else
				states_num = next_s + 1;
		}
		if (states_num >= states_dim) {
			graph = (transition **) realloc(graph, (states_num + STATES_INCREMENT) * sizeof(transition *));
			// re-initialize states vector (graph) with NULL in new positions
			for (int i = states_dim; i < states_num + STATES_INCREMENT; i++) 
				graph[i] = NULL;
			states_dim = states_num + STATES_INCREMENT;
			//printf("\nRIALLOCO VETTORE STATI (dim = %d)\n\n", states_dim);
		}
		insertTransitionInGraph(s, inputChar, outputChar, move, next_s);
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

/*******************************************************************
 * Insert a new integer in accStates list (acceptation states list)
 *******************************************************************/
void addAcceptationState(int s) {
	 int_node * new = (int_node *) malloc(sizeof(int_node));
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
 	 //free(new);
 }

/****************************************************************
 * Reads the next input string from stdin
 ****************************************************************/
void readInputStrings() {

	input_dim = DEFAULT_INPUT_DIM; 																			// actual size of the input string
	inputString = (char *) malloc(DEFAULT_INPUT_DIM * sizeof(char)); 		// current input string
	
	int i = 0;
	char c = ' ';

	while (c != EOF) {
		c = getchar();
		//printf("%d ", c);
		if (i == input_dim && c != EOF) {
			inputString = (char *) realloc(inputString, (input_dim + INPUT_INCREMENT) * sizeof(char));
			input_dim = input_dim + INPUT_INCREMENT;
			//printf("\nRIALLOCO VETTORE INPUT (dim = %d)\n\n", input_dim);
		}
		
		if (i != 0 && (c == '\n' || (c == EOF && inputString[i] != '\0'))) {
			inputString[i] = '\0';
			//printf("input string: %s\n", inputString);
			i = 0;
			run();
		}
		else if (c != EOF) {
			inputString[i] = c;
			i++;
		}
	}
	
	free(inputString);
}

/*************************************************************************
 * Runs the actual computation, calculating the result
 *************************************************************************/
void run() {
	if (DEBUG) printf("\n---------------------------------------------\n");
	if (DEBUG) printf("run 1\n");
	initTape();
	if (DEBUG) printf("run 2\n");
	initTapeStack();
	acceptString = false;
	if (DEBUG) printf("run 3\n");
	execute(DEFAULT_PADDING_DIM, 0, startingState, 1);
	printf("%c\n", checkComputationResult());
	free(tape);
	freeTapeStack(tape_stack);
	freeResultsList();
}

/*************************************************************************
 * Initializes tape with BLANK charaters before and after the input string
 *************************************************************************/
void initTape() {
	int length = (int) strlen(inputString);
	tape = (char *) malloc((length + 2*DEFAULT_PADDING_DIM) * sizeof(char));
	//tape_dim = length + 2*DEFAULT_PADDING_DIM;
	for (int i = 0; i < DEFAULT_PADDING_DIM; i++)
		tape[i] = BLANK;
	strcpy(&tape[DEFAULT_PADDING_DIM], inputString);
	for (int i = DEFAULT_PADDING_DIM+length; i < length + 2*DEFAULT_PADDING_DIM; i++)
		tape[i] = BLANK;
	tape[length+2*DEFAULT_PADDING_DIM-1] = '\0';
}

//********************************************************************
void initTapeStack() {
	tape_stack = (char **) malloc(DEFAULT_TAPE_STACK_DIM * sizeof(char *));
	for (int i = 0; i < DEFAULT_TAPE_STACK_DIM; i++) {
		tape_stack[i] = NULL;
	}
	tape_stack[0] = tape;
}

/******************************************************************
 * Inserts the transition read from stdin in the transitions graph
 ******************************************************************/

void insertTransitionInGraph(int s, char in, char out, char m, int n_s) {

	transition * new = (transition *) malloc(sizeof(transition));
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

	transition *curr = graph[s];          // insert in order (includes insertion as last)
	transition *succ = curr->next;
	while (succ != NULL && new->in > succ->in) {
		curr = succ;
		succ = succ->next;
	}

	new->next = succ;
	curr->next = new;
	new = NULL;
	free(new);
}

/****************************************************************
* Executes the Turing Machine
*****************************************************************/
void execute(int index, int tape_stack_index, int curr_state, int iteration) {
	
	if (acceptString == true) {
		if (DEBUG) printf("accept\n");
		//popTapeFromStack();
		return;
	}

	if (checkIfAccState(curr_state) == true) {
		if (DEBUG) printf("accept 2\n");
		insertResult(OK);
		acceptString = true;
		//popTapeFromStack();
		return;
	}
	else if (iteration == iterationsLimit) {
		if (DEBUG) printf("undefined\n");
		insertResult(UNDEF);
		popTapeFromStack(tape_stack_index);
		return;
	}

	/* if (curr_state < 0 || curr_state >= states_num) {
		printf("Invalid state number.\n");
		exit(0);
	} */
		
	transition * p = graph[curr_state];
	if (DEBUG) printf("\nTape stack index: %d\n", tape_stack_index);
	if (DEBUG) printf("Current tape: %s\n", tape_stack[tape_stack_index]);
	char * currTape = tape_stack[tape_stack_index];

	if (index == 0) {
		if (DEBUG) printf("realloc tape left\n");
		currTape = reallocTape(&index, currTape, LEFT);
	}
	else if (index == strlen(currTape)-1) {
		if (DEBUG) printf("realloc tape right\n");
		currTape = reallocTape(&index, currTape, RIGHT);
	}
	
	int transitions_number = countAccessibleTransitions(curr_state, currTape[index]);
	if (DEBUG) printf("Transitions number: %d\n", transitions_number);

	bool reject = true;
	
	while (p != NULL) {
		if (p->in == currTape[index]) {
			reject = false;
			int next_index = nextIndex(index, p->move);
			
			if (DEBUG) printf("%d %c %c %c %d -> iteration: %d\n", curr_state, p->in, p->out, p->move, p->next_state, iteration);
			if (DEBUG) printf("Next index: %d\n", next_index);
			if (DEBUG) printTape(index, currTape);
			
			if (transitions_number >= 2) {
				if (DEBUG) printf("\n##########   NON DETERMINISTIC TRANSITION   ##########\n");
				char * newTape = (char *) malloc((strlen(currTape)+1) * sizeof(char));
				strcpy(newTape, currTape);
				newTape[index] = p->out;
				if (DEBUG) printf("New tape: %s\n", newTape);
				tape_stack_index = putTapeInStack(newTape);
				if (DEBUG) printf("\nNew tape stack index: %d\n", tape_stack_index);
				newTape = NULL;
				free(newTape);
			}
			else
				currTape[index] = p->out;

			execute(next_index, tape_stack_index, p->next_state, iteration+1);
		}
		p = p->next;
	}

	if (transitions_number == true) {
		if (DEBUG) printf("reject\n");
		insertResult(NO);
		popTapeFromStack(tape_stack_index);
	}
	
	free(p);
	return;
}

//****************************************************************
int countAccessibleTransitions(int state, char c) {
	transition * p = graph[state];
	int i = 0;
	while (p != NULL) {
		if (p->in == c) 
			i++;
		p = p->next;
	}
	free(p);
	return i;
}

//****************************************************************
int putTapeInStack(char * toBeAdded) {
	
	int tape_stack_index;
	bool ok = false;
	for (tape_stack_index = 0; tape_stack_index < tape_stack_dim && !ok; tape_stack_index++)
		if (tape_stack[tape_stack_index] == NULL) {
			ok = true;
		}
	
	if (tape_stack_index == tape_stack_dim) {
		tape_stack = (char **) realloc(tape_stack, DEFAULT_TAPE_STACK_DIM+tape_stack_dim * sizeof(char *));
		for (int i = tape_stack_dim; i < DEFAULT_TAPE_STACK_DIM+tape_stack_dim; i++) {
			tape_stack[i] = NULL;
		}
		tape_stack_index = tape_stack_dim;
		tape_stack_dim = tape_stack_dim + DEFAULT_TAPE_STACK_DIM;
	}
	
	tape_stack[tape_stack_index] = toBeAdded;

	if (DEBUG) printTapeStack();
	
	return tape_stack_index;
}

//****************************************************************
void popTapeFromStack(int tape_stack_index) {
	
	if (DEBUG) printTapeStack();
	
	//free(tape_stack[tape_stack_index]);
	if (tape_stack_index > 0) {
		tape_stack[tape_stack_index] = NULL;
		//tape_stack_index--;
	}
}

/***********************************************************************
 * Checks if the current state is an acceptation state
 ***********************************************************************/
bool checkIfAccState(int state) {
	int_node * p = accStates;
	while (p != NULL) {
		if (p->value == state)
			return true;
		p = p->next;
	}
	return false;
}

/***********************************************************************
 * Returns the next index in the tape, according to the transition move
 ***********************************************************************/
int nextIndex(int i, char m) {
	if (m == RIGHT)
		return i+1;
	else if (m == LEFT)
		return i-1;
	else if (m == STOP)
		return i;
	return 0;
}

/****************************************************************
* Reallocates the current tape of the Turing Machine
****************************************************************/
char * reallocTape(int * index, char * currTape, char side) {
	int length = strlen(currTape)+1;
	char * tapeCopy = (char *) malloc(length * sizeof(char));
	currTape = realloc(currTape, (length + DEFAULT_PADDING_DIM) * sizeof(char));
	strcpy(tapeCopy, currTape);
	if (side == LEFT) {
		for (int i = 0; i < DEFAULT_PADDING_DIM; i++)
			currTape[i] = BLANK;
		strcpy(&currTape[DEFAULT_PADDING_DIM], tapeCopy);
	}
	else if (side == RIGHT) {
		for (int i = length-1; i < strlen(currTape); i++)
			currTape[i] = BLANK;
	}
	currTape[strlen(currTape)-1] = '\0';
	*index = *index + DEFAULT_PADDING_DIM;
	//printf("\nRIALLOCO VETTORE TAPE (dim = %d)\n\n", (int) strlen(currTape));
	free(tapeCopy);
	return currTape;
}


/****************************************************************
 * Inserts a node in the results list
 ****************************************************************/
void insertResult(int value) {
	int_node * new = (int_node *) malloc(sizeof(int_node));
	if (new != NULL) {
		new->value = value;
		new->next = NULL;
	}
	else {
		printf("Error: not enough memory...");
		exit(0);
	}
	new->next = resList;
	resList = new;
	new = NULL;
	free(new);
}

/**************************************************************************
 * Calculates the correct computation results according to the results list
 **************************************************************************/
int checkComputationResult() {
	int_node * p = resList;
	bool trovUNDEF = false;
	//printResults();
	while (p != NULL) {
		if (p->value == OK)
			return OK;       // at least an acceptation ---> ACCEPT STRING
		else if (p->value == UNDEF)
			trovUNDEF = true;
		p = p->next;
	}
	if (trovUNDEF == true)  // at least an undefined value ---> UNDEFINED
		return UNDEF;
	return NO;              // only rejection values ---> REJECT STRING
}

//*************************************************************************
void freeTapeStack() {
	for (int i = 0; i < tape_stack_dim; i++) {
		free(tape_stack[i]);
	}
	free(tape_stack);
}

/**************************************************************************
 * Frees the results list
 **************************************************************************/
void freeResultsList() {
	int_node * prec = resList;
	int_node * succ = resList;
	while (succ != NULL) {
		succ = succ->next;
		free(prec);
		prec = succ;
	}
	free(succ);
	resList = NULL;
}

/****************************************************************
 * Displays the transitions graph
 ****************************************************************/
void printGraph() {
	transition * p;
	for (int i = 0; i < states_num; i++) {
		if (graph[i] != NULL) {
			p = graph[i];
			while (p != NULL) {
				printf("\n%d %c %c %c %d", i, p->in, p->out, p->move, p->next_state);
				p = p->next;
			}
		}
	}
}

/****************************************************************
 * Displays the tape in the current state
 ****************************************************************/
void printTape(int i, char * t) {
	for (int j = 0; j < strlen(t); j++)
		printf("%c", t[j]);
	printf("\n");
	for (int j = 0; j < i; j++)
		printf("%c", ' ');
	printf("∆\n");
}

//***************************************************************
void printTapeStack() {
	printf("\nTape stack: ");
	for (int i = 0; i < tape_stack_dim; i++) {
		if (tape_stack[i] != NULL)
			printf("\n%d: %s", i, tape_stack[i]);
	}
	printf("\n");
}

/****************************************************************
 * Displays the current results list
 ****************************************************************/
void printResults() {
	int_node * p = resList;
	printf("\nRESULTS: ");
	while (p != NULL) {
		if (p->value == OK)
			printf(" 1");
		else if (p->value == NO)
			printf(" 0");
		else if (p->value == UNDEF)
			printf(" U");
		p = p->next;
	}
	printf("\n");
}
