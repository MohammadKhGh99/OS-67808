//
// Created by ameer on 5/24/2021.
//

#include "vector.h"
#include <string.h>
#include <stdbool.h>


#define NOT_IN_VECTOR -1



/**
 * Dynamically allocates a new vector.
 * @param elem_copy_func func which copies the element stored in the vector (returns
 * dynamically allocated copy).
 * @param elem_cmp_func func which is used to compare elements stored in the vector.
 * @param elem_free_func func which frees elements stored in the vector.
 * @return pointer to dynamically allocated vector.
 * @if_fail return NULL.
 */
vector *vector_alloc(vector_elem_cpy elem_copy_func, vector_elem_cmp elem_cmp_func,
					 vector_elem_free elem_free_func){
	vector *vector1 = malloc(sizeof(vector));
	if(vector1 == NULL || elem_cmp_func == NULL || elem_copy_func == NULL ||
	elem_free_func == NULL)
	{
		return NULL;
	}
	vector1->capacity = VECTOR_INITIAL_CAP;
	vector1->size = 0;
	vector1->elem_copy_func = elem_copy_func;
	vector1->elem_cmp_func = elem_cmp_func;
	vector1->elem_free_func = elem_free_func;
	vector1->data = malloc(vector1->capacity * sizeof(void *));
	if(vector1->data == NULL)
	{
		free(vector1);
		return NULL;
	}
	return vector1;
}



/**
 * Frees a vector and the elements the vector itself allocated.
 * @param p_vector pointer to dynamically allocated pointer to vector.
 */
void vector_free(vector **p_vector){
	if (p_vector == NULL || (*p_vector) == NULL) {
		return;
	}
	for (size_t i = 0; i < (*p_vector)->size; ++i) {
		(*p_vector)->elem_free_func(&((*p_vector)->data[i]));
	}
	free((*p_vector)->data);
//	(*p_vector)->data = NULL;
	free((*p_vector));
//	*p_vector = NULL;

}



/**
 * Returns the element at the given index.
 * @param vector pointer to a vector.
 * @param ind the index of the element we want to get.
 * @return the element at the given index if exists (the element itself, not a copy of it),
 * NULL otherwise.
 */
void *vector_at(const vector *vector, size_t ind){
	if (!vector)
		return NULL;
	if (ind < vector->size)
		return vector->data[ind];
	return NULL;
}



/**
 * Gets a value and checks if the value is in the vector.
 * @param vector a pointer to vector.
 * @param value the value to look for.
 * @return the index of the given value if it is in the vector ([0, vector_size - 1]).
 * Returns -1 if no such value in the vector.
 */
int vector_find(const vector *vector, const void *value){
	if (!vector || !value)
		return -1;
	for (size_t i = 0; i < vector->size; ++i)
	{
		if (vector->elem_cmp_func(vector->data[i], value))
			return (int)i;
	}
	return NOT_IN_VECTOR;
}




/**
 * Adds a new value to the back (index vector_size) of the vector.
 * @param vector a pointer to vector.
 * @param value the value to be added to the vector.
 * @return 1 if the adding has been done successfully, 0 otherwise.
 */
int vector_push_back(vector *vector, const void *value){
	if(!vector ||!value)
		return 0;
	void *val_cpy = vector->elem_copy_func(value);
	vector->data[vector->size] = val_cpy;
	vector->size++;
	if (vector_get_load_factor(vector) > VECTOR_MAX_LOAD_FACTOR){
		vector->capacity *= VECTOR_GROWTH_FACTOR;
		vector->data = realloc(vector->data, vector->capacity * sizeof(void *));
		if (!vector->data)
			return 0;
	}
	if (vector_get_load_factor(vector) == -1)
		return 0;
	return 1;
}



/**
 * This function returns the load factor of the vector.
 * @param vector a vector.
 * @return the vector's load factor, -1 if the function failed.
 */
double vector_get_load_factor(const vector *vector){
	if (!vector || vector->capacity <= 0 || vector->size > vector->capacity)
		return -1;
	double load_factor = ((double)vector->size / (double)vector->capacity);
	return load_factor;
}



/**
 * Removes the element at the given index from the vector. alters the indices of the remaining
 * elements so that there are no empty indices in the range [0, size-1] (inclusive).
 * @param vector a pointer to vector.
 * @param ind the index of the element to be removed.
 * @return 1 if the removing has been done successfully, 0 otherwise.
 */
int vector_erase(vector *vector, size_t ind){
	if(!vector)
		return 0;
//	struct vector *vector_copy = vector;
	vector->elem_free_func(&vector->data[ind]);
	free(vector->data[ind]);
	for (size_t i = ind + 1; i < vector->size; ++i)
	{
		vector->data[i - 1] = vector->data[i];
		vector->elem_free_func(vector->data[i]);
	}
	vector->size--;
	if (vector_get_load_factor(vector) < VECTOR_MIN_LOAD_FACTOR &&
	vector->capacity > VECTOR_INITIAL_CAP)
	{
		vector->capacity /= VECTOR_GROWTH_FACTOR;
		void **realloc_vector = realloc(vector->data, vector->capacity *
				sizeof(void *));
		if (realloc_vector == NULL)
		{
//			vector->data = vector_copy->data;
//			vector->capacity = vector_copy->capacity;
			return 0;
		}
		vector->data = realloc_vector;
		vector->size--;
	}
	return 1;
}



/**
 * Deletes all the elements in the vector.
 * @param vector vector a pointer to vector.
 */
void vector_clear(vector *vector){
	if (!vector)
		return;
	for (size_t i = 0; i < vector->size; ++i)
	{
		vector->elem_free_func(vector->data[i]);
	}
	vector->capacity = 2;
	vector->size = 0;
}

