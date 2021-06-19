#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/platform_device.h>
#include<linux/gpio/consumer.h>
#include<linux/interrupt.h>
#include<linux/of.h>
#include<linux/delay.h>
#include<linux/miscdevice.h>
#include<linux/fs.h>
#include<linux/poll.h>

uint8_t key_pressed=false,ledd = 0;
struct gpio_desc *key,*gren_led,*red_led;

DECLARE_WAIT_QUEUE_HEAD(wait_queue_key_press);


static irqreturn_t key_isr(int irq,void *data)
{
    pr_info("Button pressed\n");

    wake_up(&wait_queue_key_press);
    key_pressed = true;
    if(ledd == 0x00){
	gpiod_set_value(gren_led,1);
	gpiod_set_value(red_led,0);
	ledd = 0x01;
    }
    else if(ledd == 0x01){
	gpiod_set_value(gren_led,0);
	gpiod_set_value(red_led,1);
	ledd = 0x00;
    }
    return IRQ_HANDLED;

}

static int key_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int key_release(struct inode *inode,struct file *filp)
{
    return 0;
}

ssize_t key_read(struct file *filp,char __user *buff,size_t count,loff_t *offset)
{
    pr_info("key_read\n");

    if(copy_to_user(buff,"Key-pressed",12) > 0){
	pr_info("Copy to user fail\n");
    }

    return count;
}

static unsigned int key_poll(struct file *filp,poll_table *wait)
{
    unsigned int reval_mask = 0;

    pr_info("key_poll \n");
    poll_wait(filp,&wait_queue_key_press,wait);
    
    if(key_pressed == true){
	key_pressed = false;
	reval_mask = (POLLIN | POLLRDNORM);
    }
    return reval_mask;
}

static struct file_operations key_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .release = key_release,
    .read = key_read,
    .poll = key_poll,
};

static struct miscdevice key_miscdevice = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "key-led-dev",
    .fops = &key_fops,
};


static int ledkey_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    int irq,ret;
    

    pr_info("ledkey_probe called\n");

    gren_led = devm_gpiod_get_index(dev,"led",0,GPIOD_OUT_LOW);
    if(IS_ERR(gren_led)){
	pr_err("devm_gpiod_get failed\n");
	return PTR_ERR(gren_led);
    }
    red_led = devm_gpiod_get_index(dev,"led",1,GPIOD_OUT_LOW);
    if(IS_ERR(red_led)){
	pr_err("devm_gpiod_get failed\n");
	return PTR_ERR(red_led);
    }
    key = devm_gpiod_get(dev,"key",GPIOD_IN);
    if(IS_ERR(key)){
	pr_err("devm_gpiod_get failed\n");
	return PTR_ERR(key);
    }
    gpiod_direction_output(gren_led,0);
    gpiod_direction_output(red_led,0);
    gpiod_set_debounce(key,200);
    irq = gpiod_to_irq(key);
    if(irq < 0){
	pr_err("gpiod_to_irq error\n");
	return irq;
    }

    pr_info("irq = %d\n",irq);

    ret = devm_request_irq(dev,irq,key_isr,IRQF_TRIGGER_RISING,
			    "Keys",dev);

    if(ret){
	pr_info("devm_request_irq failed\n");
	return ret;
    }
    
    ret = misc_register(&key_miscdevice);
    if(ret){
	pr_err("Could not register misc device\n");
	return ret;
    }
    
    init_waitqueue_head(&wait_queue_key_press);

    pr_info("key-led-dev got minor number %d\n",key_miscdevice.minor);

    return 0;
}


static int ledkey_remove(struct platform_device *pdev)
{
    pr_info("key-led-dev exit\n");
    misc_deregister(&key_miscdevice);
    return 0;
}


static const struct of_device_id ledkeyof_ids[] = {
    {.compatible = "led-key"},
    {},
};

MODULE_DEVICE_TABLE(of,ledkeyof_ids);

static struct platform_driver ledkey_driver = {
    .probe = ledkey_probe,
    .remove = ledkey_remove,
    .driver = {
	.name = "led-key",
	.of_match_table = ledkeyof_ids,
	.owner = THIS_MODULE,
    }
};


module_platform_driver(ledkey_driver);
MODULE_AUTHOR("Jaggu");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple driver for gpio control");

