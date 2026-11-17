# --- Matrices VINTF d'Android 9 qui acceptent le noyau 4.19 (zips noyau 4.19 a9 : librehu_kernel_twrp_a9 et
# librehu_kernel32_twrp_a9), deplace de kernel-install.sh.
# Android 9 (variante a9) : les matrices de compatibilite (VINTF) du framework n'acceptent que les noyaux 4.4, 4.9 et
# 4.14 ; avec 4.19, VintfObject.verify() echoue et Android affiche a chaque demarrage « Un probleme interne est
# survenu sur votre appareil » (Build.isBuildConsistent() faux). /system n'est jamais modifie (dm-verity) : des
# copies des matrices avec une entree « noyau 4.19 » (sans exigence de configuration) sont ecrites dans
# /vendor/etc/librehu/vintf et montees par-dessus les originales au demarrage (bind mount de
# /vendor/etc/init/librehu_vintf.rc, fait par init avant system_server). Retour : supprimer ces deux chemins.
VINTF_DIR=/etc/librehu/vintf
vintf_accept_419() {
	_sd=$(sysdir)
	_out=$VENMNT$VINTF_DIR
	_rc=$VENMNT/etc/init/librehu_vintf.rc
	rm -rf $_out
	mkdir -p $_out $VENMNT/etc/init || abort "dossier $_out impossible"
	{ echo "# LibreHU $VERSION : matrices VINTF d'Android 9 qui acceptent le noyau 4.19 (zip librehu_kernel_twrp_a9)."
	  echo "# Retour aux matrices d'origine : supprimer ce fichier et /vendor$VINTF_DIR."
	  echo "on post-fs"; } > $_rc.librehu.new
	_n=0
	for _m in $_sd/etc/vintf/compatibility_matrix.*.xml $_sd/compatibility_matrix.xml; do
		[ -f "$_m" ] || continue
		grep -q '<kernel version=' "$_m" || continue
		grep -q '<kernel version="4\.19\.' "$_m" && continue
		_b=$(basename $_m)
		[ "$_m" = "$_sd/compatibility_matrix.xml" ] && _b=legacy_$_b
		awk 'BEGIN { d = 0 } !d && /<kernel version=/ { print "    <kernel version=\"4.19.0\">"; print "    </kernel>"; d = 1 } { print }' \
			"$_m" > $_out/$_b || abort "copie de $_b impossible"
		grep -q '<kernel version="4.19.0">' $_out/$_b || abort "$_b : entree 4.19 non ajoutee"
		[ $(( $(wc -l < $_out/$_b) - $(wc -l < "$_m") )) = 2 ] || abort "$_b : copie incomplete"
		chmod 0644 $_out/$_b
		[ -z "$UJC_TEST" ] && chcon u:object_r:system_file:s0 $_out/$_b
		echo "    mount none /vendor$VINTF_DIR/$_b /system${_m#$_sd} bind" >> $_rc.librehu.new
		_n=$((_n + 1))
	done
	if [ $_n = 0 ]; then
		rm -rf $_rc.librehu.new $_rc $_out
		ui_print "matrices VINTF : noyau 4.19 deja accepte par Android, rien a faire"
		return
	fi
	chmod 0644 $_rc.librehu.new
	[ -z "$UJC_TEST" ] && chcon u:object_r:vendor_configs_file:s0 $_rc.librehu.new
	mv -f $_rc.librehu.new $_rc || abort "ecriture de $_rc impossible"
	ui_print "matrices VINTF : noyau 4.19 accepte ($_n fichier(s), /vendor$VINTF_DIR)"
}
