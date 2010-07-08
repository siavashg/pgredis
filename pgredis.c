/* 
 * pgredis
 * 
 */
#include "pgredis.h"
#include <credis.h>
#include <postgres.h>

#include <access/heapam.h>
#include <access/htup.h>
#include <fmgr.h>
#include <funcapi.h>
#include <lib/stringinfo.h>
#include <utils/builtins.h>
#include <utils/datetime.h>
#include <utils/guc.h>
#include <utils/memutils.h>
#include <utils/lsyscache.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define PG_RETURN_FALSE PG_RETURN_BOOL(1 == 0)

static REDIS pgredis;

void
_PG_init(void) {
	pgredis = credis_connect("localhost", 6789, 2000);
	if (!pgredis)
		elog(ERROR, "Unable to connect to redis instance, operations will be disabled");
}

void
_PG_fini(void) {
	if (pgredis)
		credis_close(pgredis);
}

Datum
pr_set(PG_FUNCTION_ARGS) {
	text * key_arg = PG_GETARG_TEXT_P(0);
	text * value_arg = PG_GETARG_TEXT_P(1);
	int expire = PG_GETARG_UINT32(2);
	int ret;
	
	char * key = NULL;
	char * value = NULL;
	
	if (!pgredis) {
		elog(ERROR, "Unable to set, redis instance missing");
		PG_RETURN_FALSE;
	}

  	key = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(key_arg)));
  	value = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(value_arg)));
	
	if (!key || !value) {
		elog(ERROR, "Key/value incomplete in pgredis set");
		PG_RETURN_FALSE;
	}

	ret = credis_set(pgredis, key, value);
	if (ret == CREDIS_OK) {
		ret = credis_expire(pgredis, key, expire);
	}
  	PG_RETURN_BOOL(ret == CREDIS_OK);
}

Datum
pr_get(PG_FUNCTION_ARGS) {
	text * key_arg = PG_GETARG_TEXT_P(0);
	char * key = NULL;
	char * value = NULL;
	text * ret_val = NULL;
	int ret = 0;

	if (!pgredis) {
		elog(ERROR, "Redis instance missing, unable to get");
		PG_RETURN_NULL();
	}

	if (!key_arg) {
		elog(ERROR, "key argument missing");
		PG_RETURN_NULL();
	}
	
	key = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(key_arg)));
	if (!key || strlen(key) == 0) {
		elog(ERROR, "Empty key in redis get");
		PG_RETURN_NULL();
	}

	ret = credis_get(pgredis, key, &value);
	if (ret == CREDIS_OK) {
		ret_val = (text *)palloc(strlen(value) + VARHDRSZ);
		SET_VARSIZE(ret_val, strlen(value) + VARHDRSZ);
		memcpy(VARDATA(ret_val), value, strlen(value));
 		PG_RETURN_TEXT_P(ret_val);
	}

	PG_RETURN_NULL();
}
