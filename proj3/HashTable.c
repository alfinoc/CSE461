/*
 * Copyright 2011 Steven Gribble
 *
 *  This file is part of the UW CSE 333 course project sequence
 *  (333proj).
 *
 *  333proj is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  333proj is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with 333proj.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <semaphore.h>
#include "HashTable.h"
#include "HashTable_priv.h"

// Searches LinkedList for HTKeyValue with key equal to uint64_t.
// See function definition for full specification.
static int find(LinkedList, uint64_t, bool, HTKeyValuePtr, HTKeyValuePtr*);

// a free function that does nothing
static void NullFree(void *freeme) { }

// A private utility function to grow the hashtable (increase
// the number of buckets) if its load factor has become too high.
static void ResizeHashtable(HashTable ht);

HashTable AllocateHashTable(uint32_t num_buckets) {
  HashTable ht;
  uint32_t  i;

  // defensive programming
  if (num_buckets == 0) {
    return NULL;
  }

  // allocate the hash table record
  ht = (HashTable) malloc(sizeof(HashTableRecord));
  if (ht == NULL) {
    return NULL;
  }

  //initialize synchronization
  sem_init(&ht->wrt, 0, 1);
  sem_init(&ht->mutex, 0, 1);
  ht->readcount = 0;

  // initialize the record
  ht->num_buckets = num_buckets;
  ht->num_elements = 0;
  ht->buckets =
    (LinkedList *) malloc(num_buckets * sizeof(LinkedList));
  if (ht->buckets == NULL) {
    // make sure we don't leak!
    free(ht);
    return NULL;
  }
  for (i = 0; i < num_buckets; i++) {
    ht->buckets[i] = AllocateLinkedList();
    if (ht->buckets[i] == NULL) {
      // allocating one of our bucket chain lists failed,
      // so we need to free everything we allocated so far
      // before returning NULL to indicate failure.  Since
      // we know the chains are empty, we'll pass in a
      // free function pointer that does nothing; it should
      // never be called.
      uint32_t j;
      for (j = 0; j < i; j++) {
        FreeLinkedList(ht->buckets[j], NullFree);
      }
      free(ht);
      return NULL;
    }
  }
  return (HashTable) ht;
}

void FreeHashTable(HashTable table,
                   ValueFreeFnPtr value_free_function) {
  uint32_t i;

  // loop through and free the chains on each bucket
  for (i = 0; i < table->num_buckets; i++) {
    LinkedList  bl = table->buckets[i];
    HTKeyValue *nextKV;

    PopLinkedList(bl, (void **) &nextKV);
    // pop elements off the the chain list, then free the list
    while (NumElementsInLinkedList(bl) > 0) {
      value_free_function(nextKV->value);
      free(nextKV);
    }
    // the chain list is empty, so we can pass in the
    // null free function to FreeLinkedList.
    FreeLinkedList(bl, NullFree);
  }

  // free the bucket array within the table record,
  // then free the table record itself.
  free(table->buckets);
  free(table);
}

uint64_t NumElementsInHashTable(HashTable table) {
  return table->num_elements;
}

uint64_t FNVHash64(unsigned char *buffer, unsigned int len) {
  // This code is adapted from code by Landon Curt Noll
  // and Bonelli Nicola:
  //
  // http://code.google.com/p/nicola-bonelli-repo/
  static const uint64_t FNV1_64_INIT = 0xcbf29ce484222325ULL;
  static const uint64_t FNV_64_PRIME = 0x100000001b3ULL;
  unsigned char *bp = (unsigned char *) buffer;
  unsigned char *be = bp + len;
  uint64_t hval = FNV1_64_INIT;

  /*
   * FNV-1a hash each octet of the buffer
   */
  while (bp < be) {
    /* xor the bottom with the current octet */
    hval ^= (uint64_t) * bp++;
    /* multiply by the 64 bit FNV magic prime mod 2^64 */
    hval *= FNV_64_PRIME;
  }
  /* return our new hash value */
  return hval;
}

uint64_t FNVHashInt64(uint64_t hashme) {
  unsigned char buf[8];
  int i;

  for (i = 0; i < 8; i++) {
    buf[i] = (unsigned char) (hashme & 0x00000000000000FFULL);
    hashme >>= 8;
  }
  return FNVHash64(buf, 8);
}

uint64_t HashKeyToBucketNum(HashTable ht, uint64_t key) {
  return key % ht->num_buckets;
}

// Searches for an HTKeyValue pair stored in 'list' with a key equal to
// 'targetkey'. If 'remove' is true, the element will be removed from the list,
// the HTKeyValue freed, and the dereferenced value of 'found' set to the value
// of the found HTKeyValue. Otherwise, the dereferenced value of 'foundPtr' is
// set to the found HTKeyValue in memory, and list is left unmodified.
// Returns 1 if found, 0 if not found, -1 if memory error.
static int find(LinkedList list, uint64_t targetKey, bool remove,
                HTKeyValuePtr found, HTKeyValuePtr* foundPtr) {
  LLIter iter = LLMakeIterator(list, 0);
  if (iter == NULL && NumElementsInLinkedList(list) > 0)
    return -1;  // memory error
  if (iter == NULL)
    return 0;  // empty list

  bool next = true;
  while (next) {
    HTKeyValuePtr payload;
    LLIteratorGetPayload(iter, (void**) &payload);
    if ((payload->key == targetKey)) {
      // target found in list
      if (found != NULL)
        *found = *payload;
      if (remove) {
        free(payload);
        LLIteratorDelete(iter, NullFree);
      }
      LLIteratorFree(iter);
      if (foundPtr != NULL)
        *foundPtr = payload;
      return 1;
    }
    next = LLIteratorNext(iter);
  }
  // target not found in list
  LLIteratorFree(iter);
  return 0;
}

int InsertHashTable(HashTable table,
                    HTKeyValue newkeyvalue,
                    HTKeyValue *oldkeyvalue) {
  uint32_t insertBucket;
  LinkedList insertChain;

  int ret = 0;
  //Lock for writing!
  sem_wait(&table->wrt);

  ResizeHashtable(table);

  // calculate which bucket we're inserting into,
  // grab its linked list chain
  insertBucket = HashKeyToBucketNum(table, newkeyvalue.key);
  insertChain = table->buckets[insertBucket];

  HTKeyValuePtr found;
  int success = find(insertChain, newkeyvalue.key, false, NULL, &found);
  if (success == -1) {
    // out of memory
    ret = 0;
    goto cleanup;
  }

  if (success == 0) {
    // inserting this key for the first time
    table->num_elements++;
    HTKeyValuePtr newEntry = (HTKeyValuePtr) malloc(sizeof(HTKeyValue));
    if (newEntry == NULL) {
      // out of memory
      ret = 0;
      goto cleanup;
    }

    *newEntry = newkeyvalue;
    ret = PushLinkedList(insertChain, newEntry);
    goto cleanup;
  } else {
    // replace value for keyvalue already in HT
    *oldkeyvalue = *found;
    found->value = newkeyvalue.value;
    ret = 2;
    goto cleanup;
  }

 cleanup:
  sem_post(&table->wrt);
  return ret;
}

int LookupHashTable(HashTable table,
                    uint64_t key,
                    HTKeyValue *keyvalue) {
  int ret;

  sem_wait(&table->mutex);
  table->readcount++;
  if (table->readcount == 1)
    sem_wait(&table->wrt);
  sem_post(&table->mutex);

  uint64_t lookupBucket = HashKeyToBucketNum(table, key);
  LinkedList lookupChain = table->buckets[lookupBucket];

  ret = find(lookupChain, key, false, keyvalue, NULL);

  sem_wait(&table->mutex);
  table->readcount--;
  if (table->readcount == 0)
    sem_post(&table->wrt);
  sem_post(&table->mutex);

  return ret;
}

int RemoveFromHashTable(HashTable table,
                        uint64_t key,
                        HTKeyValue *keyvalue) {
  sem_wait(&table->wrt);

  uint64_t removeBucket = HashKeyToBucketNum(table, key);
  LinkedList removeChain = table->buckets[removeBucket];

  int success = find(removeChain, key, true, keyvalue, NULL);
  if (success == 1)
    table->num_elements--;

  sem_post(&table->wrt);

  return success;
}

HTIter HashTableMakeIterator(HashTable table) {
  HTIterRecord *iter;
  uint32_t      i;

  // malloc the iterator
  iter = (HTIterRecord *) malloc(sizeof(HTIterRecord));
  if (iter == NULL) {
    return NULL;
  }

  // if the hash table is empty, the iterator is immediately invalid,
  // since it can't point to anything.
  if (table->num_elements == 0) {
    iter->is_valid = false;
    iter->ht = table;
    iter->bucket_it = NULL;
    return iter;
  }

  // initialize the iterator.  there is at least one element in the
  // table, so find the first element and point the iterator at it.
  iter->is_valid = true;
  iter->ht = table;
  for (i = 0; i < table->num_buckets; i++) {
    if (NumElementsInLinkedList(table->buckets[i]) > 0) {
      iter->bucket_num = i;
      break;
    }
  }
  iter->bucket_it = LLMakeIterator(table->buckets[iter->bucket_num], 0UL);
  if (iter->bucket_it == NULL) {
    // out of memory!
    free(iter);
    return NULL;
  }
  return iter;
}

void HTIteratorFree(HTIter iter) {
  if (iter->bucket_it != NULL) {
    LLIteratorFree(iter->bucket_it);
    iter->bucket_it = NULL;
  }
  iter->is_valid = false;
  free(iter);
}

int HTIteratorNext(HTIter iter) {
  if (!iter->is_valid)
    return 0;

  // progress to next element in ll if possible
  if (iter != NULL && LLIteratorHasNext(iter->bucket_it))
    return LLIteratorNext(iter->bucket_it);

  // search until finds non-empty chain or invalid bucket
  LLIter oldListIter = iter->bucket_it;
  LinkedList nextChain;
  do {
    iter->bucket_num++;
    if (iter->bucket_num >= iter->ht->num_buckets) {
      iter->is_valid = false;
      return 0;
    }
    nextChain = iter->ht->buckets[iter->bucket_num];
  } while (NumElementsInLinkedList(nextChain) == 0);

  // reset list iterator
  LLIteratorFree(oldListIter);
  iter->bucket_it = LLMakeIterator(nextChain, 0UL);
  return 1;
}

int HTIteratorPastEnd(HTIter iter) {
  return !iter->is_valid || iter->ht->num_elements == 0;
}

int HTIteratorGet(HTIter iter, HTKeyValue *keyvalue) {
  if (!iter->is_valid || iter->ht->num_elements == 0)
    return 0;

  HTKeyValuePtr payload;
  LLIteratorGetPayload(iter->bucket_it, (void**) &payload);
  *keyvalue = *payload;
  return 1;
}

int HTIteratorDelete(HTIter iter, HTKeyValue *keyvalue) {
  HTKeyValue kv;
  int res, retval;

  // Try to get what the iterator is pointing to.
  res = HTIteratorGet(iter, &kv);
  if (res == 0)
    return 0;

  // Advance the iterator.
  res = HTIteratorNext(iter);
  if (res == 0) {
    retval = 2;
  } else {
    retval = 1;
  }
  res = RemoveFromHashTable(iter->ht, kv.key, keyvalue);
 
  return retval;
}

static void ResizeHashtable(HashTable ht) {
  // Resize if the load factor is > 3.
  if (ht->num_elements < 3 * ht->num_buckets)
    return;

  // This is the resize case.  Allocate a new hashtable,
  // iterate over the old hashtable, do the surgery on
  // the old hashtable record and free up the new hashtable
  // record.
  HashTable newht = AllocateHashTable(ht->num_buckets * 9);

  // Give up if out of memory.
  if (newht == NULL)
    return;

  // Loop through the old ht with an iterator,
  // inserting into the new HT.
  HTIter it = HashTableMakeIterator(ht);
  if (it == NULL) {
    // Give up if out of memory.
    FreeHashTable(newht, &NullFree);
    return;
  }

  while (!HTIteratorPastEnd(it)) {
    HTKeyValue item, dummy;
    if (InsertHashTable(newht, item, &dummy) != 1) {
      // failure, free up everything, return.
      HTIteratorFree(it);
      FreeHashTable(newht, &NullFree);
      return;
    }
    HTIteratorNext(it);
  }

  // Worked!  Free the iterator.
  HTIteratorFree(it);

  // Sneaky: swap the structures, then free the new table,
  // and we're done.
  {
    HashTableRecord tmp;

    tmp = *ht;
    *ht = *newht;
    *newht = tmp;
    FreeHashTable(newht, &NullFree);
  }
  return;
}
