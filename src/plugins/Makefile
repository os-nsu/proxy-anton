SUBDIRS = testPlugin

process:
	for plugin in $(SUBDIRS); do \
		make -f $(MAINDIR)/src/plugins/$$plugin/Makefile; \
	done
