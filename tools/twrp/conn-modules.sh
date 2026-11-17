# Copie de common/conn-modules.sh de LibreHU-flashablesZips, messages adaptes aux zips de la release du noyau.
# Modules Wi-Fi / BT / GPS / FM recompiles avec un noyau LibreHU (zips TWRP de la release latest-<branche>,
# tools/twrp/make_twrp_zips.py). Avec ces noyaux, les CRC des
# symboles ne sont plus ceux du stock : les modules de /vendor/lib/modules sont refuses ; on met a leur place les
# memes pilotes recompiles avec ce noyau (sources GPL MediaTek, docs/STABLE.md du depot du noyau).
# Sauvegardes : $SDCARD/LibreHU/backup/conn-modules-orig (modules d'origine, une seule fois, jamais ecrasee) et
# conn-modules-<date> (etat d'avant chaque passage). Retour : zip conn-modules-stable-restore.

CONN_MODS="wmt_drv wmt_chrdev_wifi bt_drv gps_drv fmradio_drv wlan_drv_gen4m"

# Extrait files/ du zip dans $CONN_TMP et verifie les empreintes des modules ; F = dossier files extrait
conn_extract() {
	CONN_TMP=${UJC_TMP:-/tmp}/ujc201_conn; rm -rf $CONN_TMP; mkdir -p $CONN_TMP
	unzip -o -q "$ZIPFILE" 'files/*' -d $CONN_TMP || abort "cannot extract the zip"
	F=$CONN_TMP/files
	ui_print "kernel branch: $(cat $F/branch)"
	while read -r _sum _name; do
		[ "$(sha256sum $F/modules/$_name | cut -d' ' -f1)" = "$_sum" ] || abort "wrong checksum for $_name"
	done < $F/modules/SHA256SUMS
}

# Partition boot (UJC_BOOT pour les essais)
boot_part() {
	_bp=${UJC_BOOT:-}
	if [ -z "$_bp" ]; then
		for _p in /dev/block/by-name/boot /dev/block/platform/*/by-name/boot /dev/block/platform/*/*/by-name/boot; do
			[ -e "$_p" ] && { _bp=$_p; break; }
		done
	fi
	[ -n "$_bp" ] || abort "boot partition not found"
	echo $_bp
}

# Empreinte de la section noyau d'une image de boot Android v0/v1 ($1 : fichier ou partition)
kernel_sum() {
	[ "$(dd if=$1 bs=8 count=1 2>/dev/null)" = "ANDROID!" ] || abort "$1 is not an Android boot image"
	_ksz=$(od -An -tu4 -j8 -N4 $1 | tr -d ' ')
	_page=$(od -An -tu4 -j36 -N4 $1 | tr -d ' ')
	dd if=$1 bs=$_page skip=1 2>/dev/null | head -c $_ksz | sha256sum | cut -d' ' -f1
}

# Le noyau de la partition boot doit etre celui de ces modules, sinon arret sans rien modifier
conn_check_boot() {
	_ks=$(kernel_sum $(boot_part))
	if ! grep -q "^$_ks " $F/kernels.sha256; then
		ui_print " "
		ui_print "The kernel of the boot partition is not the one of these modules:"
		ui_print "  $(cut -c1-60 $F/kernel_version)"
		_v=$(cat $F/android 2>/dev/null)
		ui_print "Flash librehu_kernel_twrp${_v:+_$_v}.zip (kernel + modules) of the release first,"
		ui_print "or boot_ac8257${_v:+_$_v}.img / boot_ac8257_bootmenu${_v:+_$_v}.img of the release latest-$(cat $F/branch)."
		abort "different kernel, nothing was changed"
	fi
	ui_print "boot kernel: $(grep "^$_ks " $F/kernels.sha256 | cut -d' ' -f3), matches"
}

# Remplace les modules de $VENMNT/lib/modules (vendor monte en ecriture, dm-verity verifie par l'appelant)
conn_install() {
	MD=$VENMNT/lib/modules
	MARK=$MD/.librehu_conn
	[ -d $MD ] || abort "$MD not found"
	[ -d $SDCARD ] || abort "internal storage not mounted: no backup possible"
	BK=$SDCARD/LibreHU/backup
	mkdir -p $BK || abort "cannot create folder $BK"
	if [ -f $MARK ]; then
		ui_print "modules already replaced by LibreHU: $(head -n1 $MARK)"
		[ -d $BK/conn-modules-orig ] || ui_print "warning: no backup of the original modules in $BK"
	elif [ -d $BK/conn-modules-orig ]; then
		ui_print "original backup already present: $BK/conn-modules-orig"
	else
		conn_save $BK/conn-modules-orig
		ui_print "original modules backed up: $BK/conn-modules-orig"
	fi
	DATED=$BK/conn-modules-$(date +%Y%m%d-%H%M%S)
	conn_save $DATED
	ui_print "current state backed up: $DATED"

	# Place libre (le plus gros module, wlan_drv_gen4m, est ecrit a cote avant de remplacer l'ancien)
	_need=$(( $(du -k $F/modules/wlan_drv_gen4m.ko | cut -f1) + 64 ))
	_free=$(df -k $VENMNT | tail -n1 | awk '{print $(NF-2)}')
	[ -n "$_free" ] && [ "$_free" -lt "$_need" ] && abort "vendor full ($_free KB free, $_need needed)"

	# Copie a cote (.librehu.new), droits et contexte de l'original, empreinte relue, puis renommage
	for m in $CONN_MODS; do
		k=$MD/$m.ko
		set -- $(grep "^$m " $DATED/meta.txt)
		if [ "$2" = absent ]; then mode=644; uid=0; gid=0; ctx=u:object_r:vendor_file:s0; else mode=$2; uid=$3; gid=$4; ctx=$5; fi
		cp $F/modules/$m.ko $k.librehu.new || abort "cannot copy $m.ko"
		chmod $mode $k.librehu.new; chown $uid:$gid $k.librehu.new 2>/dev/null
		[ -z "$UJC_TEST" ] && chcon $ctx $k.librehu.new
		[ "$(sha256sum $k.librehu.new | cut -d' ' -f1)" = "$(grep " $m.ko\$" $F/modules/SHA256SUMS | cut -d' ' -f1)" ] \
			|| abort "$m.ko read back different after copy"
		mv -f $k.librehu.new $k || abort "cannot replace $m.ko"
		ui_print "$m.ko replaced"
	done
	{ echo "$VERSION $(cat $F/branch)"; cat $F/kernel_version; } > $MARK
	chmod 0644 $MARK
	[ -z "$UJC_TEST" ] && chcon u:object_r:vendor_file:s0 $MARK
	rm -rf $CONN_TMP
}

# Sauvegarde de l'etat actuel de $MD dans $1 : fichiers, droits, proprietaire, contexte SELinux, empreintes
conn_save() {
	mkdir -p $1 || abort "cannot create folder $1"
	: > $1/meta.txt; : > $1/SHA256SUMS
	for m in $CONN_MODS; do
		k=$MD/$m.ko
		if [ -f $k ]; then
			cp $k $1/ || abort "cannot back up $m.ko (storage full?)"
			ctx=$(ls -Z $k 2>/dev/null | awk '{print $1}')
			case "$ctx" in u:*) ;; *) ctx=u:object_r:vendor_file:s0;; esac
			echo "$m $(stat -c '%a %u %g' $k) $ctx" >> $1/meta.txt
			echo "$(sha256sum $k | cut -d' ' -f1)  $m.ko" >> $1/SHA256SUMS
		else
			echo "$m absent" >> $1/meta.txt
		fi
	done
	[ -f $MARK ] && cp $MARK $1/librehu_conn.txt
}
