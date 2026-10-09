/*
 * AutoChips metazone (stock CONFIG_ATC_METAZONE): the board settings store (panel, rotation, Wi-Fi MAC,
 * backlight level, CarPlay certificate...), with the stock kernel API (MetaZone_Read, MetaZone_ReadBinary,
 * ...) and the /dev/mtz ioctl interface libmetazone.so (rotationd, lights HAL...) uses.
 *
 * The LK loads the metazone partition into the autochips,metazone reserved memory (0x60700000) in both
 * boot modes, with the partition layout:
 *   +0x000  MTK partition header (0x58881688 "metazone")
 *   +0x200  header: version, magic 0xabcdef01, size, dw_offset, dw_num, bin_offset, bin_num, bin_unit,
 *           copy2_offset, resv_offset (the offsets are relative to the header)
 *   dword entries (10 bytes):          u8, u8 flags (bit 0 valid), u32 value, u32 default
 *   binary entries (2 * unit + 6 bytes): u8, u8 flags (bit 0 valid), u32 length, data
 * The indexes are 0x10000 + n.
 *
 * Limitation: the writes only change the copy in memory (they last until the next reboot); the stock
 * flush thread that writes the zone back to the eMMC (two copies and a CRC) is not reconstructed, so
 * that a mistake here can never damage the board's metazone partition.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 */
#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <asm/unaligned.h>

#define MTZ_OK			0
#define MTZ_ERR			0x80000000	/* stock "failed" value */
#define MTZ_ERR_INIT		0x80000001	/* stock "not initialised" value */

#define MTZ_HDR_OFFSET		0x200
#define MTZ_MAGIC		0xabcdef01
#define MTZ_IDX_BASE		0x10000
#define MTZ_IDX_END		0x20000
#define MTZ_DW_ENTRY		10
#define MTZ_ENTRY_VALID		0x01

/* libmetazone.so ioctl codes */
#define IOCTL_MZ_READ			0x220804
#define IOCTL_MZ_WRITE			0x220808
#define IOCTL_MZ_READ_BINARY		0x22080c
#define IOCTL_MZ_WRITE_BINARY		0x220810
#define IOCTL_MZ_READ_INFO		0x220820
#define IOCTL_MZ_FLUSH			0x220824
#define IOCTL_MZ_SPEC_WRITE		0x220854
#define IOCTL_MZ_SPEC_WRITE_BINARY	0x220858
#define IOCTL_MZ_SPEC_READ_RESERVED	0x22085c
#define IOCTL_MZ_SPEC_WRITE_RESERVED	0x220860
#define IOCTL_MZ_FACTORY_RESET		0x220864

#define MTZ_MAX_IO		4096

struct mtz_header {
	u32 version;
	u32 magic;
	u32 size;
	u32 dw_offset;
	u32 dw_num;
	u32 bin_offset;
	u32 bin_num;
	u32 bin_unit;
	u32 copy2_offset;
	u32 resv_offset;
};

/* MetaZone_ReadInfo, 36 bytes */
struct mtz_info {
	u32 rsv[3];
	u32 dw_num;
	u32 bin_num;
	u32 bin_unit;
	u32 max_dw;
	u32 max_bin;
	u32 max_unit;
};

struct mtz_ioctl_arg {
	u32 in_len;
	u32 out_len;
	void __user *in;
	void __user *out;
	u32 __user *returned;
};

struct mtz_ioctl_arg32 {
	u32 in_len;
	u32 out_len;
	compat_uptr_t in;
	compat_uptr_t out;
	compat_uptr_t returned;
};

static u8 *mtz_base;
static struct mtz_header mtz_hdr;
static DEFINE_MUTEX(mtz_lock);

static u8 *mtz_dw_entry(u32 idx)
{
	u32 n = idx - MTZ_IDX_BASE;

	if (idx < MTZ_IDX_BASE || n >= mtz_hdr.dw_num)
		return NULL;
	return mtz_base + MTZ_HDR_OFFSET + mtz_hdr.dw_offset + n * MTZ_DW_ENTRY;
}

static u8 *mtz_bin_entry(u32 idx)
{
	u32 n = idx - MTZ_IDX_BASE;

	if (idx < MTZ_IDX_BASE || n >= mtz_hdr.bin_num)
		return NULL;
	return mtz_base + MTZ_HDR_OFFSET + mtz_hdr.bin_offset + n * (2 * mtz_hdr.bin_unit + 6);
}

int MetaZone_Read(u32 idx, u32 *value)
{
	u8 *e;

	if (!mtz_base)
		return MTZ_ERR_INIT;
	e = mtz_dw_entry(idx);
	if (!e || !(e[1] & MTZ_ENTRY_VALID) || !value)
		return MTZ_ERR;
	*value = get_unaligned_le32(e + 2);
	return MTZ_OK;
}
EXPORT_SYMBOL(MetaZone_Read);

int MetaZone_SpecWrite(u32 idx, u32 value, u32 flag)
{
	u8 *e;

	if (!mtz_base)
		return MTZ_ERR_INIT;
	e = mtz_dw_entry(idx);
	if (!e)
		return MTZ_ERR;
	mutex_lock(&mtz_lock);
	put_unaligned_le32(value, e + 2);
	e[1] |= MTZ_ENTRY_VALID;
	mutex_unlock(&mtz_lock);
	return MTZ_OK;
}
EXPORT_SYMBOL(MetaZone_SpecWrite);

int MetaZone_Write(u32 idx, u32 value)
{
	return MetaZone_SpecWrite(idx, value, 0);
}
EXPORT_SYMBOL(MetaZone_Write);

/* Copies min(len, stored length) bytes, returns 0 like the stock function */
int MetaZone_ReadBinary(u32 idx, void *buf, u32 len)
{
	u8 *e;
	u32 n;

	if (!mtz_base)
		return MTZ_ERR_INIT;
	e = mtz_bin_entry(idx);
	if (!e || !(e[1] & MTZ_ENTRY_VALID) || !buf)
		return MTZ_ERR;
	n = min3(get_unaligned_le32(e + 2), len, 2 * mtz_hdr.bin_unit);
	memcpy(buf, e + 6, n);
	return MTZ_OK;
}
EXPORT_SYMBOL(MetaZone_ReadBinary);

int MetaZone_SpecWriteBinary(u32 idx, void *buf, u32 len, u32 flag)
{
	u8 *e;

	if (!mtz_base)
		return MTZ_ERR_INIT;
	e = mtz_bin_entry(idx);
	if (!e || !buf || len > 2 * mtz_hdr.bin_unit)
		return MTZ_ERR;
	mutex_lock(&mtz_lock);
	memcpy(e + 6, buf, len);
	put_unaligned_le32(len, e + 2);
	e[1] |= MTZ_ENTRY_VALID;
	mutex_unlock(&mtz_lock);
	return MTZ_OK;
}
EXPORT_SYMBOL(MetaZone_SpecWriteBinary);

int MetaZone_WriteBinary(u32 idx, void *buf, u32 len)
{
	return MetaZone_SpecWriteBinary(idx, buf, len, 0);
}
EXPORT_SYMBOL(MetaZone_WriteBinary);

int MetaZone_ReadInfo(void *info)
{
	struct mtz_info *i = info;

	if (!mtz_base)
		return MTZ_ERR_INIT;
	if (!i)
		return MTZ_ERR;
	memset(i, 0, sizeof(*i));
	i->dw_num = mtz_hdr.dw_num;
	i->bin_num = mtz_hdr.bin_num;
	i->bin_unit = mtz_hdr.bin_unit;
	i->max_dw = 2000;
	i->max_bin = 500;
	i->max_unit = 100;
	return MTZ_OK;
}
EXPORT_SYMBOL(MetaZone_ReadInfo);

/* No write-back to the eMMC (see the top of the file) */
int MetaZone_Flush(u32 flag)
{
	if (!mtz_base)
		return MTZ_ERR_INIT;
	pr_info_once("metazone: flush requested, changes are kept in memory only\n");
	return MTZ_OK;
}
EXPORT_SYMBOL(MetaZone_Flush);

/* Returns an MTZ_* value; *ret is the byte count reported to the caller */
static u32 mtz_do_ioctl(unsigned int cmd, u8 *in, u32 in_len, u8 *out, u32 out_len, u32 *ret)
{
	u32 r;

	*ret = 0;
	switch (cmd) {
	case IOCTL_MZ_READ:
		if (in_len < 4 || out_len < 4)
			return MTZ_ERR;
		r = MetaZone_Read(get_unaligned_le32(in), (u32 *)out);
		if (r == MTZ_OK)
			*ret = 4;
		return r;
	case IOCTL_MZ_WRITE:
		if (in_len < 8)
			return MTZ_ERR;
		return MetaZone_Write(get_unaligned_le32(in), get_unaligned_le32(in + 4));
	case IOCTL_MZ_SPEC_WRITE:
		if (in_len < 12)
			return MTZ_ERR;
		return MetaZone_SpecWrite(get_unaligned_le32(in), get_unaligned_le32(in + 4),
					  get_unaligned_le32(in + 8));
	case IOCTL_MZ_READ_BINARY: {
		u8 *e;

		if (in_len < 4 || !out)
			return MTZ_ERR;
		r = MetaZone_ReadBinary(get_unaligned_le32(in), out, out_len);
		if (r == MTZ_OK) {
			e = mtz_bin_entry(get_unaligned_le32(in));
			*ret = min3(get_unaligned_le32(e + 2), out_len, 2 * mtz_hdr.bin_unit);
		}
		return r;
	}
	case IOCTL_MZ_WRITE_BINARY:
		/* libmetazone passes the data in the "out" buffer */
		if (in_len < 4 || !out)
			return MTZ_ERR;
		return MetaZone_WriteBinary(get_unaligned_le32(in), out, out_len);
	case IOCTL_MZ_SPEC_WRITE_BINARY:
		if (in_len < 8 || !out)
			return MTZ_ERR;
		return MetaZone_SpecWriteBinary(get_unaligned_le32(in), out, out_len,
						get_unaligned_le32(in + 4));
	case IOCTL_MZ_READ_INFO:
		if (out_len < sizeof(struct mtz_info))
			return MTZ_ERR;
		r = MetaZone_ReadInfo(out);
		if (r == MTZ_OK)
			*ret = sizeof(struct mtz_info);
		return r;
	case IOCTL_MZ_FLUSH:
		return MetaZone_Flush(in_len >= 4 ? get_unaligned_le32(in) : 0);
	case IOCTL_MZ_SPEC_READ_RESERVED:
	case IOCTL_MZ_SPEC_WRITE_RESERVED:
	case IOCTL_MZ_FACTORY_RESET:
	default:
		pr_info_ratelimited("metazone: ioctl 0x%x not supported\n", cmd);
		return MTZ_ERR;
	}
}

/*
 * Like the stock mtz_ioctl: the in and out buffers are copied in, the operation runs, then in, out and
 * the returned count are copied back; a failed operation returns 1 (libmetazone only treats a negative
 * value as an error), a bad user pointer returns -EFAULT.
 */
static long mtz_ioctl_common(unsigned int cmd, struct mtz_ioctl_arg *a)
{
	u8 *in = NULL, *out = NULL;
	u32 ret = 0, r;
	long rc = 0;

	if (a->in_len > MTZ_MAX_IO || a->out_len > MTZ_MAX_IO)
		return -EINVAL;
	if (a->in && a->in_len) {
		in = memdup_user(a->in, a->in_len);
		if (IS_ERR(in))
			return PTR_ERR(in);
	}
	if (a->out && a->out_len) {
		out = memdup_user(a->out, a->out_len);
		if (IS_ERR(out)) {
			rc = PTR_ERR(out);
			out = NULL;
			goto done;
		}
	}

	r = mtz_do_ioctl(cmd, in, in ? a->in_len : 0, out, out ? a->out_len : 0, &ret);

	if ((out && copy_to_user(a->out, out, a->out_len)) ||
	    (a->returned && put_user(ret, a->returned)))
		rc = -EFAULT;
	else
		rc = r == MTZ_OK ? 0 : 1;
done:
	kfree(in);
	kfree(out);
	return rc;
}

static long mtz_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mtz_ioctl_arg a;

	if (copy_from_user(&a, (void __user *)arg, sizeof(a)))
		return -EFAULT;
	return mtz_ioctl_common(cmd, &a);
}

#ifdef CONFIG_COMPAT
static long mtz_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mtz_ioctl_arg32 a32;
	struct mtz_ioctl_arg a;

	if (copy_from_user(&a32, compat_ptr(arg), sizeof(a32)))
		return -EFAULT;
	a.in_len = a32.in_len;
	a.out_len = a32.out_len;
	a.in = compat_ptr(a32.in);
	a.out = compat_ptr(a32.out);
	a.returned = compat_ptr(a32.returned);
	return mtz_ioctl_common(cmd, &a);
}
#endif

static const struct file_operations mtz_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mtz_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mtz_compat_ioctl,
#endif
};

static struct miscdevice mtz_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mtz",
	.fops = &mtz_fops,
	.mode = 0666,
};

static int __init mtz_init(void)
{
	struct device_node *np;
	struct resource res;
	u64 bin_end, dw_end;
	int ret;

	/* the reserved-memory node is the autochips,metazone node with a reg */
	for_each_compatible_node(np, NULL, "autochips,metazone") {
		if (!of_address_to_resource(np, 0, &res)) {
			of_node_put(np);
			break;
		}
	}
	if (!np) {
		pr_err("metazone: no reserved memory\n");
		return -ENODEV;
	}

	mtz_base = memremap(res.start, resource_size(&res), MEMREMAP_WB);
	if (!mtz_base) {
		pr_err("metazone: cannot map %pR\n", &res);
		return -ENOMEM;
	}
	memcpy(&mtz_hdr, mtz_base + MTZ_HDR_OFFSET, sizeof(mtz_hdr));
	/* in 64 bits: the u32 header fields must not wrap the bounds checks */
	dw_end = (u64)MTZ_HDR_OFFSET + mtz_hdr.dw_offset + (u64)mtz_hdr.dw_num * MTZ_DW_ENTRY;
	bin_end = (u64)MTZ_HDR_OFFSET + mtz_hdr.bin_offset +
		  (u64)mtz_hdr.bin_num * (2 * mtz_hdr.bin_unit + 6);
	if (mtz_hdr.magic != MTZ_MAGIC || mtz_hdr.bin_unit > MTZ_MAX_IO / 2 ||
	    mtz_hdr.dw_num > MTZ_IDX_END - MTZ_IDX_BASE || mtz_hdr.bin_num > MTZ_IDX_END - MTZ_IDX_BASE ||
	    dw_end > resource_size(&res) || bin_end > resource_size(&res)) {
		pr_err("metazone: bad header (magic 0x%x), not loaded by the LK?\n", mtz_hdr.magic);
		memunmap(mtz_base);
		mtz_base = NULL;
		return -EINVAL;
	}
	pr_info("metazone: version 0x%x, %u values, %u binaries of %u bytes at %pa (writes kept in memory)\n",
		mtz_hdr.version, mtz_hdr.dw_num, mtz_hdr.bin_num, 2 * mtz_hdr.bin_unit, &res.start);

	ret = misc_register(&mtz_misc);
	if (ret)
		pr_err("metazone: misc_register failed (%d)\n", ret);
	return ret;
}
subsys_initcall(mtz_init);
