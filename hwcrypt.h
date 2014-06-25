/*
 * Copyright (c) 2014, Mihai Cristea, REDANS SRL
 * email at: mihai _AT_ redans -DOT- eu
 * GPLv3 License applies
 */

#define DEVICE_NAME "serial"
#define CLASS_NAME "hwcrypt"
 
#define HWCRYPT_MSG_MAX  32

#define AUTHOR "Mihai Cristea <mihai AT redans DOT eu>"
#define DESCRIPTION "'atmel crypto' sample device driver"
#define VERSION "0.1"
 
/* macros for printk */
#define dbg(format, arg...) do { if (debug) pr_info(CLASS_NAME ": %s: " format, __FUNCTION__, ## arg); } while (0)
#define err(format, arg...) pr_err(CLASS_NAME ": " format, ## arg)
#define info(format, arg...) pr_info(CLASS_NAME ": " format, ## arg)
