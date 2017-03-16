/* i like you  mother
*/
#include <linux/module.h>  
#include <linux/types.h>  
#include <linux/fs.h>  
#include <linux/errno.h>  
#include <linux/mm.h>  
#include <linux/cdev.h>  
#include <linux/sched.h> 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h> 
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h> 



#define DEVICE_NAME "IOCTTT"    //定义设备名  
#define LED_MAJOR 0             //定义主设备号  
#define LED_MINOR 0     
#define LED_MAGIC 0xBB            //定义幻数  
//定义操作    
#define LED_ON  _IOW(LED_MAGIC,1,int)
#define LED_OFF _IOW(LED_MAGIC,2,int)

static struct class *usb_class;
static struct device *dev;
static const char shortname [] = "test_usb1";


static int led_major = LED_MAJOR; //设置主设备号
struct statusled_dev{  //define dev struct
	unsigned char value;
	struct semaphore sem;   //定义信号量
	struct cdev cdev;
};
struct led_gpio {
	unsigned base;
	unsigned gpio;
	#define GPI_DIR_OUT       0x00000001
	#define GPI_DIR_IN 	 0x00000002
	#define GPI_INIT_LOW 	 0x00000004
	#define GPI_INIT_HIGH	 0x00000008
unsigned long flags;
	const char *label;
};
static struct led_gpio leds_gpio[]={
	{300,7,GPI_DIR_OUT | GPI_INIT_LOW,"RUN LED"},
	{300,5,GPI_DIR_OUT | GPI_INIT_LOW,"ERRO LED"},
	{300,10,GPI_DIR_OUT | GPI_INIT_HIGH,"ERO LED"},
	{300,18,GPI_DIR_OUT | GPI_INIT_HIGH,"ERO1 LED"},
	//{},
};
struct statusled_dev *statusled_devp = NULL;
static int leds_open(struct inode *inode, struct file *filp)
{
	struct statusled_dev *dev;  
    dev = container_of(inode->i_cdev, struct statusled_dev, cdev); //通过结构体成员指针找到对应结构体的指针  
    filp->private_data = dev;    //将设备结构体指针赋给文件私有数据指针   
    //TODO: Status tips
	printk("statusled device opened\n"); 
    return 0;  
}
static int leds_release(struct inode *inode, struct file *filp)
{
 
    //TODO 
	printk("statusled device closing\n");
    return 0;  
}
/*
*	leds_write
*	I want to modify all the leds status,so,there ervery one ....bits
*	note: don't use size.so,the max led number is 8;
*/
static ssize_t leds_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	int i;	
	struct statusled_dev *dev = filp->private_data; 
	int num =  ARRAY_SIZE(leds_gpio);
    down(&dev->sem);    
	copy_from_user(&(dev->value),buf,1);
        switch (dev->value){
        case 48:
      gpio_set_value(307,0);
        printk("Monday\n");  break;
        case 49:
        gpio_set_value(307,1);
        printk("Tuesday\n");   break;
        case 50:
gpio_set_value(305,0);
      
printk("Wednesday\n");  break;
        case 51:
gpio_set_value(305,1);
      
printk("Thursday\n");  break;
        case 52:
gpio_set_value(310,0);
  
printk("Friday\n");  break;
        case 53:
gpio_set_value(310,1);
  
printk("Saturday\n");  break;
        case 54:printk("Sunday\n");  break;
        default:printk("error\n");
    }
    up(&dev->sem);  
	printk(KERN_ALERT "status LED device write ,dev->value: %d\n",dev->value);
    return size;  
}
static int leds_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned int arg)
{
	if(arg > 4){
		return -EINVAL;
	}
	printk("%d_ad_%d",cmd,arg);
	switch (cmd)  
    {  

        case 0:   
			gpio_direction_output(leds_gpio[arg].base + leds_gpio[arg].gpio,0);



			printk("123");
            return 0;  
		case 1:  
			gpio_direction_output(leds_gpio[arg].base + leds_gpio[arg].gpio,1);
           printk("545");
			return 0;  
			
        default:  
            return -EINVAL;  
    }  
}
//file_operations  
static struct file_operations leds_fops = {  
    .owner = THIS_MODULE,  
    .open = leds_open,  
    .release = leds_release,   
    .write = leds_write,  
  .unlocked_ioctl  = leds_ioctl,  
};  
	 
static int leds_request(struct led_gpio *leds,size_t num){
	int i,ret = 0;
	for(i=0;i<num;i++){
		ret = gpio_request(leds[i].gpio + leds[i].base,leds[i].label);
		if(ret){
			printk("%s:request gpio %d faild\n",leds[i].label,leds[i].gpio);
			return ret;
		}
		if((leds[i].flags) & GPI_DIR_OUT) {// output
			if((leds[i].flags) & GPI_INIT_LOW){
				gpio_direction_output(leds_gpio[i].base + leds[i].gpio,0);
			}
			else{
				gpio_direction_output(leds_gpio[i].base + leds[i].gpio,1);
			}
		}
		else if((leds[i].flags) & GPI_DIR_IN){ //input
			gpio_direction_input(leds_gpio[i].base + leds[i].gpio);
		}
		else{
			return -EINVAL;
		}
	}
	return ret;
}
static int leds_free(struct led_gpio *leds,size_t num){
	int  i,ret = 0;
	for(i=0; i<num;i++){
		gpio_free(leds[i].base + leds[i].gpio);}
	return ret;
}
// inint cdev struct and register a char dev
static void statusled_setup_cdev(struct statusled_dev *dev,int index)
{
	int err,devno=MKDEV(led_major, LED_MINOR+index);
	cdev_init(&dev->cdev,&leds_fops);
    dev->cdev.owner = THIS_MODULE;  
    dev->cdev.ops = &leds_fops;
	err = leds_request(&leds_gpio,ARRAY_SIZE(leds_gpio));
	if(err){
		printk(KERN_NOTICE "Error:request gpio faild \n");}
	err = cdev_add(&dev->cdev,devno,1);
	if(err)
		printk(KERN_NOTICE "Error %d adding statusled%d\n",err,index);
}
static  int __init statusleds_init(void)
{
    int result;  
    dev_t devno = MKDEV(led_major, LED_MINOR); //创建设备号   
     //注册设备号  
     if (led_major)  {  

        result = register_chrdev_region(devno, 1, DEVICE_NAME);  
    }  
    else {  
        result = alloc_chrdev_region(&devno, LED_MINOR, 1, DEVICE_NAME);  
        led_major = MAJOR(devno);  
    }  

	usb_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (result < 0) {  
        printk("Can't register\n");  
        return result;  
    }  
	 dev=device_create(usb_class, NULL, devno, NULL,DEVICE_NAME);
	 
	statusled_devp = kmalloc(sizeof(struct statusled_dev), GFP_KERNEL);  
    if (!statusled_devp)  {  
        result = -ENOMEM;  
        unregister_chrdev_region(devno, 1);  
        return result;  
    }  
    memset(statusled_devp, 0, sizeof(struct statusled_dev));  
	statusled_setup_cdev(statusled_devp,0);





if (IS_ERR(usb_class))
{
printk( KERN_DEBUG "class_create error\n" );
return -1;
}
	
	sema_init(&statusled_devp->sem,1);
	printk("Status LEDs Function is enable\n");
	return 0;
}
static  void __exit statusleds_exit(void){
	
	leds_free(&leds_gpio,ARRAY_SIZE(leds_gpio));
	cdev_del(&statusled_devp->cdev);  
    kfree(statusled_devp);  
	
	device_destroy(usb_class,MKDEV(led_major, LED_MINOR));
   
	
    unregister_chrdev_region(MKDEV(led_major, LED_MINOR), 1); 
	

	 class_destroy(usb_class);

	 
	printk("Status LEDs Function is disable\n");
}
module_init(statusleds_init);  
module_exit(statusleds_exit);  

MODULE_AUTHOR("Caijun");  
MODULE_DESCRIPTION("Status LEDs Driver"); // 一些描述信息  
MODULE_LICENSE("GPL");
