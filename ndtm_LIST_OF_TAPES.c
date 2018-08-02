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

#define OK     	              '1'
#define NO                    '0'
#define UNDEF	              'U'
#define RIGHT                  1
#define LEFT  			      -1
#define STOP  			       0
#define BLANK 			      '_'
#define DEFAULT_INPUT_DIM     50
#define INPUT_INCREMENT		  50
#define DEFAULT_STATES_DIM    25
#define STATES_INCREMENT      5
#define TAPE_CHUNK_LENGTH	  100 + 1

typedef enum {true, false} bool;

typedef struct transition {     // node of the graph
	char in;
	char out;
	int move;
	int next_state;
	struct transition * next;   // list of all possible next states of the current one
} transition;

typedef struct tape_chunk {
	char * string;
	struct tape_chunk * right;
	struct tape_chunk * left;
} tape_chunk;

typedef struct int_node {
	int value;
	struct int_node * next;
} int_node;

void initGraph();
void readMTStructure();
void addAcceptationState(int);
void readInputStrings();
//void putInputOnTape();
void addTapeChunk(char *);
void run();
void execute(tape_chunk *, tape_chunk *, tape_chunk *, int, int, int);
void insertTransitionInGraph(int, char, char, int, int);
bool checkIfAccState(int);
void insertResult(char);
char checkComputationResult();
void printGraph();
void printTape();
void printResults();
void freeTape(tape_chunk *, tape_chunk *);
void freeGraph();
void freeAccStatesList();

int states_num = 0;											// the number of states of the TM
int states_dim = DEFAULT_STATES_DIM;    // the actual size of states array (graph)
transition ** graph;				    				// array containing all the states read from input

int_node * accStates = NULL;	     	    // acceptation states list
int startingState = 0;
int reject_state = -1;    							// the state of rejection
int iterationsLimit;            				// the limit to the iteration number (to avoid machine loop)

int_node * resList = NULL;							// list containing all computation results (OK, NO or UNDEF)

bool acceptString = false;            // true when a path accepts the input string

tape_chunk * tape = NULL;					  // the tape of the Turing Machine
tape_chunk * tape_tail = NULL;
//tape_chunk * curr_chunk;
int chunks_number;					  // total number of chunks composing the Turing Machine tape

/**************************************************************
 * 					     Main function
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
	freeGraph();
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
	int move = 2;
	int next_s = -1;
	char in[5];

	scanf("%s", in);
	if (strcmp(in, "tr") != 0)
		exit(0);

	scanf("%s", in);
	while (strcmp(in, "acc") != 0) { 	// cycle until find the word "acc"
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

/******************************************************************
 * Inserts the transition read from stdin in the transitions graph
 ******************************************************************/

void insertTransitionInGraph(int s, char in, char out, int m, int n_s) {

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
	 new = NULL;
 	 free(new);
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
		//printf("%d ", c);
		if (i == TAPE_CHUNK_LENGTH-1 && c != EOF) {
			//printf("\nRIALLOCO VETTORE INPUT (dim = %d)\n\n", input_dim);
			inputString[i] = '\0';
			addTapeChunk(inputString);
			//printf("\ninput string: %s", inputString);
			i = 0;
		}
		
		if (i != 0 && (c == '\n' || (c == EOF && inputString[i-1] != '\0'))) {
			inputString[i] = '\0';
			//printf("\nlength: %d", (int) strlen(inputString));
			if (strlen(inputString) != 0)
				addTapeChunk(inputString);
			//printf("\ninput string: %s", inputString);
			//putInputOnTape();
			run();
			i = 0;
		}
		else if (c != EOF) {
			inputString[i] = c;
			i++;
		}
	}
}

//******************************************************************
/*
void putInputOnTape() {
	tape = NULL;
	char * string = (char *) malloc(TAPE_CHUNK_LENGTH * sizeof(char));
	int tape_index = 0;
	int i = 0;
	while (inputString[i] != '\0') {
		if (tape_index == TAPE_CHUNK_LENGTH-1) {
			string[TAPE_CHUNK_LENGTH-1] = '\0';
			//printf("string: %s\n", string);
			addTapeChunk(string);
			tape_index = 0;
		}
		else {
			string[tape_index] = inputString[i];
			tape_index++;
			i++;
		}
	}
	
	if (strlen(string) > 0) {
		string[tape_index] = '\0';
		//printf("string: %s\n", string);
		addTapeChunk(string);
	}
	
	free(string);
}
*/
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
	
	new = NULL;
	free(new);

	chunks_number++;
}

/*************************************************************************
 * Runs the actual computation, calculating the result
 *************************************************************************/
void run() {
	//printTape();
	acceptString = false;
	execute(tape, tape, tape_tail, 0, startingState, 1);
	freeTape(tape, tape_tail);
	tape = NULL;
	tape_tail = NULL;
}

/****************************************************************
* Executes the Turing Machine
*****************************************************************/
void execute(tape_chunk * head, tape_chunk * curr_chunk, tape_chunk * tail, int index, int curr_state, int iteration) {
	
}

/****************************************************************
 * Inserts a node in the results list
 ****************************************************************/
void insertResult(char value) {
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

/***************************************************************************
 * Calculates the correct computation results according to the results list
 ***************************************************************************/
char checkComputationResult() {
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

//******************************************************************

void printTape() {
	tape_chunk * p = tape;
	int i = 0;
	printf("\n");
	while (p != NULL) {
		printf("chunk %d: %s\n", i, p->string);
		p = p->right;
		i++;
	}
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

//******************************************************************

void freeTape(tape_chunk * head, tape_chunk * tail) {
	tape_chunk * prec = head;
	tape_chunk * succ = tail;
	while (succ != NULL) {
		succ = succ->right;
		free(prec->string);
		free(prec);
		prec = succ;
	}
	free(succ);
}

//******************************************************************

void freeGraph() {
	int i = 0;
	transition * prec = NULL;
	transition * succ = NULL;
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
