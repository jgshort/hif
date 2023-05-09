#include <stdlib.h>

#include "storage_adapter.h"
#include "memo_repository.h"

static int delete_memo(memo_repository_interface const * repository, int id, int * affected_rows);
static int insert_memo(memo_repository_interface const * repository, char const * memo, int * affected_rows);

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

  repository->insert_memo = &insert_memo;  
  repository->delete_memo = &delete_memo;
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

static int insert_memo(memo_repository_interface const * repository, char const * memo, int * affected_rows) {
  storage_interface const * adapter = ((memo_repository *)repository)->data->adapter;
  return adapter->insert_memo(adapter, memo, affected_rows);
}

static int delete_memo(memo_repository_interface const * repository, int id, int * affected_rows) {
  storage_interface const * adapter = ((memo_repository *)repository)->data->adapter;
  return adapter->delete_by_id(adapter, "hif_memos", id, affected_rows);
}

