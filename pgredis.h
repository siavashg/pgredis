#ifndef _PGREDIS_H_
#define _PGREDIS_H_
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
#define PR_DEFAULT_DB 0
#define PR_DEFAULT_HOST "localhost"

/*
 * server - The hostname of the Redis server
 * port - The portnumber of the Redis server
 * database - The database within the Redis server to use
 * redis_server - The database handle to communicate with Redis
 */
struct pr_server {
	char *server;
	int port;
	int database;
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
static int pgredis_set(text *, text *, int);

/* Function prototypes for pgredis stored procedures API */
Datum pr_set(PG_FUNCTION_ARGS);
Datum pr_set_expire(PG_FUNCTION_ARGS);
Datum pr_get(PG_FUNCTION_ARGS);
Datum pr_keys(PG_FUNCTION_ARGS);
Datum pr_select_db(PG_FUNCTION_ARGS);
Datum pr_set_server(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pr_set);
PG_FUNCTION_INFO_V1(pr_set_expire);
PG_FUNCTION_INFO_V1(pr_get);
PG_FUNCTION_INFO_V1(pr_keys);
PG_FUNCTION_INFO_V1(pr_select_db);
PG_FUNCTION_INFO_V1(pr_set_server);

#endif /* _PGREDIS_H_ */
