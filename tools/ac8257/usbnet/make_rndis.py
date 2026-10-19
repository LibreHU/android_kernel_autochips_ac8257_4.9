#!/usr/bin/env python3
"""
LibreHU: makes rndis_host usable with the stock AC8257 kernel.

The stock kernel has cdc_ether built in without CONFIG_USB_NET_RNDIS_HOST, so its exported
usbnet_generic_cdc_bind() treats RNDIS functions (USB tethering of Android phones) as bad CDC descriptors.
This writes, into <out dir>:
  - cdc_rndis_bind.c: cdc_ether.c up to that function, with the RNDIS checks enabled, the function renamed
    librehu_generic_cdc_bind() and not exported (no clash with the built-in cdc_ether);
  - rndis_host_main.c: rndis_host.c calling librehu_generic_cdc_bind(), its interfaces named eth%d.

    make_rndis.py <kernel tree>/drivers/net/usb <out dir>
"""
import sys

src, out = sys.argv[1], sys.argv[2]
lines = open(f"{src}/cdc_ether.c").read().split("\n")
end = next(i for i, l in enumerate(lines) if l.startswith("EXPORT_SYMBOL_GPL(usbnet_generic_cdc_bind)"))
body = "\n".join(lines[:end])
for old, new in (
    ("#if IS_ENABLED(CONFIG_USB_NET_RNDIS_HOST)", "#if 1 /* LibreHU: RNDIS recognised, the stock cdc_ether is built without it */"),
    ("int usbnet_generic_cdc_bind(", "int librehu_generic_cdc_bind("),
):
    assert body.count(old) == 1, old
    body = body.replace(old, new)
header = (
    "/*\n * LibreHU: copy of usbnet_generic_cdc_bind() from drivers/net/usb/cdc_ether.c (Linux 4.9), with the RNDIS\n"
    " * interface checks enabled. The stock AC8257 kernel has cdc_ether built in without CONFIG_USB_NET_RNDIS_HOST,\n"
    " * so its exported bind refuses RNDIS functions (USB tethering of Android phones); rndis_host uses this copy.\n */\n"
)
open(f"{out}/cdc_rndis_bind.c", "w").write(header + body + "\n")

m = open(f"{src}/rndis_host.c").read()
assert m.count("usbnet_generic_cdc_bind(") == 1
m = m.replace("usbnet_generic_cdc_bind(", "librehu_generic_cdc_bind(")
m = m.replace(
    "#include <linux/usb/rndis_host.h>",
    "#include <linux/usb/rndis_host.h>\n\n/* LibreHU: cdc_rndis_bind.c */\n"
    "int librehu_generic_cdc_bind(struct usbnet *dev, struct usb_interface *intf);",
    1,
)
# Named eth%d instead of usb%d: Android 9 only takes interfaces matching eth\d (config_ethernet_iface_regex), and
# FLAG_POINTTOPOINT only chooses the name in usbnet. The phone then shows up as a wired network, with DHCP.
flags = "FLAG_ETHER | FLAG_POINTTOPOINT | FLAG_FRAMING_RN | FLAG_NO_SETINT"
assert m.count(flags) == 2
m = m.replace(flags, "FLAG_ETHER | FLAG_FRAMING_RN | FLAG_NO_SETINT /* LibreHU: eth%d */")
open(f"{out}/rndis_host_main.c", "w").write(m)
