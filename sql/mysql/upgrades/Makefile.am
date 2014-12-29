noinst_LIBRARIES = libmydsu.a

libmydsu_a_SOURCES = 32001.mysql 32002.mysql 32003.mysql 32004.mysql
libmydsu_a_LIBADD = $(patsubst %.mysql, %.o, $(libmydsu_a_SOURCES))

%.o: %.mysql
	$(LD) -b binary -r -o $@ $<
