
misdir = $(datadir)/signus/$(VERSION)/nolang

mis_DATA = missions.dat

missions.dat: 
	$(MAKEDAT) $@ /nohdr $(srcdir)/*.mis

clean-local:
	rm -f missions.dat
