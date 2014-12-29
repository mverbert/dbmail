SUBDIRS=upgrades

noinst_LIBRARIES = liblitedsc.a

liblitedsc_a_SOURCES = create_tables.sqlite
liblitedsc_a_LIBADD = $(patsubst %.sqlite, %.o, $(liblitedsc_a_SOURCES))

%.o: %.sqlite
	$(LD) -b binary -r -o $@ $<
