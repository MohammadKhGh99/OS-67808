//
// Created by ameer on 5/28/2021.
//
#include "hashmap.h"
#include <stdbool.h>
#include <stdlib.h>
#include "string.h"


/**
 * Allocates dynamically new hash map element.
 * @param func a function which "hashes" keys.
 * @return pointer to dynamically allocated hashmap.
 * @if_fail return NULL.
 */
hashmap *hashmap_alloc(hash_func func)
{
	if (func == NULL)
	{
		return NULL;
	}
	hashmap *hashmap1 = malloc(sizeof(hashmap));
	if (hashmap1 == NULL)
	{
		return NULL;
	}
	hashmap1->size = 0;
	hashmap1->capacity = HASH_MAP_INITIAL_CAP;
	hashmap1->hash_func = func;
	hashmap1->buckets = malloc(HASH_MAP_INITIAL_CAP * sizeof(vector));
	if (hashmap1->buckets == NULL)
	{
		free(hashmap1);
		hashmap1 = NULL;
		return NULL;
	}
	for (size_t i = 0; i < hashmap1->capacity; ++i)
	{
		hashmap1->buckets[i] = vector_alloc(pair_copy, pair_cmp, pair_free);
		if (hashmap1->buckets[i] == NULL)
		{
			return NULL;
		}
	}
	for (size_t j = 0; j < hashmap1->capacity; ++j)
	{
		for (size_t k = 0; k < hashmap1->buckets[j]->capacity; ++k)
		{
			hashmap1->buckets[j]->data[k] = NULL;
		}
	}
	return hashmap1;
}


/**
 * Frees a hash map and the elements the hash map itself allocated.
 * @param p_hash_map pointer to dynamically allocated pointer to hash_map.
 */
void hashmap_free(hashmap **p_hash_map)
{
	if ((p_hash_map == NULL) || (*p_hash_map) == NULL)
	{
		return;
	}
	for (size_t i = 0; i < (*p_hash_map)->capacity; ++i)
	{
		vector_free(&((*p_hash_map)->buckets[i]));
	}
	free((*p_hash_map)->buckets);
	free((*p_hash_map));
}


bool find_key(hashmap *hashmap1, keyT key)
{
	if (hashmap1 == NULL)
	{
		return NULL;
	}
	if (hashmap_at(hashmap1, key) != NULL)
	{
		return true;
	}
	return false;
}

void rehashing(hashmap *hash_map, char *direction)
{
	hashmap *new_hashmap = malloc(sizeof(hashmap) * hash_map->capacity);
	if (new_hashmap == NULL)
		return;
	new_hashmap->buckets = malloc(sizeof(vector) * hash_map->capacity);
	if (new_hashmap->buckets == NULL)
		return;
	new_hashmap->capacity = hash_map->capacity;
	if (strcmp(direction, "down") == 0)
		hash_map->capacity /= HASH_MAP_GROWTH_FACTOR;
	else
		hash_map->capacity *= HASH_MAP_GROWTH_FACTOR;

	for (size_t i = 0; i < new_hashmap->capacity; ++i)
	{
		new_hashmap->buckets[i] = vector_alloc(pair_copy, pair_cmp, pair_free);
	}
	for (size_t i = 0; i < new_hashmap->capacity; ++i)
	{
		if (hash_map->buckets[i] == NULL)
		{
			continue;
		}
		for (size_t j = 0; j < hash_map->buckets[i]->size; ++j)
		{
			vector_push_back(new_hashmap->buckets[i],
					hash_map->buckets[i]->data[j]);
		}
	}
	for (size_t i = 0; i < hash_map->capacity; ++i)
	{
		free(hash_map->buckets[i]);
	}
	for (size_t i = 0; i < hash_map->capacity; ++i)
	{
		hash_map->buckets[i] = vector_alloc(pair_copy, pair_cmp, pair_free);
	}
	hash_map->size = 0;
	for (size_t i = 0; i < new_hashmap->capacity; ++i)
	{
		for (size_t j = 0; j < new_hashmap->buckets[i]->size; ++j)
		{
			hashmap_insert(hash_map, new_hashmap->buckets[i]->data[j]);
		}
	}
	hashmap_free(&new_hashmap);
}



/**
 * Inserts a new in_pair to the hash map.
 * The function inserts *new*, *copied*, *dynamically allocated* in_pair,
 * NOT the in_pair it receives as a parameter.
 * @param hash_map the hash map to be inserted with new element.
 * @param in_pair a in_pair the hash map would contain.
 * @return returns 1 for successful insertion, 0 otherwise.
 */
int hashmap_insert(hashmap *hash_map, const pair *in_pair)
{
	if (!hash_map || !in_pair)
	{
		return 0;
	}
	if (find_key(hash_map, in_pair->key))
	{
		return 0;
	}
	size_t hash_func_result = (hash_map->hash_func(in_pair->key)) &
							  (hash_map->capacity - 1);
	if (vector_push_back(hash_map->buckets[hash_func_result], in_pair))
	{
		hash_map->size++;
	}
	else
	{
		return 0;
	}
	if (hashmap_get_load_factor(hash_map) > HASH_MAP_MAX_LOAD_FACTOR)
	{

		hash_map->buckets = realloc(hash_map->buckets, hash_map->capacity *
													   sizeof(vector));
		if (hash_map->buckets == NULL)
			return 0;

		rehashing(hash_map, "up");
//		hash_map->capacity *= HASH_MAP_GROWTH_FACTOR;
	}
	return 1;
}


/**
 * The function returns the value associated with the given key.
 * @param hash_map a hash map.
 * @param key the key to be checked.
 * @return the value associated with key if exists, NULL otherwise (the value itself,
 * not a copy of it).
 */
valueT hashmap_at(const hashmap *hash_map, const_keyT key)
{
	if (!hash_map)
	{
		return NULL;
	}
	size_t hash_func_result = (hash_map->hash_func(key)) &
							  (hash_map->capacity - 1);
	if (hash_map->buckets[hash_func_result]->data[0] == NULL)
	{
		return NULL;
	}
	for (size_t i = 0; i < hash_map->buckets[hash_func_result]->capacity; ++i)
	{
		pair *pair1 = hash_map->buckets[hash_func_result]->data[i];

		if (pair1 != NULL)
		{
			if (pair1->key_cmp(key, pair1->key))
			{
				return pair1->value;
			}
		}

	}
	return NULL;
}


/**
 * The function erases the pair associated with key.
 * @param hash_map a hash map.
 * @param key a key of the pair to be erased.
 * @return 1 if the erasing was done successfully, 0 otherwise. (if key not in map,
 * considered fail).
 */
int hashmap_erase(hashmap *hash_map, const_keyT key)
{
	if (!hash_map)
	{
		return 0;
	}
	if (hashmap_at(hash_map, key) == NULL)
	{
		return 0;
	}
	size_t hash_func_result = (hash_map->hash_func(key)) &
							  (hash_map->capacity - 1);
	for (size_t i = 0; i < hash_map->buckets[hash_func_result]->size; ++i)
	{
		pair *pair1 = hash_map->buckets[hash_func_result]->data[i];
		if (pair1->key_cmp(key, pair1->key))
		{
			vector_erase(hash_map->buckets[hash_func_result], i);
			break;
		}
	}
	hash_map->size--;
	if (HASH_MAP_MIN_LOAD_FACTOR > hashmap_get_load_factor(hash_map))
	{
		hash_map->buckets = realloc(hash_map->buckets, hash_map->capacity *
													   sizeof(vector));
		if (hash_map->buckets == NULL)
			return 0;
		rehashing(hash_map, "down");
	}
	return 1;
}


/**
 * This function returns the load factor of the hash map.
 * @param hash_map a hash map.
 * @return the hash map's load factor, -1 if the function failed.
 */
double hashmap_get_load_factor(const hashmap *hash_map)
{
	if (!hash_map || hash_map->capacity <= 0)
	{
		return -1;
	}
	return ((double) hash_map->size / (double) hash_map->capacity);
}


/**
 * This function receives a hashmap and 2 functions, the first checks a condition on the keys,
 * and the seconds apply some modification on the values. The function should apply the modification
 * only on the values that are associated with keys that meet the condition.
 *
 * Example: if the hashmap maps char->int, keyT_func checks if the char is a capital letter (A-Z),
 * and val_t_func multiples the number by 2, hashmap_apply_if will change the map:
 * {('C',2),('#',3),('X',5)}, to: {('C',4),('#',3),('X',10)}, and the return value will be 2.
 * @param hash_map a hashmap
 * @param keyT_func a function that checks a condition on keyT and return 1 if true, 0 else
 * @param valT_func a function that modifies valueT, in-place
 * @return number of changed values
 */
int hashmap_apply_if(const hashmap *hash_map, keyT_func keyT_func,
					 valueT_func valT_func)
{
	int num_of_changes = 0;
	for (size_t i = 0; i < hash_map->capacity; ++i)
	{
		for (size_t j = 0; j < hash_map->buckets[i]->size; ++j)
		{
			pair *pair1 = hash_map->buckets[i]->data[j];
			if (keyT_func(pair1))
			{
				valT_func(pair1->value);
				num_of_changes++;
			}
		}
	}
	return num_of_changes;
}
