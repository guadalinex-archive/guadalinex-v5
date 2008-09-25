all: dumpkeycodes thinkpad-keys

dumpkeycodes: dumpkeycodes.c
	$(CC) -O2 -o $@ $<
	strip $@

thinkpad-keys: thinkpad-keys.c
	$(CC) -O2 -o $@ $<
	strip $@

key-constants: /usr/include/linux/input.h aliased-keys
	test -f $<
	echo -n '# Generated from $< dated ' > $@
	date -r $< >> $@

	awk '/^#define  *KEY_/{if(strtonum($$3) <= 255) print $$2"="strtonum($$3)}' $< | sort -nt'=' -k2 >> $@
	echo -n "# Aliases for keys not yet assigned in 'linux/input.h'"
	cat aliased-keys >> $@

atkbd.hk: /var/run/hotkey-setup
	test -f $<
	./make-atkbd.sh

clean:
	- rm -f dumpkeycodes thinkpad-keys thinkpad-keys-update *~


