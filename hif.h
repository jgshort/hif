#ifndef HIF_HIF
#define HIF_HIF

#define HIF_EXECUTABLE "hif"
#define HIF_VERSION "v1.0.0"

typedef enum hif_feel {
	HIF_BAD = 1 << 0,
	HIF_MEH = 1 << 1,
	HIF_WOO = 1 << 2,

	HIF_COMMAND = 1 << 3,
	HIF_COMMAND_INIT = 1 << 4,
	HIF_COMMAND_JSON = 1 << 5,
	HIF_COMMAND_COUNT = 1 << 6,
	HIF_COMMAND_DELETE = 1 << 7,
	HIF_COMMAND_CREATE_FEEL = 1 << 8,
	HIF_COMMAND_ADD = 1 << 9
} hif_feel;

#endif /* HIF_HIF */
