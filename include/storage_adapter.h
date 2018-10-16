#ifndef HIF_STORAGE_ADAPTER
#define HIF_STORAGE_ADAPTER

typedef int (*kvp_handler)(char const * key, char const * value, int is_numeric);

typedef struct storage_interface storage_interface;
typedef struct storage_interface {
	int (*create_storage)(storage_interface const * adapter, char const * path);
	int (*open_storage)(storage_interface  const * adapter, char const * context_name);
	int (*close)(storage_interface const *adapter);
	
	int (*create_feel)(storage_interface const * adapter, char const * feel, char const * description);
	int (*get_feel_description)(storage_interface const * adapter,  char const * feel, char **description);

	int (*insert_feel)(storage_interface const * adapter, char const * feel, char **description);
	int (*delete_feel)(storage_interface const * adapter, int id, int * affected_rows);
	int (*count_feels)(storage_interface const * adapter);
	
	int (*insert_memo)(storage_interface const * adapter, char const * memo, int * affected_rows);
	int (*delete_memo)(storage_interface const * adapter, int id, int * affected_rows);

	int (*export)(storage_interface const * adapter, kvp_handler kvp);

	void (*free)(storage_interface const * adapter);
} storage_interface;

storage_interface const * storage_adapter_alloc();
storage_interface const * storage_adapter_init(storage_interface * adapter);
void storage_adapter_free(storage_interface const * adapter);

#endif /* HIF_STORAGE_ADAPTER */
