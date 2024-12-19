/*
 * Copyright 2024 by Lukas Fischer
 * 
 * Template for creating a network linux network device from the SX1262
 * LoRa chip.  LoRa-Fi already exists in some form or another.
 * 
 * 
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/config.h>

#include <linux/netdevice.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lukas A. Fischer");
MODULE_DESCRIPTION("Enables the use of the SX1262 LoRa as a net_device");
MODULE_VERSION(0.1);

struct net_device * sx1262;

int sx1262_init_module(void)
{
	// TODO: probe/self-test hardware
	strcpy(sx1262.name, "lora%d");
	register_netdev(&sx1262);
	return 0;
}

int sx1262_init(struct net_device *dev)
{
	net_device = alloc_netdev(sizeof(struct sx1262_priv), "lora%d", sx1262_init
	dev->open = sx1262_open;
	dev->stop = sx1262_release;
	dev->hard_start_xmit = sx1262_xmit;
	return 0;
}

void sx1262_cleanup(void)
{
	unregister_netdev(&sx1262);
	return;
}

module_init(sx1262_init_module);
module_exit(sx1262_cleanup);
