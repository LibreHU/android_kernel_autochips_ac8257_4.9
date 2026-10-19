#ifndef __ATC_CUSTOMIZE_H__
#define __ATC_CUSTOMIZE_H__

#define CUSTOMIZE_TS		"customize_ts"
#define TOUCHPAD_KEYSUPPORT
#define TOUCHPAD_EXTERNFLAG

#ifdef  TOUCHPAD_KEYSUPPORT
#define MAX_TOUCHPAD_KEYNUM 0x8
#define TPKEY_ARRAY_SIZE     0x20
#endif

#define TP_READ_SIZE 34
#define TP_MT_TOUCH  5

struct touchpoint {
    int point_id;
    int point_status;
    int x;
    int y;
};

struct customize_ts_data {
    struct i2c_client *client;
    struct input_dev *input_dev;
    int abs_x_max;
    int abs_y_max;
    bool swapped_x_y;
    bool inverted_x;
    bool inverted_y;
    unsigned int max_touch_num;
    unsigned int int_trigger_type;
    struct gpio_desc *gpiod_int;
    struct gpio_desc *gpiod_rst;
    u16 id;
    u16 version;
    struct completion firmware_loading_complete;
    unsigned long irq_flags;
    /* Extend */
	unsigned char use_irq;
	struct hrtimer timer;
	struct workqueue_struct *customize_wq;
	struct work_struct  work;
#ifdef TOUCHPAD_EXTERNFLAG
    unsigned int tp_extern;
#endif
#ifdef TOUCHPAD_KEYSUPPORT
    unsigned int buttons;
    unsigned int bt_status;
    u32 key_event[MAX_TOUCHPAD_KEYNUM];
    u32 key_range[MAX_TOUCHPAD_KEYNUM][4];
#endif
    void * private_data;
	//int touchscreen_reset(struct customize_ts_data *ts);
	//int touchscreen_test(struct i2c_client *client);
	//int touchscreen_read_verision(struct customize_ts_data *ts);
	int (*i2c_read16)(struct i2c_client *client,u16 reg, u8 *buf, int len);
	int (*i2c_write16)(struct i2c_client *client, u16 reg, const u8 *buf,
			unsigned len);
	int (*i2c_read)(struct i2c_client *client,char reg, u8 *buf, int len);
	int (*i2c_write)(struct i2c_client *client,char reg, const u8 *buf,
			unsigned len);
	int (*i2c_read_row)(struct i2c_client *client, u8 *buf, int len);
	int (*handle_irq)(struct customize_ts_data *ts);
	int (*init_touchscreen)(struct customize_ts_data *ts);
	int (*deinit_touchscreen)(struct customize_ts_data *ts);
	int (*inputdev_config)(struct customize_ts_data *ts);

	//void * sv_private_data;
	int touch_status;
	struct touchpoint cursor_last;
};

#define CUSTOMIZE_GPIO_INT_NAME        "irq"
#define CUSTOMIZE_GPIO_RST_NAME        "reset"

#define CUSTOMIZE_MAX_HEIGHT        4096
#define CUSTOMIZE_MAX_WIDTH        4096
#define CUSTOMIZE_INT_TRIGGER        1
#define CUSTOMIZE_CONTACT_SIZE        8
#define CUSTOMIZE_MAX_CONTACTS        10

#define CUSTOMIZE_CONFIG_MAX_LENGTH    240
#define CUSTOMIZE_CONFIG_911_LENGTH    186
#define CUSTOMIZE_CONFIG_967_LENGTH    228

/* Register defines */
#define RESOLUTION_LOC        1
#define MAX_CONTACTS_LOC    5
#define TRIGGER_LOC        6

#endif
