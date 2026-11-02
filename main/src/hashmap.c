#include "hashmap.h"

HashMap init_hashmap() {
  HashMap map = {0};
  map.put = hashmap_put;
  map.get = hashmap_get;
  return map;
}

unsigned int hash(const char *key) {
  unsigned int h = 5381;
  while (*key)
    h = ((h << 5) + h) + *key++;
  return h % TABLE_SIZE;
}

void hashmap_put(HashMap *map, const char *key, int value) {
  unsigned int index = hash(key);
  Entry *entry = map->buckets[index];

  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      entry->value = value;
      return;
    }
    entry = entry->next;
  }

  entry = calloc(1, sizeof(Entry));
  entry->key = strdup(key);
  entry->value = value;

  entry->next = map->buckets[index];
  map->buckets[index] = entry;
}

int hashmap_get(HashMap *map, const char *key, int *value) {
  unsigned int index = hash(key);

  Entry *entry = map->buckets[index];

  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      *value = entry->value;
      return 1;
    }
    entry = entry->next;
  }

  return 0;
}