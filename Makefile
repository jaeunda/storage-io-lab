PROJECTS = 01-flash-io 02-hybrid-ftl 03-record-store

.PHONY: all clean measure $(PROJECTS)
all: $(PROJECTS)

$(PROJECTS):
	$(MAKE) -C $@

measure:
	python3 experiments/measure.py

clean:
	$(foreach project,$(PROJECTS),$(MAKE) -C $(project) clean;)
