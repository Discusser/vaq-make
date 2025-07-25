#pragma once

#include "value.h"
#include <stdint.h>

#define VMAKE_TABLE_LOAD_FACTOR 0.5

typedef struct vmake_obj_string vmake_obj_string;

typedef struct vmake_table_entry {
  vmake_value key;
  vmake_value *value;
} vmake_table_entry;

typedef struct vmake_table {
  int count;
  int capacity;
  vmake_table_entry *entries;
} vmake_table;

// Initializes a vmake_table pointer
void vmake_table_init(vmake_table *table);
// Frees a vmake_table pointer.
void vmake_table_free(vmake_table *table);
// Copies the contents of `from` into `to`
void vmake_table_copy_to(vmake_table *from, vmake_table *to);
// Puts an entry with the given key and value into the table, expanding the
// table if necessary. `key` should not be NULL. Returns true if the key didn't
// exist previously in the table, or false if it did, that is, the return value
// indicates if this key is a new key.
bool vmake_table_put_cpy(vmake_table *table, vmake_value key, vmake_value value);
bool vmake_table_put_ptr(vmake_table *table, vmake_value key, vmake_value *value);
bool vmake_table_put_ret(vmake_table *table, vmake_value key, vmake_value *value,
                         vmake_value **inserted);
// Removes an entry with the given key from the hash table. Returns true if the
// entry was removed, or false if there was no entry with the given key.
bool vmake_table_remove(vmake_table *table, vmake_value key);
// Retrieves the value associated with a key. The value is stored in the `value`
// pointer passed to the function. Returns true if the key exists in the table,
// or false if it doesn't, in which case `value` is unmodified. If `value` is
// NULL, it is also unmodified.
bool vmake_table_get(vmake_table *table, vmake_value key, vmake_value **value);
// Returns true if the key exists in the table, or false if it doesn't. This is
// equivalent to vmake_table_get(table, key, NULL)
bool vmake_table_has(vmake_table *table, vmake_value key);
// Finds the entry where a key should go, given a list of entries and a
// capacity. `key` should not be NULL
vmake_table_entry *vmake_table_find_entry(vmake_table_entry *entries, int capacity,
                                          vmake_value key);
// Finds the key containing the given string. The hash is required for faster
// comparison, since two strings different hashes are certainly not equal. This
// only exists to 'bootstrap' lox_hash_table_find_entry by deduplicating
// strings.
vmake_obj_string *vmake_table_find_string(vmake_table *table, const char *chars, int length,
                                          uint32_t hash);
// Resizes a hash table to the given size.
void vmake_table_resize(vmake_table *table, int new_capacity);
