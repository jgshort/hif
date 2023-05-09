#ifndef HIF_STORAGE_ADAPTER
#define HIF_STORAGE_ADAPTER

typedef int (*kvp_handler)(char const * key, char const * value, int is_numeric);

typedef struct storage_adapter storage_adapter;

storage_adapter const * storage_adapter_alloc();
storage_adapter const * storage_adapter_init(storage_adapter * adapter);
void storage_adapter_free(storage_adapter * adapter);

typedef struct storage_adapter {
	int (*create_storage)(storage_adapter const * adapter, char const * path);
	int (*open_storage)(storage_adapter const * adapter, char const * context_name);
	void (*close)(storage_adapter const *adapter);
	
	int (*create_feel)(storage_adapter const * adapter, char const * feel, char const * description);
	int (*insert_feel)(storage_adapter const * adapter, char const * feel, char **description);
	int (*delete_feel)(storage_adapter const * adapter, int id);
	int (*count_feels)(storage_adapter const * adapter);
	
	int (*insert_memo)(storage_adapter const * adapter, char const * memo);
	
	int (*export)(storage_adapter const * adapter, kvp_handler kvp);

	void (*free)(storage_adapter * adapter);

	struct storage_adapter_data * data;
} storage_adapter;

#endif /* HIF_STORAGE_ADAPTER */
