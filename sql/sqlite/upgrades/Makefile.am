noinst_LIBRARIES = liblitedsu.a

liblitedsu_a_SOURCES = 32001.sqlite 32002.sqlite 32003.sqlite 32004.sqlite
liblitedsu_a_LIBADD = $(patsubst %.sqlite, %.o, $(liblitedsu_a_SOURCES))

%.o: %.sqlite
	$(LD) -b binary -r -o $@ $<
