MODULE_big=pgredis
PGMC_VERSION=2.0.4
SHLIB_LINK=-lcredis
OBJS=pgredis.o
DATA_built=$(MODULE_big).sql

PGXS=$(shell pg_config --pgxs)
include $(PGXS)

help:
	@echo "To build and install: ''make install''"
	@echo "See README for notes on installing pgredis in your database"
