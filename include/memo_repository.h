#ifndef MEMO_REPOSITORY
#define MEMO_REPOSITORY

#include "storage_adapter.h"

typedef struct memo_repository_interface memo_repository_interface;

typedef struct memo_repository_interface {
	void (*free)(memo_repository_interface const * repository);
} memo_repository_interface;


memo_repository_interface const * memo_repository_alloc();
memo_repository_interface const * memo_repository_init(memo_repository_interface * repository, storage_interface const * adapter);
void memo_repository_free(memo_repository_interface const * repository);

#endif /* MEMO_REPOSITORY */
