//
// Created by ameer on 5/30/2021.
//

#include "test_suite.h"
#include "pair_char_int.h"
#include "hash_funcs.h"





/**
 * This function checks the hashmap_insert function of the hashmap library.
 * If hashmap_insert fails at some points, the functions exits with exit code 1.
 */
void test_hash_map_insert(void){
	// Create Pairs.
	pair *pairs[20];
	for (int j = 0; j < 20; ++j)
	{
		char key = (char) (j + 48);
		//even keys are capital letters, odd keys are digits
		if (key % 2)
		{
			key += 17;
		}
		int value = j;
		pairs[j] = pair_alloc (&key, &value, char_key_cpy, int_value_cpy, char_key_cmp,
							   int_value_cmp, char_key_free, int_value_free);
	}
	hashmap *hashmap1 = hashmap_alloc(hash_char);
	assert(hashmap1);
	assert(hashmap1->size == 0);
	assert(hashmap_insert(hashmap1, pairs[0]) == 1);
	assert(hashmap_insert(NULL, pairs[1]) == 0);
	assert(hashmap_insert(hashmap1, pairs[1]) == 1);
	assert(hashmap1->size == 2);
	assert(hashmap_insert(hashmap1, pairs[1]) == 0);
	assert(hashmap1->capacity == HASH_MAP_INITIAL_CAP);
	for (int i = 2; i < 13; ++i)
	{
		assert(hashmap_insert(hashmap1, pairs[i]) == 1);
	}
	assert(hashmap1->size == 13);
	assert(hashmap1->capacity == 32);
}



/**
 * This function checks the hashmap_at function of the hashmap library.
 * If hashmap_at fails at some points, the functions exits with exit code 1.
 */
void test_hash_map_at(void){
	// Create Pairs.
	pair *pairs[20];
	for (int j = 0; j < 20; ++j)
	{
		char key = (char) (j + 48);
		//even keys are capital letters, odd keys are digits
		if (key % 2)
		{
			key += 17;
		}
		int value = j;
		pairs[j] = pair_alloc (&key, &value, char_key_cpy, int_value_cpy, char_key_cmp,
							   int_value_cmp, char_key_free, int_value_free);
	}
	assert(hashmap_at(NULL, pairs[10]->key) == NULL);
	hashmap *hashmap1 = hashmap_alloc(hash_char);
	assert(hashmap_at(hashmap1, pairs[0]->key) == NULL);
	hashmap_insert(hashmap1, pairs[0]);
	assert(pairs[0]->value_cmp(hashmap_at(hashmap1, pairs[0]->key),
							pairs[0]->value));
	for (int i = 1; i < 12; ++i)
	{
		hashmap_insert(hashmap1, pairs[i]);
	}
	assert(pairs[11]->value_cmp(hashmap_at(hashmap1, pairs[11]->key),
									 pairs[11]->value));
}



/**
 * This function checks the hashmap_erase function of the hashmap library.
 * If hashmap_erase fails at some points, the functions exits with exit code 1.
 */
void test_hash_map_erase(void){
	// Create Pairs.
	pair *pairs[20];
	for (int j = 0; j < 20; ++j)
	{
		char key = (char) (j + 48);
		//even keys are capital letters, odd keys are digits
		if (key % 2)
		{
			key += 17;
		}
		int value = j;
		pairs[j] = pair_alloc (&key, &value, char_key_cpy, int_value_cpy, char_key_cmp,
							   int_value_cmp, char_key_free, int_value_free);
	}
	hashmap *hashmap1 = hashmap_alloc(hash_char);
	assert(hashmap_erase(NULL, pairs[0]) == 0);
	for (int i = 0; i < 14; ++i)
	{
		hashmap_insert(hashmap1, pairs[i]);
	}
	assert(hashmap_erase(hashmap1, pairs[10]->key) == 1);
	assert(hashmap_erase(hashmap1, pairs[10]->key) == 0);
	assert(hashmap1->size == 13);
	assert(hashmap1->capacity == 32);
	for (int i = 0; i < 6; ++i)
	{
		assert(hashmap_erase(hashmap1, pairs[i]->key) == 1);
	}
	assert(hashmap1->size == 7);
	assert(hashmap1->capacity == HASH_MAP_INITIAL_CAP);
}



/**
 * This function checks the hashmap_get_load_factor function of the hashmap library.
 * If hashmap_get_load_factor fails at some points, the functions exits with exit code 1.
 */
void test_hash_map_get_load_factor(void){
	// Create Pairs.
	pair *pairs[20];
	for (int j = 0; j < 20; ++j)
	{
		char key = (char) (j + 48);
		//even keys are capital letters, odd keys are digits
		if (key % 2)
		{
			key += 17;
		}
		int value = j;
		pairs[j] = pair_alloc (&key, &value, char_key_cpy, int_value_cpy, char_key_cmp,
							   int_value_cmp, char_key_free, int_value_free);
	}
	assert(hashmap_get_load_factor(NULL) == -1);
	hashmap *hashmap1 = hashmap_alloc(hash_char);
	hashmap_insert(hashmap1, pairs[15]);
	assert(hashmap_get_load_factor(hashmap1) == ((float)1 / (float)16));
	for (int i = 0; i < 12; ++i)
	{
		hashmap_insert(hashmap1, pairs[i]);
	}
	assert(hashmap_get_load_factor(hashmap1) == (float)13 / (float)32);
}






void *elem_cpy (const void *elem)
{
	int *a = malloc (sizeof (int));
	*a = *((int *) elem);
	return a;
}
int elem_cmp (const void *elem_1, const void *elem_2)
{
	return *((const int *) elem_1) == *((const int *) elem_2);
}
void elem_free (void **elem)
{
	free (*elem);
}


int main()
{
	test_hash_map_insert();
	test_hash_map_at();
	test_hash_map_erase();
	test_hash_map_get_load_factor();
}