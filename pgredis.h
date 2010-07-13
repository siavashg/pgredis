#ifndef _PGREDIS_H_
#define _PGREDIS_H_

#include "postgres.h"

#include "access/heapam.h"
#include "access/htup.h"
#include "fmgr.h"
#include "funcapi.h"
#include "lib/stringinfo.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/guc.h"
#include "utils/memutils.h"
#include "utils/lsyscache.h"

#include <string.h>
#include <credis.h>

#define PR_CONN 0x01
#define PR_FAIL 0x02

#define PR_DEFAULT_PORT 6789
#define PR_DEFAULT_HOST "localhost"

struct pr_server {
	char *server;
	int port;
	REDIS redis_server;
};

/* PostgreSQL related function prototypes */
void _PG_init(void);
void _PG_fini(void);

#define PG_RETURN_FALSE PG_RETURN_BOOL(1 == 0)
#define PG_RETURN_TRUE PG_RETURN_BOOL(1 == 1)

static GucStringAssignHook guc_add_default_server(const char *value, bool doit, GucSource source);
static GucShowHook guc_show_server_info(void);

/* Helper function prototypes */
static void add_redis_server(const char * host_port);
static int redis_connect(void);

/* Function prototypes for pgredis stored procedures API */
Datum pr_set(PG_FUNCTION_ARGS);
Datum pr_get(PG_FUNCTION_ARGS);
Datum pr_set_server(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pr_set);
PG_FUNCTION_INFO_V1(pr_get);
PG_FUNCTION_INFO_V1(pr_set_server);

#endif /* _PGREDIS_H_ */
