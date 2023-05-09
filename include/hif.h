#ifndef HIF_HIF
#define HIF_HIF

#define HIF_EXECUTABLE "hif"
#define HIF_VERSION "v1.0.0"

typedef enum hif_command {
	HIF_COMMAND_CREATE,
	/* HIF_COMMAND_OPEN */
	HIF_COMMAND_COUNT_FEELS,
	
	HIF_COMMAND_JSON,

	HIF_COMMAND_DELETE_FEEL,
	HIF_COMMAND_CREATE_FEEL,
	HIF_COMMAND_ADD_FEEL,

	HIF_COMMAND_COUNT_MEMOS,
	HIF_COMMAND_ADD_MEMO,

	HIF_COMMAND_EOF /* must be last */
} hif_command;

#endif /* HIF_HIF */
