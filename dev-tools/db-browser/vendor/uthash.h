/*
 * vendor/uthash.h
 *
 * Minimal string-keyed hash table macros for C structs.
 *
 * Based on the interface of uthash by Troy D. Hanson (BSD license).
 * This file provides the core macros used in this project:
 *
 *   HASH_ADD_STR(head, keyfield, item)
 *   HASH_FIND_STR(head, findstr, out)
 *   HASH_DEL(head, delptr)
 *   HASH_ITER(hh, head, el, tmp)
 *   HASH_CLEAR(hh, head)
 *   HASH_COUNT(head)
 *
 * Usage:
 *   Add a `UT_hash_handle hh;` field and a char * key field to your struct.
 *   The item's key field must be a char* and must remain valid for the
 *   lifetime of the item in the table.
 *
 * Example:
 *   typedef struct { char *name; int value; UT_hash_handle hh; } Entry;
 *   Entry *table = NULL;
 *   Entry *e = malloc(sizeof *e); e->name = strdup("foo"); e->value = 42;
 *   HASH_ADD_STR(table, name, e);
 *   Entry *found = NULL;
 *   HASH_FIND_STR(table, "foo", found);
 */

#ifndef UTHASH_H
#define UTHASH_H

#include <stdlib.h>
#include <string.h>

/* ---- Internal structures ------------------------------------------- */

#define UTHASH_NBUCKETS 64u   /* must be power of 2 */

typedef struct UT_hash_handle {
    struct UT_hash_handle *next;   /* next in bucket chain      */
    struct UT_hash_handle *prev;   /* prev in global item list  */
    struct UT_hash_handle *hh_next;/* next in global item list  */
    void                  *tbl;   /* back-pointer to UT_hash_table  */
    const char            *key;   /* pointer to item's key field    */
} UT_hash_handle;

typedef struct UT_hash_table {
    UT_hash_handle *buckets[UTHASH_NBUCKETS]; /* bucket heads    */
    UT_hash_handle *ht_head; /* head of ordered insertion list     */
    UT_hash_handle *ht_tail; /* tail of ordered insertion list     */
    unsigned        count;   /* total items                        */
} UT_hash_table;

/* ---- Internal helpers (not for direct use) ------------------------- */

static inline unsigned _uthash_strhash(const char *s) {
    unsigned h = 5381u;
    while (*s) h = ((h << 5) + h) + (unsigned char)*s++;
    return h & (UTHASH_NBUCKETS - 1u);
}

/* Return handle pointer embedded inside a user struct, given:
 *   item     - pointer to the user struct
 *   hh_ofs   - offsetof(userstruct, hh)
 */
#define _UH_HH(item, hh_ofs)  \
    ((UT_hash_handle*)((char*)(item) + (hh_ofs)))

/* Return a pointer to the table from the head pointer */
#define _UH_TBL(head, hh_ofs)  \
    ((UT_hash_table*)_UH_HH(head, hh_ofs)->tbl)

/* ---- Public macros -------------------------------------------------- */

/*
 * HASH_ADD_STR(head, keyfield, item)
 *   head     - pointer to the head of the hash table (NULL if empty)
 *   keyfield - name of the char* key member in the struct
 *   item     - pointer to the struct to add
 */
#define HASH_ADD_STR(head, keyfield, item) do {                             \
    size_t _hh_ofs = (size_t)((char*)&((item)->hh) - (char*)(item));       \
    UT_hash_handle *_hh = _UH_HH(item, _hh_ofs);                           \
    _hh->key = (item)->keyfield;                                            \
    _hh->next = NULL;                                                       \
    _hh->prev = NULL;                                                       \
    _hh->hh_next = NULL;                                                    \
    if (!(head)) {                                                          \
        UT_hash_table *_t = (UT_hash_table*)calloc(1, sizeof(UT_hash_table));\
        if (!_t) break;                                                     \
        _hh->tbl = _t;                                                      \
        _t->ht_head = _hh; _t->ht_tail = _hh; _t->count = 1;               \
        unsigned _bkt = _uthash_strhash(_hh->key);                          \
        _t->buckets[_bkt] = _hh;                                            \
        (head) = (item);                                                    \
    } else {                                                                \
        UT_hash_table *_t = _UH_TBL(head, _hh_ofs);                        \
        _hh->tbl = _t;                                                      \
        unsigned _bkt = _uthash_strhash(_hh->key);                         \
        _hh->next = _t->buckets[_bkt];                                      \
        _t->buckets[_bkt] = _hh;                                            \
        _t->ht_tail->hh_next = _hh;                                         \
        _hh->prev = _t->ht_tail;                                            \
        _t->ht_tail = _hh;                                                  \
        _t->count++;                                                        \
    }                                                                       \
} while (0)

/*
 * HASH_FIND_STR(head, findstr, out)
 *   head    - pointer to the head of the hash table
 *   findstr - the string key to look up
 *   out     - set to pointer to found struct, or NULL if not found
 */
#define HASH_FIND_STR(head, findstr, out) do {                              \
    (out) = NULL;                                                           \
    if (head) {                                                             \
        size_t _hh_ofs = (size_t)((char*)&((head)->hh) - (char*)(head));   \
        UT_hash_table *_t = _UH_TBL(head, _hh_ofs);                        \
        unsigned _bkt = _uthash_strhash(findstr);                           \
        UT_hash_handle *_cur = _t->buckets[_bkt];                           \
        while (_cur) {                                                      \
            if (_cur->key && strcmp(_cur->key, (findstr)) == 0) {           \
                (out) = (void*)((char*)_cur - _hh_ofs);                     \
                break;                                                      \
            }                                                               \
            _cur = _cur->next;                                              \
        }                                                                   \
    }                                                                       \
} while (0)

/*
 * HASH_DEL(head, delptr)
 *   head   - pointer to the head of the hash table (may be updated)
 *   delptr - pointer to the struct to remove
 * Does NOT free the struct itself.
 */
#define HASH_DEL(head, delptr) do {                                         \
    if ((head) && (delptr)) {                                               \
        size_t _hh_ofs = (size_t)((char*)&((head)->hh) - (char*)(head));   \
        UT_hash_handle *_dhh = _UH_HH(delptr, _hh_ofs);                    \
        UT_hash_table  *_t   = (UT_hash_table*)_dhh->tbl;                  \
        unsigned _bkt = _uthash_strhash(_dhh->key);                        \
        /* remove from bucket chain */                                      \
        UT_hash_handle **_pp = &_t->buckets[_bkt];                          \
        while (*_pp && *_pp != _dhh) _pp = &(*_pp)->next;                  \
        if (*_pp) *_pp = _dhh->next;                                        \
        /* remove from ordered list */                                      \
        if (_dhh->prev) _dhh->prev->hh_next = _dhh->hh_next;               \
        else            _t->ht_head = _dhh->hh_next;                       \
        if (_dhh->hh_next) {                                                \
            ((UT_hash_handle*)_dhh->hh_next)->prev = _dhh->prev;            \
        } else {                                                            \
            _t->ht_tail = _dhh->prev;                                       \
        }                                                                   \
        _t->count--;                                                        \
        if (_t->count == 0) { free(_t); (head) = NULL; }                   \
        else if ((head) == (delptr)) {                                      \
            (head) = (void*)((char*)_t->ht_head - _hh_ofs);                \
        }                                                                   \
    }                                                                       \
} while (0)

/*
 * HASH_ITER(hh, head, el, tmp)
 *   Iterate over all items in insertion order. Safe for deletions during loop.
 *   el  - loop variable (pointer to user struct)
 *   tmp - temporary variable of same type
 */
#define HASH_ITER(hh, head, el, tmp) \
    for ((el) = (head),                                                         \
         (tmp) = ((el) ? (void*)((char*)_UH_HH(el,                             \
                   (size_t)((char*)&((el)->hh)-(char*)(el)))->hh_next          \
                   ? (char*)_UH_HH(el,                                         \
                       (size_t)((char*)&((el)->hh)-(char*)(el)))->hh_next      \
                       - (size_t)((char*)&((el)->hh)-(char*)(el)) : NULL)      \
                 : NULL);                                                       \
         (el) != NULL;                                                          \
         (el) = (void*)(tmp),                                                   \
         (tmp) = ((el) ? (void*)((char*)_UH_HH(el,                             \
                   (size_t)((char*)&((el)->hh)-(char*)(el)))->hh_next          \
                   ? (char*)_UH_HH(el,                                         \
                       (size_t)((char*)&((el)->hh)-(char*)(el)))->hh_next      \
                       - (size_t)((char*)&((el)->hh)-(char*)(el)) : NULL)      \
                 : NULL))

/*
 * HASH_CLEAR(hh, head)
 *   Remove all entries from the table, freeing internal table state.
 *   Does NOT free the individual structs themselves.
 */
#define HASH_CLEAR(hh, head) do {                                           \
    if (head) {                                                             \
        size_t _hh_ofs = (size_t)((char*)&((head)->hh) - (char*)(head));   \
        UT_hash_table *_t = _UH_TBL(head, _hh_ofs);                        \
        free(_t);                                                           \
        (head) = NULL;                                                      \
    }                                                                       \
} while (0)

/*
 * HASH_COUNT(head)
 *   Returns the number of items in the table (0 if head is NULL).
 */
#define HASH_COUNT(head) \
    ((head) ? ((UT_hash_table*)_UH_HH(head,                                 \
        (size_t)((char*)&((head)->hh)-(char*)(head)))->tbl)->count : 0u)

#endif /* UTHASH_H */
