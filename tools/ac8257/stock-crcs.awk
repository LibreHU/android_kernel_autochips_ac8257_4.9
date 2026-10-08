# Replaces the CONFIG_MODVERSIONS CRCs computed by genksyms with the stock UJC201 kernel's ones
# (tools/ac8257/stock/Module.symvers, rebuilt from the stock image by tools/ac8257/stock_symvers.py),
# for the symbols the stock kernel exports. Used by scripts/Makefile.build with CONFIG_AC8257_STOCK_CRCS.
#
# Why: the stock kernel was built with clang, this tree with gcc. genksyms hashes the preprocessed
# declarations, attributes included, and clang presents itself as GCC 4.2.1, so compiler-gcc.h leaves out
# what it gates on newer GCC versions (__cold, for one): the declarations and therefore most CRCs differ
# although the structures are the same (checked against the stock image: sizes of task_struct, sk_buff,
# mm_struct, signal_struct, sighand_struct, files_struct, inode, file, vm_area_struct, and struct module
# with init at 0x158 and exit at 0x2f0, the same in the stock vendor modules). Without this the vendor
# modules (wmt_drv, wlan_drv_gen4m, bt_drv, gps_drv, fmradio_drv, ...) are refused: "disagrees about
# version of symbol module_layout".
#
# Input: the stock Module.symvers, then genksyms' output ("__crc_NAME = 0x01234567 ;").
FNR == NR { stock[$2] = $1; next }
{
	if ($1 ~ /^_*__crc_/ && $2 == "=") {
		name = $1
		sub(/^_*__crc_/, "", name)
		if (name in stock) {
			printf "%s = %s ;\n", $1, stock[name]
			next
		}
	}
	print
}
