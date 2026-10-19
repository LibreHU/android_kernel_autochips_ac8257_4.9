/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include "imgsensor.h"
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/string.h>


#include "imgsensor_proc.h"
#include "kd_imgsensor_define.h"
#include "imgsensor_sensor_list.h"

char mtk_ccm_name[camera_info_size] = { 0 };
char mtk_i2c_dump[camera_info_size] = { 0 };

static struct SENSOR_FUNCTION_STRUCT *tp2850_pfFunc;
static struct SENSOR_FUNCTION_STRUCT *n4_pfFunc;
static int proc_flag = 0;
static MSDK_SENSOR_REG_INFO_STRUCT proc_tempSensor;



static int pdaf_type_info_read(struct seq_file *m, void *v)
{
#define bufsz 512

	unsigned int len = bufsz;
	char pdaf_type_info[bufsz];

	struct SENSOR_FUNCTION_STRUCT *psensor_func =
	    pgimgsensor->sensor[IMGSENSOR_SENSOR_IDX_MAIN].pfunc;

	memset(pdaf_type_info, 0, 512);

	if (psensor_func == NULL)
		return 0;


	psensor_func->SensorFeatureControl(
	    SENSOR_FEATURE_GET_PDAF_TYPE,
	    pdaf_type_info,
	    &len);

	seq_printf(m, "%s\n", pdaf_type_info);
	return 0;
};

static int proc_SensorType_open(struct inode *inode, struct file *file)
{
	return single_open(file, pdaf_type_info_read, NULL);
};

static ssize_t proc_SensorType_write(
	struct file *file,
	const char *buffer,
	size_t count,
	loff_t *data)
{
	char regBuf[64] = { '\0' };
	u32 u4CopyBufSize =
	    (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1);

	struct SENSOR_FUNCTION_STRUCT *psensor_func =
	    pgimgsensor->sensor[IMGSENSOR_SENSOR_IDX_MAIN].pfunc;

	if (copy_from_user(regBuf, buffer, u4CopyBufSize))
		return -EFAULT;

	if (psensor_func)
		psensor_func->SensorFeatureControl(
		    SENSOR_FEATURE_SET_PDAF_TYPE,
		    regBuf,
		    &u4CopyBufSize);

	return count;
};
#if 0
/************************************************************************
 * CAMERA_HW_Reg_Debug()
 * Used for sensor register read/write by proc file
 ************************************************************************/
static ssize_t CAMERA_HW_Reg_Debug(
	struct file *file,
	const char *buffer,
	size_t count,
	loff_t *data)
{
	char regBuf[64] = { '\0' };
	u32 u4CopyBufSize =
	    (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1);

	struct IMGSENSOR_SENSOR *psensor =
	    &pgimgsensor->sensor[IMGSENSOR_SENSOR_IDX_MAIN];

	MSDK_SENSOR_REG_INFO_STRUCT sensorReg;

	memset(&sensorReg, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

	if (psensor == NULL || copy_from_user(regBuf, buffer, u4CopyBufSize))
		return -EFAULT;

	if (sscanf(
	    regBuf,
	    "%x %x",
	    &sensorReg.RegAddr,
	    &sensorReg.RegData) == 2) {

		imgsensor_sensor_feature_control(
		    psensor,
		    SENSOR_FEATURE_SET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

		imgsensor_sensor_feature_control(
		    psensor, SENSOR_FEATURE_GET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

		pr_debug(
		    "write addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

		snprintf(
		    mtk_i2c_dump,
		    sizeof(mtk_i2c_dump),
		    "addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

	} else if (kstrtouint(regBuf, 16, &sensorReg.RegAddr) == 0) {
		imgsensor_sensor_feature_control(
		    psensor,
		    SENSOR_FEATURE_GET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

		pr_debug(
		    "read addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

		snprintf(
		    mtk_i2c_dump,
		    sizeof(mtk_i2c_dump),
		    "addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);
	}
	return count;
}
#endif

static ssize_t CAMERA_HW_Reg_Debug2(
	struct file *file,
	const char *buffer,
	size_t count,
	loff_t *data)
{
	char regBuf[64] = { '\0' };
	u32 u4CopyBufSize =
	    (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1);

	struct IMGSENSOR_SENSOR *psensor =
	    &pgimgsensor->sensor[IMGSENSOR_SENSOR_IDX_SUB];

	MSDK_SENSOR_REG_INFO_STRUCT sensorReg;

	memset(&sensorReg, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

	if (psensor == NULL || copy_from_user(regBuf, buffer, u4CopyBufSize))
		return -EFAULT;

	if (sscanf(
	    regBuf,
	    "%x %x",
	    &sensorReg.RegAddr,
	    &sensorReg.RegData) == 2) {
		imgsensor_sensor_feature_control(
		    psensor, SENSOR_FEATURE_SET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

		imgsensor_sensor_feature_control(
		    psensor, SENSOR_FEATURE_GET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

		pr_debug(
		    "write addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

		snprintf(
		    mtk_i2c_dump,
		    sizeof(mtk_i2c_dump),
		    "addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

	} else if (kstrtouint(regBuf, 16, &sensorReg.RegAddr) == 0) {
		imgsensor_sensor_feature_control(
		    psensor,
		    SENSOR_FEATURE_GET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

		pr_debug(
		    "read addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

		snprintf(
		    mtk_i2c_dump,
		    sizeof(mtk_i2c_dump),
		    "addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr, sensorReg.RegData);
	}

	return count;
}

static ssize_t CAMERA_HW_Reg_Debug3(
	struct file *file,
	const char *buffer,
	size_t count,
	loff_t *data)
{
	char regBuf[64] = { '\0' };
	u32 u4CopyBufSize =
	    (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1);

	struct IMGSENSOR_SENSOR *psensor =
	    &pgimgsensor->sensor[IMGSENSOR_SENSOR_IDX_MAIN2];

	MSDK_SENSOR_REG_INFO_STRUCT sensorReg;

	memset(&sensorReg, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

	if (psensor == NULL || copy_from_user(regBuf, buffer, u4CopyBufSize))
		return -EFAULT;

	if (sscanf(
	    regBuf,
	    "%x %x",
	    &sensorReg.RegAddr,
	    &sensorReg.RegData) == 2) {

		imgsensor_sensor_feature_control(
		    psensor,
		    SENSOR_FEATURE_SET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

		imgsensor_sensor_feature_control(
		    psensor, SENSOR_FEATURE_GET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

		pr_debug(
		    "write addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

		snprintf(
		    mtk_i2c_dump,
		    sizeof(mtk_i2c_dump),
		    "addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

	} else if (kstrtouint(regBuf, 16, &sensorReg.RegAddr) == 0) {
		imgsensor_sensor_feature_control(
		    psensor,
		    SENSOR_FEATURE_GET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

		pr_debug(
		    "read addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

		snprintf(
		    mtk_i2c_dump,
		    sizeof(mtk_i2c_dump),
		    "addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);
	}

	return count;
}

static ssize_t CAMERA_HW_Reg_Debug4(
	struct file *file,
	const char *buffer,
	size_t count,
	loff_t *data)
{
	char regBuf[64] = { '\0' };
	u32 u4CopyBufSize =
	    (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1);

	struct IMGSENSOR_SENSOR *psensor =
	    &pgimgsensor->sensor[IMGSENSOR_SENSOR_IDX_SUB2];

	MSDK_SENSOR_REG_INFO_STRUCT sensorReg;

	memset(&sensorReg, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

	if (psensor == NULL || copy_from_user(regBuf, buffer, u4CopyBufSize))
		return -EFAULT;

	if (sscanf(
	    regBuf,
	    "%x %x",
	    &sensorReg.RegAddr,
	    &sensorReg.RegData) == 2) {

		imgsensor_sensor_feature_control(
		    psensor,
		    SENSOR_FEATURE_SET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

		imgsensor_sensor_feature_control(
		    psensor,
		    SENSOR_FEATURE_GET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));


		pr_debug(
		    "write addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

		snprintf(
		    mtk_i2c_dump,
		    sizeof(mtk_i2c_dump),
		    "addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

	} else if (kstrtouint(regBuf, 16, &sensorReg.RegAddr) == 0) {
		imgsensor_sensor_feature_control(
		    psensor,
		    SENSOR_FEATURE_GET_REGISTER,
		    (MUINT8 *) &sensorReg,
		    (MUINT32 *) sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

		pr_debug(
		    "read addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);

		snprintf(
		    mtk_i2c_dump,
		    sizeof(mtk_i2c_dump),
		    "addr = 0x%08x, data = 0x%08x\n",
		    sensorReg.RegAddr,
		    sensorReg.RegData);
	}

	return count;
}

/*******************************************************************************
  * CAMERA_HW_Reg_Show()
  * Used to dump some critical sensor register
  ********************************************************************************/
static int  CAMERA_HW_Reg_Show(struct seq_file *m, void *v)
{
    int i=0x00;
    MSDK_SENSOR_REG_INFO_STRUCT TP2850_sensorReg;
    MSDK_SENSOR_REG_INFO_STRUCT N4_sensorReg;
    seq_printf(m, "CAMERA_HW_Reg_Show\n");
    memset(&TP2850_sensorReg, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
    memset(&N4_sensorReg, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

    if(0 == proc_flag){
        if (tp2850_pfFunc != NULL) {
            while(i <= 0xff){
                TP2850_sensorReg.RegAddr = i;
                tp2850_pfFunc->SensorFeatureControl(SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&TP2850_sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
                seq_printf(m,"[TP2850]read addr = 0x%02x, data = 0x%02x\n", TP2850_sensorReg.RegAddr, TP2850_sensorReg.RegData);
                i++;
            }
        }
    } else if (1 == proc_flag){
        seq_printf(m,"[TP2850]read addr = 0x%02x, data = 0x%02x\n", proc_tempSensor.RegAddr, proc_tempSensor.RegData);
    } else if (-1 == proc_flag) {
        if (n4_pfFunc != NULL) {
            while(i <= 0xff){
                N4_sensorReg.RegAddr = i;
                n4_pfFunc->SensorFeatureControl(SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&N4_sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
                seq_printf(m,"[N4]read addr = 0x%02x, data = 0x%02x\n", N4_sensorReg.RegAddr, N4_sensorReg.RegData);
                i++;
            }
        }
    } else if (-2 == proc_flag) {
        seq_printf(m,"[N4]read addr = 0x%02x, data = 0x%02x\n", proc_tempSensor.RegAddr, proc_tempSensor.RegData);
    }
    return 0;
}

static int CAMERA_HW_Reg_open(struct inode *inode, struct file *file)
{
    return single_open(file, CAMERA_HW_Reg_Show, NULL);
};

static ssize_t CAMERA_HW_Reg_Write(struct file *pFile, const char __user *buffer, size_t count,
				   loff_t *ppos)
{
	char regBuf[64] = {'\0'};
	int reg;
	u32 u4CopyBufSize = (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1);

	MSDK_SENSOR_REG_INFO_STRUCT sensorReg;
	memset(&sensorReg, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

	if (copy_from_user(regBuf, buffer, u4CopyBufSize)) {
		return -EFAULT;
	}

	if (sscanf(regBuf, "%d %x %x", &reg, &sensorReg.RegAddr, &sensorReg.RegData) == 3) {
		if(2850 == reg){
			if (tp2850_pfFunc != NULL) {
				tp2850_pfFunc->SensorFeatureControl(SENSOR_FEATURE_SET_REGISTER, (MUINT8 *)&sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
				proc_tempSensor.RegAddr = sensorReg.RegAddr;
				tp2850_pfFunc->SensorFeatureControl(SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&proc_tempSensor, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
				proc_flag = 1;
			}
		} else if (4 == reg) {
			if (n4_pfFunc != NULL) {
				n4_pfFunc->SensorFeatureControl(SENSOR_FEATURE_SET_REGISTER, (MUINT8 *)&sensorReg, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
				proc_tempSensor.RegAddr = sensorReg.RegAddr;
				n4_pfFunc->SensorFeatureControl(SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&proc_tempSensor, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
				proc_flag = -2;
			}
		}
	} else if (sscanf(regBuf, "%d %x", &reg, &proc_tempSensor.RegAddr) == 2) {
		if(2850 == reg){
			if (tp2850_pfFunc != NULL) {
				tp2850_pfFunc->SensorFeatureControl(SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&proc_tempSensor, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
				proc_flag = 1;
			}
		} else if (4 == reg){
			if (n4_pfFunc != NULL) {
				n4_pfFunc->SensorFeatureControl(SENSOR_FEATURE_GET_REGISTER, (MUINT8 *)&proc_tempSensor, (MUINT32 *)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
				proc_flag = -2;
			}
		}
	}else if(sscanf(regBuf, "%d",&reg) == 1){
		if(2850 == reg){
			proc_flag = 0;
		} else if (4 == reg) {
			proc_flag = -1;
		}
	}
	return count;
}

/* Camera information */
static int subsys_camera_info_read(struct seq_file *m, void *v)
{
	pr_debug("%s %s\n", __func__, mtk_ccm_name);
	seq_printf(m, "%s\n", mtk_ccm_name);
	return 0;
};
static int subsys_camsensor_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", mtk_i2c_dump);
	return 0;
};

static int proc_camera_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, subsys_camera_info_read, NULL);
};

static int proc_camsensor_open(struct inode *inode, struct file *file)
{
	return single_open(file, subsys_camsensor_read, NULL);
};

static int imgsensor_proc_status_read(struct seq_file *m, void *v)
{
	char status_info[IMGSENSOR_STATUS_INFO_LENGTH];

	snprintf(status_info, sizeof(status_info),
			"ERR_L0, %x\n",
			*((uint32_t *)(&pgimgsensor->status)));
	seq_printf(m, "%s\n", status_info);
	return 0;
};

static int imgsensor_proc_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, imgsensor_proc_status_read, NULL);
};

static const struct file_operations fcamera_proc_fops_status = {
	.owner = THIS_MODULE,
	.open = imgsensor_proc_status_open,
	.read = seq_read,
};

static const struct file_operations fcamera_proc_fops1 = {
	.owner = THIS_MODULE,
	.open = proc_camera_info_open,
	.read = seq_read,
};

static const struct file_operations fcamera_proc_fops = {
	.owner = THIS_MODULE,
	.open = CAMERA_HW_Reg_open,
	.read = seq_read,
	.write = CAMERA_HW_Reg_Write,
	.release = single_release,
};

static const struct file_operations fcamera_proc_fops2 = {
	.owner = THIS_MODULE,
	.read = seq_read,
	.open = proc_camsensor_open,
	.write = CAMERA_HW_Reg_Debug2
};

static const struct file_operations fcamera_proc_fops3 = {
	.owner = THIS_MODULE,
	.read = seq_read,
	.open = proc_camsensor_open,
	.write = CAMERA_HW_Reg_Debug3
};

static const struct file_operations fcamera_proc_fops4 = {
	.owner = THIS_MODULE,
	.read = seq_read,
	.open = proc_camsensor_open,
	.write = CAMERA_HW_Reg_Debug4
};


static const struct file_operations fcamera_proc_fops_set_pdaf_type = {
	.owner = THIS_MODULE,
	.open = proc_SensorType_open,
	.read = seq_read,
	.write = proc_SensorType_write
};



enum IMGSENSOR_RETURN imgsensor_proc_init(void)
{
	memset(mtk_ccm_name, 0, camera_info_size);

	proc_create("driver/camsensor", 0664, NULL, &fcamera_proc_fops);
	proc_create("driver/camsensor2", 0664, NULL, &fcamera_proc_fops2);
	proc_create("driver/camsensor3", 0664, NULL, &fcamera_proc_fops3);
	proc_create("driver/camsensor4", 0664, NULL, &fcamera_proc_fops4);
	proc_create(
	    "driver/pdaf_type", 0664, NULL, &fcamera_proc_fops_set_pdaf_type);
	proc_create(PROC_SENSOR_STAT, 0664, NULL, &fcamera_proc_fops_status);

	/* Camera information */
	proc_create(PROC_CAMERA_INFO, 0664, NULL, &fcamera_proc_fops1);
#if defined(TP2850_YUV)
	TP2850_YUV_SensorInit(&tp2850_pfFunc);
#endif
#if defined(N4_YUV)
	N4_YUV_SensorInit(&n4_pfFunc);
#endif
	memset(&proc_tempSensor, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

	return IMGSENSOR_RETURN_SUCCESS;
}
