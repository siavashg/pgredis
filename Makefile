CFLAGS+=-g -O2 -Wall -Werror -ansi -pedantic -std=c99

MODULE_big=pgredis
PGMC_VERSION=2.0.4
SHLIB_LINK=-lcredis
OBJS=pgredis.o
DATA_built=$(MODULE_big).sql

PGXS=$(shell pg_config --pgxs)
include $(PGXS)

