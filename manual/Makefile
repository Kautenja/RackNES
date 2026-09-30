.DEFAULT_GOAL := manuals
.PHONY: manuals RackNES CVGenie clean
manuals: RackNES CVGenie
RackNES CVGenie:
	$(MAKE) -C $@
clean:
	$(MAKE) -C RackNES clean
	$(MAKE) -C CVGenie clean
