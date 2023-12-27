//
// Created by vyach on 04.10.2023.
//

#ifndef LLP_LAB1_LIST_H
#define LLP_LAB1_LIST_H

#include <stdbool.h>
#include <stddef.h>

#define FOR_LIST(L, I, CODE)  {\
    ListIterator I = ListHeadIterator(L); \
while (!ListIteratorIsEmpty(I)) { \
 CODE \
ListIteratorNext(I); \
} \
ListIteratorFree(&(I)); }

typedef struct List *List;

typedef struct ListIterator *ListIterator;

typedef void *ListValue;

typedef void (*Applier)(void *);

List ListNew(void);

void ListFree(List list);

void ListApply(List list, Applier applier);

size_t ListSize(List list);

bool ListIsEmpty(List list);

void ListAppendHead(List list, ListValue value);

void ListAppendTail(List list, ListValue value);

void ListRemoveHead(List list);

void ListRemoveTail(List list);

ListValue ListGetHead(List list);

ListValue ListGetTail(List list);

ListIterator ListHeadIterator(List list);

ListIterator ListTailIterator(List list);

void ListIteratorFree(ListIterator *it);

ListValue ListIteratorGet(ListIterator it);

bool ListIteratorIsEmpty(ListIterator it);

void ListIteratorNext(ListIterator it);

void ListIteratorPrev(ListIterator it);

void ListIteratorDeleteNode(ListIterator it);

#endif //LLP_LAB1_LIST_H
