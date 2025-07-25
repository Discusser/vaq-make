#include "table.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define TOMBSTONE ((void *)1)

void vmake_table_init(vmake_table *table) {
  table->capacity = 0;
  table->count = 0;
  table->entries = NULL;
}

void vmake_table_free(vmake_table *table) {
  free(table->entries);
  vmake_table_init(table);
}

void vmake_table_copy_to(vmake_table *from, vmake_table *to) {
  for (int i = 0; i < from->capacity; i++) {
    vmake_table_entry entry = from->entries[i];
    if (entry.key.type != VAL_EMPTY) {
      vmake_table_put_ptr(to, entry.key, entry.value);
    }
  }
}

bool vmake_table_put_cpy(vmake_table *table, vmake_value key, vmake_value value) {
  return vmake_table_put_ret(table, key, &value, NULL);
}

bool vmake_table_put_ptr(vmake_table *table, vmake_value key, vmake_value *value) {
  return vmake_table_put_ret(table, key, value, NULL);
}

bool vmake_table_put_ret(vmake_table *table, vmake_value key, vmake_value *value,
                         vmake_value **inserted) {
  if (table->count + 1 > table->capacity * VMAKE_TABLE_LOAD_FACTOR) {
    int capacity = table->capacity < 8 ? 8 : table->capacity * 2;
    vmake_table_resize(table, capacity);
  }

  vmake_table_entry *entry = vmake_table_find_entry(table->entries, table->capacity, key);
  bool is_new_key = entry->key.type == VAL_EMPTY;
  // Only increment the count if the entry is not a tombstone entry, because
  // removing entries does not decrease the count
  if (is_new_key && entry->value == NULL)
    table->count++;

  entry->key = key;
  entry->value = malloc(sizeof(vmake_value));
  if (value == NULL || value == TOMBSTONE) {
    entry->value = value;
  } else {
    *entry->value = *value;
  }

  if (inserted)
    *inserted = entry->value;

  return is_new_key;
}

bool vmake_table_remove(vmake_table *table, vmake_value key) {
  if (table->count == 0)
    return false;

  vmake_table_entry *entry = vmake_table_find_entry(table->entries, table->capacity, key);
  if (entry->key.type == VAL_EMPTY)
    return false;

  entry->key = vmake_value_empty();
  // 'Tombstone' value to indicate that there used to be a value at this entry
  entry->value = TOMBSTONE;

  return true;
}

bool vmake_table_get(vmake_table *table, vmake_value key, vmake_value **value) {
  if (table->count == 0)
    return false;

  vmake_table_entry *entry = vmake_table_find_entry(table->entries, table->capacity, key);
  if (entry->key.type == VAL_EMPTY)
    return false;

  if (value != NULL)
    *value = entry->value;
  return true;
}

bool vmake_table_has(vmake_table *table, vmake_value key) {
  return vmake_table_get(table, key, NULL);
}

vmake_table_entry *vmake_table_find_entry(vmake_table_entry *entries, int capacity,
                                          vmake_value key) {
  if (capacity == 0)
    return NULL;

  assert(key.type != VAL_EMPTY);

  // This only works instead of the modulo operator because we know that capacity is a power of 2
  int index = vmake_value_hash(key) & (capacity - 1);
  vmake_table_entry *tombstone = NULL;

  while (true) {
    vmake_table_entry *entry = &entries[index];
    if (entry->key.type == VAL_EMPTY) {
      if (entry->value == NULL) {
        return tombstone == NULL ? entry : tombstone;
      } else if (entry->value == TOMBSTONE && tombstone == NULL) {
        tombstone = entry;
      }
    } else if (vmake_value_equals(entry->key, key)) {
      return entry;
    }

    index = (index + 1) & (capacity - 1);
  }
}

vmake_obj_string *vmake_table_find_string(vmake_table *table, const char *chars, int length,
                                          uint32_t hash) {
  if (table->count == 0)
    return NULL;

  // NOTE: Is it possible for the table to be full of tombstones, making this
  // loop infinite?
  int index = hash & (table->capacity - 1);

  while (true) {
    vmake_table_entry *entry = &table->entries[index];
    if (entry->key.type == VAL_EMPTY) {
      // If the entry is not a tombstone and is empty, that means we haven't
      // found anything.
      if (entry->value == NULL) {
        return NULL;
      }
    } else if (vmake_value_is_string(entry->key)) {
      vmake_obj_string *str = (vmake_obj_string *)entry->key.as.obj;
      if (str->length == length && str->hash == hash && memcmp(str->chars, chars, length) == 0) {
        return str;
      }
    }

    index = (index + 1) & (table->capacity - 1);
  }
}

void vmake_table_resize(vmake_table *table, int new_capacity) {
  vmake_table_entry *entries = malloc(sizeof(vmake_table_entry) * new_capacity);
  for (int i = 0; i < new_capacity; i++) {
    entries[i].key = vmake_value_empty();
    entries[i].value = NULL;
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    vmake_table_entry entry = table->entries[i];
    if (entry.key.type == VAL_EMPTY)
      continue;

    vmake_table_entry *new_entry = vmake_table_find_entry(entries, new_capacity, entry.key);
    new_entry->key = entry.key;
    new_entry->value = entry.value;
    table->count++;
  }

  free(table->entries);
  table->entries = entries;
  table->capacity = new_capacity;
}
