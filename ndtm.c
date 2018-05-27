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

#define OK   			 1
#define NO  			 0
#define UNDEF			 2
#define RIGHT 			'R'
#define LEFT  			'L'
#define STOP  			'S'
#define BLANK 			'_'
#define TERM_CHAR 		'@'
#define INPUT_DIM		2000
#define TAPE_DIM		2000
#define STARTING_INDEX  500

void initGraph();
void initTape();
void readFile();
void insertTransitionInGraph(int, char, char, char, int);
void constructTrasitionsTree();
int execute(char *, int, int, int);
int nextIndex(int, char);
void printGraph();
void printTape(int, char *);

typedef struct s {     // node of the graph
	char in;
	char out;
	char move;
	int next_state;
	struct s * next;   // list of all possible next states of the current one
} state;

state * states[30];       		// list containing all the states read from input
int states_num = 0;       		// the number of states of the TM
int accState;	     	  		// all passible acceptation states
int reject_state = -1;    		// the state of rejection
int iterationsLimit;	  		// the limit to the iteration number (to avoid machine loop)
char inputString[INPUT_DIM];    // current input string on the machine tape
int length;
char tape[TAPE_DIM];

int main(int argc, char *argv[]) {
	initGraph();
	readFile();
	//printf("\nStates number: %d", states_num);
	//printf("\nAcceptation states: %d", accState);
	//printf("\nIterations limit: %d", iterationsLimit);
	//printGraph();
	
	scanf("%s", inputString);
	while (strcmp(inputString, "end") != 0) {
		initTape();
		int result = execute(tape, STARTING_INDEX, 0, 1);
		if (result == UNDEF)
			printf("U\n");
		else 
			printf("%d\n", result);
		scanf("%s", inputString);
	}
}

/***************************************************************
* Initializes states vector elements to NULL
****************************************************************/
void initGraph() {
	for(int i = 0; i < 50; i++)
		states[i] = NULL;
}

/*************************************************************************
* Initializes tape with BLANK charaters before and after the input string
**************************************************************************/
void initTape() {
	length = strlen(inputString);
	for (int i = 0; i < STARTING_INDEX; i++)
		tape[i] = BLANK;
	strcpy(&tape[STARTING_INDEX], inputString);
	for (int i = STARTING_INDEX+length; i < TAPE_DIM; i++)
		tape[i] = BLANK;
	tape[TAPE_DIM-1] = '\0';
}

/***************************************************************
* Reads the file saving data in program structures:
* => reads only the lines that contain transitions
****************************************************************/
void readFile() {
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
	while (strcmp(in, "acc") != 0) { // cycle until find the word "acc"
		sscanf(in, "%d", &s); 		 // read state
		if (s >= states_num)		 // mantain the maximum state number
			states_num = s+1;
		scanf("%s", in);
		inputChar = in[0];    		 // read input character
		scanf("%s", in);
		outputChar = in[0];   		 // read output character
		scanf("%s", in);
		move = in[0];		  		 		 // read move
		scanf("%s", in);
		sscanf(in, "%d", &next_s); // read next state
		insertTransitionInGraph(s, inputChar, outputChar, move, next_s);
		scanf("%s", in);
	}

	int i = 0;
	while (strcmp(in, "max") != 0 ) { // read acceptation states
		scanf("%s", in);
		sscanf(in, "%d", &accState);
		i++;
	}

	scanf("%d", &iterationsLimit);    // read maximum number of iterations

	scanf("%s", in);                  // read the word "run" to start computations
	if (strcmp(in, "run") != 0)
		exit(0);
}

/****************************************************************
* Inserts the transition read from stdin in the transitions graph
*****************************************************************/

// NOTA: se aggiungessi sempre in testa nelle liste avrei costruzione del grafo in tempo costante

void insertTransitionInGraph(int s, char in, char out, char m, int n_s) {
	state * new = (state *) malloc(sizeof(state *));
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

	if (states[s] == NULL) {   // insert when list is empty
		states[s] = new;
		new->next = NULL;
		return;
	}

	if (new->in <= states[s]->in) { // insert as first element
		new->next = states[s];
		states[s] = new;
		return;
	}

	state *curr = states[s];          // insert in order (includes insertion as last)
	state *succ = curr->next;
	while (succ != NULL && new->in > succ->in) {
		curr = succ;
		succ = succ->next;
	}
	new->next = succ;
	curr->next = new;
	
	new = NULL;
	free(new);
	
	return;
}

/****************************************************************
* Executes the Turing Machine
*****************************************************************/
int execute(char * tape, int index, int curr_state, int iteration) {
	
	if (curr_state == accState) {
		/*printf("\n\n******************************************************");
		printf("\n********************** ACCEPTED **********************");
		printf("\n******************************************************");*/
		return OK;
	}

	else if (iteration > iterationsLimit) {
		/*printf("\n\n******************************************************");
		printf("\n********************** UNDEFINED *********************");
		printf("\n******************************************************");*/
		return UNDEF;
	}
	
	else if (curr_state == reject_state || states[curr_state] == NULL) {
		/*printf("\n\n******************************************************");
		printf("\n********************** REJECTED **********************");
		printf("\n******************************************************");*/
		return NO;
	}
		
	char newTape[TAPE_DIM];
	state *p = states[curr_state];
	
	while (p != NULL) {
		if (p->in == tape[index]) {
			strcpy(newTape, tape);
			newTape[index] = p->out;
			
			//printTape(index, newTape);
			
			//printf("\n%d %c %c %c %d -> iteration: %d", curr_state, p->in, p->out, p->move, p->next_state, iteration);
			//printf("\n%s", &tape[STARTING_INDEX]);
			//printf("\n%s", &newTape[STARTING_INDEX]);
			int result = execute(newTape, nextIndex(index, p->move), p->next_state, iteration+1);
			if (result == OK)
				return OK;
			else if (result == UNDEF)
				return UNDEF;
		}
		p = p->next;
	}
	
	return NO;
}

/***********************************************************************
* Returns the next index in the tape, according to the transition move
************************************************************************/
int nextIndex(int i, char m) {
	if (m == RIGHT)
		 return i+1;
	else if (m == LEFT)
		 return i-1;
	else if (m == STOP)
		 return i;
	return -1;
}

/****************************************************************
* Displays the transitions graph
*****************************************************************/
void printGraph() {
	for (int i = 0; i < states_num; i++) {
		if (states[i] != NULL) {
			state *p = states[i];
			while (p != NULL) {
				printf("\n%d %c %c %c %d", i, p->in, p->out, p->move, p->next_state);
				p = p->next;
			}
		}
	}
}

/****************************************************************
* Displays the tape in the current state
*****************************************************************/
void printTape(int i, char * t) {
	printf("\n");
	for (int j = 0; j < length; j++)
		printf("%c", t[STARTING_INDEX+j]);
	printf("\n");
	for (int j = STARTING_INDEX; j < i; j++)
		printf("%c", ' ');
	printf("∆\n");
}
