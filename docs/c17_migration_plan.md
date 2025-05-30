# C17 Migration Planning

## Header File Inventory
Total header files: 11648

The first 30 header files:

./usr.lib/libcrypt/crypt.h
./usr.lib/libskey/skey.h
./usr.lib/libterminfo/term_private.h
./usr.lib/libterminfo/termcap.h
./usr.lib/libterminfo/term.h
./usr.lib/libradius/radlib.h
./usr.lib/libradius/radlib_vs.h
./usr.lib/libradius/radlib_private.h
./usr.lib/libcurses/curses.h
./usr.lib/libcurses/fileio.h
./usr.lib/libcurses/unctrl.h
./usr.lib/libcurses/keymap.h
./usr.lib/libcurses/curses_private.h
./usr.lib/libm/complex/cephes_subr.h
./usr.lib/libm/complex/cephes_subrf.h
./usr.lib/libm/noieee_src/mathimpl.h
./usr.lib/libm/noieee_src/trig.h
./usr.lib/libm/src/namespace.h
./usr.lib/libm/src/math_private.h
./usr.lib/libm/src/exception.h
./usr.lib/libm/arch/i387/abi.h
./usr.lib/libpcap/sunatmpos.h
./usr.lib/libpcap/gencode.h
./usr.lib/libpcap/sll.h
./usr.lib/libpcap/version.h
./usr.lib/libpcap/ethertype.h
./usr.lib/libpcap/atmuni31.h
./usr.lib/libpcap/pcap-namedb.h
./usr.lib/libpcap/pcap.h
./usr.lib/libpcap/ppp.h


A search for `();` within these headers returned no results, suggesting they already use modern prototypes.

## Proposed Batch Processing
Modernize headers in batches of 30. With 11648 headers this yields approximately 389 batches.

### Audit Checklist
- Identify K&R-style declarations
- Ensure functions with no parameters use `void`
- Replace outdated macros
- Apply formatting and comment cleanup

Potential K&R prototype detection command:
```sh
grep -n "();" <header>
```

