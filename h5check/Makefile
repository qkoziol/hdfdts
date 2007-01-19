# Overall Makefile for the h5check tool.

# Default target
all:
	for d in tool test; do \
	    ( cd $$d && $(MAKE)); \
	done

check:
	for d in test; do \
	    ( cd $$d && $(MAKE) $@ ); \
	done

clean:
	for d in tool test; do \
	    ( cd $$d && $(MAKE) $@ ); \
	done
