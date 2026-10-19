#!/usr/bin/env python3
"""LibreHU: checks out-of-tree modules against the stock Module.symvers (vermagic, every import resolved with the stock CRC).

    check_ko.py <stock Module.symvers> <module.ko>...
"""
import sys,struct,re
stock={l.split()[1]:int(l.split()[0],16) for l in open(sys.argv[1])}
for ko in sys.argv[2:]:
    d=open(ko,'rb').read()
    shoff=struct.unpack_from('<Q',d,0x28)[0]; es,n,si=struct.unpack_from('<HHH',d,0x3a)
    secs=[struct.unpack_from('<IIQQQQIIQQ',d,shoff+i*es) for i in range(n)]
    st=secs[si]; nm=lambda o:d[st[4]+o:d.index(b'\0',st[4]+o)].decode()
    sec={nm(s[0]):s for s in secs}
    v=sec['__versions']; bad=[]; cnt=0
    for i in range(v[4],v[4]+v[5],64):
        crc=struct.unpack_from('<Q',d,i)[0]; name=d[i+8:i+64].split(b'\0')[0].decode(); cnt+=1
        if stock.get(name)!=crc: bad.append(name)
    mi=d[sec['.modinfo'][4]:sec['.modinfo'][4]+sec['.modinfo'][5]].split(b'\0')
    info={x.split(b'=',1)[0].decode():x.split(b'=',1)[1].decode() for x in mi if b'=' in x}
    # undefined symbols must all be versioned (otherwise: unknown symbol at load)
    symtab=sec['.symtab']; strtab=secs[symtab[6]]
    undef=set()
    for i in range(symtab[4],symtab[4]+symtab[5],24):
        name_off,info_b,other,shndx,val,size=struct.unpack_from('<IBBHQQ',d,i)
        if shndx==0 and name_off:
            undef.add(d[strtab[4]+name_off:d.index(b'\0',strtab[4]+name_off)].decode())
    vers={d[i+8:i+64].split(b'\0')[0].decode() for i in range(v[4],v[4]+v[5],64)}
    missing=sorted(undef-vers-{''})
    print(f"{ko.rsplit('/',1)[-1]:16} imports={cnt:3} crc_bad={len(bad)} unresolved={missing} vermagic_ok={info.get('vermagic')=='4.9.117+ SMP preempt mod_unload modversions aarch64'} depends={info.get('depends')!r}")
