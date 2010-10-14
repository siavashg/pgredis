/*
 * Copyright (c) 2010 Daniel Gustafsson, Siavash Ghorbani, William Tisater
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* 
 * pgredis
 * 
 */
#include "pgredis.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

static struct pr_server *pgredis;

static char *pgredis_server = "";

void
_PG_init(void) {
	
	/* Setup custom GUC variables */
	DefineCustomStringVariable(
		"pgredis.redis_server",
		"The Redis server to connect to",
		"Redis server specified as host[:port[:db]]",
		&pgredis_server,
		NULL,
		PGC_USERSET,
		GUC_LIST_INPUT,
		(GucStringAssignHook)guc_add_default_server,
		(GucShowHook)guc_show_server_info);
}

void
_PG_fini(void) {
	if (pgredis && pgredis->redis_server)
		credis_close(pgredis->redis_server);
}

/*
 * GUC Functions
 */

static GucStringAssignHook
guc_add_default_server(const char *value, bool doit, GucSource source) {
	add_redis_server(value);
	return (GucStringAssignHook) value;
}

static GucShowHook
guc_show_server_info(void) {
	if (!pgredis || !pgredis->server)
		return (GucShowHook)"";
		
	return (GucShowHook)pgredis->server;
}

/*
 * add_redis_server - Adds a Redis server instance to connect to with an optional portnumber and
 * Redis database number. The format for connect string is: <hostname>[:<port>[:<redis database>]]
 */
static void
add_redis_server(const char * host_port) {
	char *tmp = (char *)host_port;
	char *ptr;
	int ret;

	if (!pgredis) {
		if ((pgredis = (struct pr_server *)palloc(sizeof (struct pr_server))) == NULL)
			return;
		pgredis->server = NULL;
		pgredis->redis_server = NULL;
	}

	/* Currently we dont overwrite the current server or support multiple servers */
	if (pgredis->server)
		return;

	while (*tmp && *tmp != ':')
		tmp++;

	pgredis->server = (char *)palloc(tmp - host_port + 1);
	strncpy(pgredis->server, host_port, (tmp - host_port));
	pgredis->server[(tmp - host_port)] = '\0';

	if (*tmp == ':') {
		pgredis->port = (int)strtod(++tmp, &ptr);
		if (tmp == ptr) {
			pgredis->port = PR_DEFAULT_PORT;
			pgredis->database = PR_DEFAULT_DB;
		}
		else {
			if (*ptr == ':') {
				tmp = ++ptr;
				pgredis->database = (int)strtod(tmp, &ptr);
				if (tmp == ptr)
					pgredis->database = PR_DEFAULT_DB;
			}
		}
	}
	else {
		pgredis->port = PR_DEFAULT_PORT;
		pgredis->database = PR_DEFAULT_DB;
	}
	
	elog(LOG, "pgredis: added server: %s:%i:%i", pgredis->server, pgredis->port, pgredis->database);

	ret = redis_connect();
	if (ret == PR_FAIL)
		elog(ERROR, "pgredis: unable to connect to server");
	else
		elog(LOG, "pgredis: connected to server");
}

/*
 * Connect to the redis server. If the passed server struct is uninitialized it
 * will be allocated prior to connection.
 * Returns PR_CONN if connection succeeded, else PR_FAILED 
 */
static int
redis_connect(void) {
	int ret = 0;

	if (!pgredis)
		return PR_FAIL;

	if (pgredis->redis_server != NULL)
		return PR_CONN;

	pgredis->redis_server = credis_connect((pgredis->server ? pgredis->server : PR_DEFAULT_HOST), (pgredis->port > 0 ? pgredis->port : PR_DEFAULT_PORT), 2000);
	if (!pgredis->redis_server)
		return PR_FAIL;

	ret = credis_select(pgredis->redis_server, pgredis->database);			
	return (ret == CREDIS_OK ? PR_CONN : PR_FAIL);
}


/*
 * DML Functions
 */
static int
pgredis_set(text *key_arg, text *value_arg, int expire_arg) {
	int ret;
	char * key = NULL;
	char * value = NULL;
	
	if (!pgredis || !pgredis->redis_server) {
		elog(ERROR, "Unable to set, redis instance missing");
		PG_RETURN_FALSE;
	}

  	key = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(key_arg)));
  	value = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(value_arg)));
	
	if (!key || !value) {
		elog(ERROR, "Key/value incomplete in pgredis set");
		PG_RETURN_FALSE;
	}

	ret = credis_set(pgredis->redis_server, key, value);
	if (ret == CREDIS_OK && expire_arg > 0)
		ret = credis_expire(pgredis->redis_server, key, expire_arg);

  	return ret;
}

Datum
pr_set(PG_FUNCTION_ARGS) {
	text * key_arg = PG_GETARG_TEXT_P(0);
	text * value_arg = PG_GETARG_TEXT_P(1);

	PG_RETURN_BOOL(pgredis_set(key_arg, value_arg, 0) == CREDIS_OK);
}

Datum
pr_set_expire(PG_FUNCTION_ARGS) {
	text * key_arg = PG_GETARG_TEXT_P(0);
	text * value_arg = PG_GETARG_TEXT_P(1);
	int expire_arg = PG_GETARG_UINT32(2);

	PG_RETURN_BOOL(pgredis_set(key_arg, value_arg, expire_arg) == CREDIS_OK);
}

Datum
pr_set_server(PG_FUNCTION_ARGS) {
	text * server_arg = PG_GETARG_TEXT_P(0);
	char * server;
	
	server = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(server_arg)));
	add_redis_server(server);
	PG_RETURN_NULL();
}

Datum
pr_select_db(PG_FUNCTION_ARGS) {
	int database_arg = PG_GETARG_UINT32(0);
	int ret = 0;

	if (!pgredis || !pgredis->redis_server) {
		elog(ERROR, "Redis server unspecified, unable to select database");
		PG_RETURN_NULL();
	}

	if (database_arg < 0 || database_arg > 15)
		PG_RETURN_FALSE;
	
	if (database_arg != pgredis->database) {
		ret = credis_select(pgredis->redis_server, database_arg);
		if (ret == CREDIS_OK)
			pgredis->database = database_arg;
	}
	
	PG_RETURN_BOOL(pgredis->database == database_arg);
}

Datum
pr_get(PG_FUNCTION_ARGS) {
	text * key_arg = PG_GETARG_TEXT_P(0);
	char * key = NULL;
	char * value = NULL;
	text * ret_val = NULL;
	int ret = 0;

	if (!pgredis || !pgredis->redis_server) {
		elog(ERROR, "Redis instance missing, unable to get");
		PG_RETURN_NULL();
	}

	if (!key_arg)
		PG_RETURN_NULL();
	
	key = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(key_arg)));
	if (!key || strlen(key) == 0)
		PG_RETURN_NULL();

	ret = credis_get(pgredis->redis_server, key, &value);
	if (ret == CREDIS_OK) {
		ret_val = (text *)palloc(strlen(value) + VARHDRSZ);
		SET_VARSIZE(ret_val, strlen(value) + VARHDRSZ);
		memcpy(VARDATA(ret_val), value, strlen(value));
 		PG_RETURN_TEXT_P(ret_val);
	}

	PG_RETURN_NULL();
}

