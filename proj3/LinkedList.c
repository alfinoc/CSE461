/*
 * Copyright 2012 Steven Gribble
 *
 *  This file is part of the UW CSE 333 project sequence (333proj12).
 *
 *  333proj12 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License,
 *  or (at your option) any later version.
 *
 *  333proj12 is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with 333proj12.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>

#include "Assert333.h"
#include "LinkedList.h"
#include "LinkedList_priv.h"

LinkedList AllocateLinkedList(void) {
  // allocate the linked list record
  LinkedList ll = (LinkedList) malloc(sizeof(LinkedListHead));
  if (ll == NULL) {
    // out of memory
    return (LinkedList) NULL;
  }

  // initialize the newly allocated record structure
  ll->num_elements = 0;
  ll->head = NULL;
  ll->tail = NULL;

  // return our newly minted linked list
  return ll;
}

void FreeLinkedList(LinkedList list,
                    LLPayloadFreeFnPtr payload_free_function) {
  // defensive programming: check arguments for sanity.
  Assert333(list != NULL);
  Assert333(payload_free_function != NULL);

  // sweep through the list and free all of the nodes' payloads as
  // well as the nodes themselves
  while (list->head != NULL) {
    LinkedListNodePtr next = (list->head)->next;
    payload_free_function((list->head)->payload);
    free(list->head);
    list->head = next;
  }

  // free the list record
  free(list);
}

uint64_t NumElementsInLinkedList(LinkedList list) {
  // defensive programming: check argument for safety.
  Assert333(list != NULL);
  return list->num_elements;
}

bool PushLinkedList(LinkedList list, void *payload) {
  // defensive programming: check argument for safety. The user-supplied
  // argument can be anything, of course, so we need to make sure it's
  // reasonable (e.g., not NULL).
  Assert333(list != NULL);

  // allocate space for the new node.
  LinkedListNodePtr ln =
    (LinkedListNodePtr) malloc(sizeof(LinkedListNode));
  if (ln == NULL) {
    // out of memory
    return false;
  }

  // set the payload
  ln->payload = payload;
  if (list->num_elements == 0) {
    // degenerate case; list is currently empty
    Assert333(list->head == NULL);  // debugging aid
    Assert333(list->tail == NULL);  // debugging aid
    ln->next = ln->prev = NULL;
    list->head = list->tail = ln;
    list->num_elements = 1U;
    return true;
  }

  // typical case; list has >=1 elements
  ln->next = list->head;
  (list->head)->prev = ln;
  ln->prev = NULL;
  list->head = ln;
  (list->num_elements)++;

  // return success
  return true;
}

bool PopLinkedList(LinkedList list, void **payload_ptr) {
  // defensive programming.
  Assert333(payload_ptr != NULL);
  Assert333(list != NULL);

  if (list->num_elements == 0)
    return false;

  if (list->num_elements == 1)
    list->tail = NULL;

  LinkedListNodePtr popped = list->head;
  list->head = popped->next;

  if (list->num_elements > 1)
    (list->head)->prev = NULL;

  *payload_ptr = popped->payload;
  free(popped);
  (list->num_elements)--;
  return true;
}

bool AppendLinkedList(LinkedList list, void *payload) {
  // defensive programming: check argument for safety.
  Assert333(list != NULL);

  // allocate space for the new node.
  LinkedListNodePtr ln =
    (LinkedListNodePtr) malloc(sizeof(LinkedListNode));
  if (ln == NULL) {
    // out of memory
    return false;
  }

  ln->payload = payload;
  if (list->num_elements == 0) {
    // degenerate case; list is currently empty
    Assert333(list->head == NULL);  // debugging aid
    Assert333(list->tail == NULL);  // debugging aid
    ln->next = ln->prev = NULL;
    list->head = list->tail = ln;
    list->num_elements = 1U;
    return true;
  }

  // typical case; list has >=1 elements
  ln->prev = list->tail;
  (list->tail)->next = ln;
  ln->next = NULL;
  list->tail = ln;
  (list->num_elements)++;

  // return success
  return true;
}

bool SliceLinkedList(LinkedList list, void **payload_ptr) {
  // defensive programming.
  Assert333(payload_ptr != NULL);
  Assert333(list != NULL);

  // Step 6: implement SliceLinkedList.

  if (list->num_elements == 0) {
    // can't pop from empty list
    return false;
  }

  if (list->num_elements == 1) {
    list->head = NULL;
  }

  LinkedListNodePtr popped = list->tail;
  list->tail = popped->prev;

  if (list->num_elements > 1) {
    (list->tail)->next = NULL;
  }

  *payload_ptr = popped->payload;
  free(popped);
  (list->num_elements)--;
  return true;
}

void SortLinkedList(LinkedList list, unsigned int ascending,
                    LLPayloadComparatorFnPtr comparator_function) {
  Assert333(list != NULL);  // defensive programming
  if (list->num_elements < 2) {
    // no sorting needed
    return;
  }

  // we'll implement bubblesort! nice and easy, and nice and slow :)
  int swapped;
  do {
    LinkedListNodePtr curnode;

    swapped = 0;
    curnode = list->head;
    while (curnode->next != NULL) {
      int compare_result = comparator_function(curnode->payload,
                                               curnode->next->payload);
      if (ascending) {
        compare_result *= -1;
      }
      if (compare_result < 0) {
        // bubble-swap payloads
        void *tmp;
        tmp = curnode->payload;
        curnode->payload = curnode->next->payload;
        curnode->next->payload = tmp;
        swapped = 1;
      }
      curnode = curnode->next;
    }
  } while (swapped);
}

LLIter LLMakeIterator(LinkedList list, int pos) {
  // defensive programming
  Assert333(list != NULL);
  Assert333((pos == 0) || (pos == 1));

  // if the list is empty, return failure.
  if (NumElementsInLinkedList(list) == 0U)
    return NULL;

  // OK, let's manufacture an iterator.
  LLIter li = (LLIter) malloc(sizeof(LLIterSt));
  if (li == NULL) {
    // out of memory!
    return NULL;
  }

  // set up the iterator.
  li->list = list;
  if (pos == 0) {
    li->node = list->head;
  } else {
    li->node = list->tail;
  }

  // return the new iterator
  return li;
}

void LLIteratorFree(LLIter iter) {
  // defensive programming
  Assert333(iter != NULL);
  free(iter);
}

bool LLIteratorHasNext(LLIter iter) {
  // defensive programming
  Assert333(iter != NULL);
  Assert333(iter->list != NULL);
  Assert333(iter->node != NULL);

  // Is there another node beyond the iterator?
  if (iter->node->next == NULL)
    return false;  // no

  return true;  // yes
}

bool LLIteratorNext(LLIter iter) {
  // defensive programming
  Assert333(iter != NULL);
  Assert333(iter->list != NULL);
  Assert333(iter->node != NULL);

  if (LLIteratorHasNext(iter)) {
    iter->node = (iter->node)->next;
    return true;
  }

  // Nope, there isn't another node, so return failure.
  return false;
}

bool LLIteratorHasPrev(LLIter iter) {
  // defensive programming
  Assert333(iter != NULL);
  Assert333(iter->list != NULL);
  Assert333(iter->node != NULL);

  // Is there another node beyond the iterator?
  if (iter->node->prev == NULL)
    return false;  // no

  return true;  // yes
}

bool LLIteratorPrev(LLIter iter) {
  // defensive programming
  Assert333(iter != NULL);
  Assert333(iter->list != NULL);
  Assert333(iter->node != NULL);

  if (LLIteratorHasPrev(iter)) {
    iter->node = (iter->node)->prev;
    return true;
  }

  return false;
}

void LLIteratorGetPayload(LLIter iter, void **payload) {
  // defensive programming
  Assert333(iter != NULL);
  Assert333(iter->list != NULL);
  Assert333(iter->node != NULL);

  // set the return parameter.
  *payload = iter->node->payload;
}

bool LLIteratorDelete(LLIter iter,
                      LLPayloadFreeFnPtr payload_free_function) {
  // defensive programming
  Assert333(iter != NULL);
  Assert333(iter->list != NULL);
  Assert333(iter->node != NULL);

  LinkedListNodePtr deleted = iter->node;
  void* payload_ptr = deleted->payload;
  bool isHead = iter->list->head == iter->node;
  bool isTail = iter->list->tail == iter->node;
  bool isSoleNode = (iter->list)->num_elements == 1;

  payload_free_function(payload_ptr);

  if ((!isHead && !isTail) || isSoleNode) {
    // modify necessary list pointers
    if (isSoleNode) {
      iter->list->head = NULL;
      iter->list->tail = NULL;
    } else {
      deleted->prev->next = deleted->next;
      deleted->next->prev = deleted->prev;
    }
    iter->list->num_elements--;

    // remove deleted node and progress iterator
    iter->node = deleted->next;
    free(deleted);
    return !isSoleNode;
  } else if (isHead || isTail) {
    // move iterator cursor and pop/slice
    if (isHead) {
      iter->node = iter->node->next;
      PopLinkedList(iter->list, &payload_ptr);
    } else {
      iter->node = iter->node->prev;
      SliceLinkedList(iter->list, &payload_ptr);
    }
  }
  return true;
}

bool LLIteratorInsertBefore(LLIter iter, void *payload) {
  // defensive programming
  Assert333(iter != NULL);
  Assert333(iter->list != NULL);
  Assert333(iter->node != NULL);

  // If the cursor is pointing at the head, use our
  // PushLinkedList function.
  if (iter->node == iter->list->head) {
    return PushLinkedList(iter->list, payload);
  }

  // General case: we have to do some splicing.
  LinkedListNodePtr newnode =
    (LinkedListNodePtr) malloc(sizeof(LinkedListNode));
  if (newnode == NULL)
    return false;  // out of memory

  newnode->payload = payload;
  newnode->next = iter->node;
  newnode->prev = iter->node->prev;
  newnode->prev->next = newnode;
  newnode->next->prev = newnode;
  iter->list->num_elements += 1;
  return true;
}
