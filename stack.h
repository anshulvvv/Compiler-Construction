#ifndef STACK_H
#define STACK_H
#include "parserDef.h"
typedef struct StkNode
{
    parsetreeNode *data;
    struct StkNode *next;
} StkNode;

typedef StkNode *stkNODE;

typedef struct stack
{
    stkNODE top;
} Stack;

void push(Stack *stack, parsetreeNode *ptn);
void pop(Stack *stack);
parsetreeNode *top(Stack *stack);
Stack *createNewStack();
bool isEmpty(Stack *stack);

#endif