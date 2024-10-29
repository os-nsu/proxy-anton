SUBDIRS = testPlugin

process:
	for plugin in $(SUBDIRS); do \
		make -f $(MAINDIR)/contrib/$$plugin/Makefile; \
	done
