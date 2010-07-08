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

/* PostgreSQL related function prototypes */
void _PG_init(void);
void _PG_fini(void);

/* Function prototypes for pgredis stored procedures API */
Datum pr_set(PG_FUNCTION_ARGS);
Datum pr_get(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pr_set);
PG_FUNCTION_INFO_V1(pr_get);

#endif /* _PGREDIS_H_ */
