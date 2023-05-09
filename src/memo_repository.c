#include <stdlib.h>

#include "storage_adapter.h"
#include "memo_repository.h"

struct memo_repository_data;

typedef struct memo_repository {
	memo_repository_interface _interface;

	struct memo_repository_data * data;
} memo_repository;

typedef struct memo_repository_data {
	storage_interface const * adapter;
} memo_repository_data;

memo_repository_interface const * memo_repository_alloc(storage_interface const * adapter) {
	memo_repository * repository = malloc(sizeof * repository);

	return memo_repository_init((memo_repository_interface *)repository, adapter);
}

memo_repository_interface const * memo_repository_init(memo_repository_interface * repository, storage_interface const * adapter) {
	memo_repository_data * data = malloc(sizeof * data);
	((memo_repository *)repository)->data = data;

	((memo_repository *)repository)->data->adapter = adapter;
	repository->free = &memo_repository_free;

	return repository;
}

void memo_repository_free(memo_repository_interface const * repository) {
	if(!repository) return; 
	
	if(((memo_repository *)repository)->data) {
		free(((memo_repository *)repository)->data), ((memo_repository *)repository)->data = NULL;
	}
	free((memo_repository *)repository), repository = NULL;
}


