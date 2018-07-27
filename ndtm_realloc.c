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

#define OK   			    1
#define NO                  0
#define UNDEF			    2
#define RIGHT 			   'R'
#define LEFT  			   'L'
#define STOP  			   'S'
#define BLANK 			   '_'
#define TERM_CHAR 		   '@'
#define DEFAULT_TAPE_DIM   200
#define STARTING_INDEX     50
#define DEFAULT_INPUT_DIM  250
#define DEFAULT_STATES_DIM 25

void initGraph();
void readMTStructure();
void readInputString();
void insertTransitionInGraph(int, char, char, char, int);
void printGraph();

typedef enum {true, false} bool;

typedef struct s {     // node of the graph
	char in;
	char out;
	char move;
	int next_state;
	struct s * next;   // list of all possible next states of the current one
} transition;


int states_num = 0;						// the number of states of the TM
int states_dim = DEFAULT_STATES_DIM;    // the actual size of states array (graph)
transition ** graph;				    // array containing all the states read from input

int accState;	     	  				// all passible acceptation states
int reject_state = -1;    				// the state of rejection
int iterationsLimit;            		// the limit to the iteration number (to avoid machine loop)

char * tape;

/**************************************************************
 * 						Main function
 **************************************************************/
int main(int argc, char *argv[]) {
	
	graph = (transition **) malloc(DEFAULT_STATES_DIM * sizeof(transition));
	
	initGraph();
	readMTStructure();
	
	printf("\nStates number: %d", states_num);
	printf("\nAcceptation state: %d", accState);
	printf("\nIterations limit: %d", iterationsLimit);
	
	printGraph();
	
	printf("\n");
	
	readInputString();
}

/***************************************************************
 * Initializes graph (states vector) elements to NULL
 ***************************************************************/
void initGraph() {
	for(int i = 0; i < DEFAULT_STATES_DIM; i++)
		graph[i] = NULL;
}

/******************************************************************
 * Reads the stind (MT structure) saving data in program structures
 ******************************************************************/
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
	while (strcmp(in, "acc") != 0) { // cycle until find the word "acc"
		sscanf(in, "%d", &s); 		 // read state
		if (s >= states_num)		 // maintain the maximum state number
			states_num = s+1;
		scanf("%s", in);
		inputChar = in[0];    		 // read input character
		scanf("%s", in);
		outputChar = in[0];   		 // read output character
		scanf("%s", in);
		move = in[0];		  		 // read move
		scanf("%s", in);
		sscanf(in, "%d", &next_s);   // read next state
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
 * Reads the next input string from stdin
 ****************************************************************/
void readInputString() {
	
	int input_dim = DEFAULT_INPUT_DIM; 										// actual size of the input string
	char * inputString = (char *) malloc(DEFAULT_INPUT_DIM * sizeof(char)); // current input string on the machine tape
	int i = 0;
	char c = ' ';
	
	while (c != EOF) {
		c = getchar();
		if (i == input_dim) {
			inputString = realloc(inputString, 4 * input_dim * sizeof(char));
			input_dim = input_dim * 4;
			printf("\nRIALLOCO VETTORE INPUT (dim = %d)\n", input_dim);
		}
		if (c == '\n' || c == EOF) {
			i = 0;
			printf("input string: %s\n", inputString);
			for (int j = 0; j < input_dim; j++)
				inputString[j] = '\0';
		}
		else {
			inputString[i] = c;
			i++;
		}
	}
}

/*****************************************************************
 * Inserts the transition read from stdin in the transitions graph
 *****************************************************************/

void insertTransitionInGraph(int s, char in, char out, char m, int n_s) {
	
	if (s >= states_dim) {
		graph = realloc(graph, 2 * s * sizeof(transition));
		for (int i = states_dim; i < s * 2; i++) // re-initialize states vector with NULL in new positions
			graph[i] = NULL;
		states_dim = s * 2;
		printf("\nRIALLOCO VETTORE STATI (dim = %d)\n", states_dim);
	}
	
	transition * new = (transition *) malloc(sizeof(transition *));
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
	
	return;
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