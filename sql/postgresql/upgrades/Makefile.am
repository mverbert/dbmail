noinst_LIBRARIES = libpgdsu.a

libpgdsu_a_SOURCES = 32001.psql 32002.psql 32003.psql 32004.psql
libpgdsu_a_LIBADD = $(patsubst %.psql, %.o, $(libpgdsu_a_SOURCES))

%.o: %.psql
	$(LD) -b binary -r -o $@ $<
