//PROJECT

/* Necessary includes for device drivers */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */
#include <asm/param.h> /* include HZ */
#include <linux/string.h> /* string operations */
#include <linux/timer.h> /* timer gizmos */
#include <linux/list.h> /* include list data struct */
#include <asm/gpio.h>
#include <linux/interrupt.h>
#include <asm/arch/pxa-regs.h>
#include <asm-arm/arch/hardware.h>



#define BUTTON0 17
#define BUTTON1 101
#define BUTTON2 117
#define BUTTON3 118


MODULE_LICENSE("MIT");

/* Declaration of memory.c functions */
static int mycar_open(struct inode *inode, struct file *filp);
static int mycar_release(struct inode *inode, struct file *filp);
static ssize_t mycar_read(struct file *filp,char *buf, size_t count, loff_t *f_pos);
static ssize_t mycar_write(struct file *filp,const char *buf, size_t count, loff_t *f_pos);
static void mycar_exit(void);
static int mycar_init(void);

static void MOTORSetter(char d); //set how to spin the motors


static void timer_handler(unsigned long data);

/*irqreturn_t gpio_irq_b0(int irq, void *dev_id, struct pt_regs *regs);
irqreturn_t gpio_irq_b1(int irq, void *dev_id, struct pt_regs *regs);
irqreturn_t gpio_irq_b2(int irq, void *dev_id, struct pt_regs *regs);
irqreturn_t gpio_irq_b3(int irq, void *dev_id, struct pt_regs *regs);*/


/* Structure that declares the usual file */
/* access functions */
struct file_operations mycar_fops = {
	read: mycar_read,
	write: mycar_write,
	open: mycar_open,
	release: mycar_release
};

/* Declaration of the init and exit functions */
module_init(mycar_init);
module_exit(mycar_exit);

static unsigned capacity = 2560;
static unsigned bite = 2560;
module_param(capacity, uint, S_IRUGO);
module_param(bite, uint, S_IRUGO);

/* Global variables of the driver */
/* Major number */
static int mycar_major = 61;

/* Buffer to store data */
static char *mycar_buffer;
/* length of the current message */
static int mycar_len;




/* timer pointer */
static struct timer_list the_timer[1];

unsigned MOTOR[4];
int timePer = 500;

char command = 's';
char speed = 'H';

//int command=4;
//int speed;



static int mycar_init(void)
{
	int result, ret;
	MOTOR[0] = 28; //the GPIO pins
	MOTOR[1] = 29;
	MOTOR[2] = 30;
	MOTOR[3] = 31;
	
	//Buttons are not assigned as ionput, they are used for Interrupt things.
	//gpio_direction_input(button[0]); //set button as input direction
	//gpio_direction_input(button[1]); //set button as input direction

	//set output MOTORs
	gpio_direction_output(MOTOR[0],0); 
	gpio_direction_output(MOTOR[1],0);
	gpio_direction_output(MOTOR[2],0);
	gpio_direction_output(MOTOR[3],0);

	pxa_gpio_set_value(MOTOR[0],0); //
	pxa_gpio_set_value(MOTOR[1],0); //
	pxa_gpio_set_value(MOTOR[2],0); //
	pxa_gpio_set_value(MOTOR[3],0); //

	
	
	/* Registering device */
	result = register_chrdev(mycar_major, "mycar", &mycar_fops);
	if (result < 0)
	{
		printk(KERN_ALERT "mycar: cannot obtain major number %d\n", mycar_major);
		return result;
	}

	/* Allocating mycar for the buffer */
	mycar_buffer = kmalloc(capacity, GFP_KERNEL);
	/* Check if all right */
	if (!mycar_buffer)
	{ 
		//printk(KERN_ALERT "Insufficient kernel memory\n"); 
		result = -ENOMEM;
		goto fail; 
	} 
	
	memset(mycar_buffer, 0, capacity);
	mycar_len = 0;
	//+printk(KERN_ALERT "Inserting mycar module\n"); 

	setup_timer(&the_timer[0], timer_handler, 0);
	ret = mod_timer(&the_timer[0], jiffies + msecs_to_jiffies(timePer/2));

	/*pxa_gpio_mode(BUTTON0 | GPIO_IN);
	pxa_gpio_mode(BUTTON1 | GPIO_IN);
	pxa_gpio_mode(BUTTON2 | GPIO_IN);
	pxa_gpio_mode(BUTTON3 | GPIO_IN);
	int irq_b0= IRQ_GPIO(BUTTON0);
	int irq_b1 = IRQ_GPIO(BUTTON1);
	int irq_b2 = IRQ_GPIO(BUTTON2);
	int irq_b3 = IRQ_GPIO(BUTTON3);

	if (request_irq(irq_b0, &gpio_irq_b0, SA_INTERRUPT | SA_TRIGGER_RISING,"mycar", NULL) != 0 ) 
	{
                printk ( "\nirq_b0 not acquired \n" );
                return -1;
        }
	else
	{
                printk ( "\nirq %d acquired successfully for BUTTON 0 \n", irq_b0 );
	}

	if (request_irq(irq_b1, &gpio_irq_b1, SA_INTERRUPT | SA_TRIGGER_RISING,"mycar", NULL) != 0 ) 
	{
                printk ( "\nirq_b1 not acquired \n" );
                return -1;
        }
	else
	{
                printk ( "\nirq %d acquired successfully BUTTON 1 \n", irq_b1 );
	}

	if (request_irq(irq_b2, &gpio_irq_b2, SA_INTERRUPT | SA_TRIGGER_RISING,"mycar", NULL) != 0 ) 
	{
                printk ( "\nirq_b2 not acquired \n" );
                return -1;
        }
	else
	{
                printk ( "\nirq %d acquired successfully BUTTON 2\n", irq_b2 );
	}

	if (request_irq(irq_b3, &gpio_irq_b3, SA_INTERRUPT | SA_TRIGGER_RISING,"mycar", NULL) != 0 ) 
	{
                printk ( "\nirq_b2 not acquired \n" );
                return -1;
        }
	else
	{
                printk ( "\nirq %d acquired successfully BUTTON 3 \n", irq_b3 );
	}*/

	// PWM Set up
	//turn on CKEN
	pxa_set_cken(CKEN0_PWM0, 1);
	pxa_set_cken(CKEN1_PWM1, 1);
	//CKEN |= 0x1;
	//GAFR0_U	= 0x00000002;
	pxa_gpio_mode(GPIO16_PWM0_MD);
	pxa_gpio_mode(GPIO17_PWM1_MD);

	PWM_CTRL0   = 0x00000020;
	PWM_PWDUTY0 = 0x00000fff;
	PWM_PERVAL0 = 0x000003ff;

	PWM_CTRL1   = 0x00000020;
	PWM_PWDUTY1 = 0x00000fff;
	PWM_PERVAL1 = 0x000003ff;

return 0;

fail: 
	mycar_exit(); 
	return result;
}

static void mycar_exit(void)
{

	gpio_direction_output(MOTOR[0],0); 
	gpio_direction_output(MOTOR[1],0);
	gpio_direction_output(MOTOR[2],0);
	gpio_direction_output(MOTOR[3],0);

	PWM_PWDUTY0 = 0x00000000;
	PWM_PWDUTY1 = 0x00000000;

	/* Freeing the major number */
	unregister_chrdev(mycar_major, "mycar");

	/* Freeing buffer memory */
	if (mycar_buffer)
		kfree(mycar_buffer);
	/* Freeing timer list */
	//if (the_timer)
	//	kfree(the_timer);
	
	if (timer_pending(&the_timer[0]))
		del_timer(&the_timer[0]);

	/*free_irq(IRQ_GPIO(BUTTON0), NULL);
	free_irq(IRQ_GPIO(BUTTON1), NULL);
	free_irq(IRQ_GPIO(BUTTON2), NULL);
	free_irq(IRQ_GPIO(BUTTON3), NULL);*/

	//printk(KERN_ALERT "Removing mycar module\n");

}

static int mycar_open(struct inode *inode, struct file *filp)
{
	/*printk(KERN_INFO "open calMOTOR: process id %d, command %s\n",
		current->pid, current->comm);*/
	/* Success */
	return 0;
}

static int mycar_release(struct inode *inode, struct file *filp)
{
	/*printk(KERN_INFO "release called: process id %d, command %s\n",
		current->pid, current->comm);*/
	/* Success */
	return 0;
}



static ssize_t mycar_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int temp;
	//int digitCount = 0;
	char tbuf[256];


	//printk(KERN_ALERT "In write\n");
		
	
	/* end of buffer reached */
	if (*f_pos >= capacity)
	{
		return -ENOSPC;
	}

	/* do not eat more than a bite */
	if (count > bite) count = bite;

	/* do not go over the end */
	if (count > capacity - *f_pos)
		count = capacity - *f_pos;

	if (copy_from_user(mycar_buffer + *f_pos, buf, count))
	{
		return -EFAULT;
	}


	temp = *f_pos;

	if (mycar_buffer[temp] == 's' && count < 2) 
	{
		command = 's';
	}
	else if (mycar_buffer[temp] == 'e' && count < 6) 
	{
		command = 's';
	}	
	else if (mycar_buffer[temp] == 'f' && mycar_buffer[temp+1] == 'H' && count < 3) 
	{
		command = 'f';
		speed = 'H';
	}
	else if (mycar_buffer[temp] == 'f' && mycar_buffer[temp+1] == 'M' && count < 3) 
	{
		command = 'f';
		speed = 'M';
	}
	else if (mycar_buffer[temp] == 'f' && mycar_buffer[temp+1] == 'L' && count < 3) 
	{
		command = 'f';
		speed = 'L';
	}
	else if (mycar_buffer[temp] == 'b' && mycar_buffer[temp+1] == 'H' && count < 3) 
	{
		command = 'b';
		speed = 'M';
	}
	else if (mycar_buffer[temp] == 'b' && mycar_buffer[temp+1] == 'M' && count < 3) 
	{
		command = 'b';
		speed = 'M';
	}
	else if (mycar_buffer[temp] == 'b' && mycar_buffer[temp+1] == 'L' && count < 3) 
	{
		command = 'b';
		speed = 'L';
	}
	//right
	else if (mycar_buffer[temp] == 'r' && mycar_buffer[temp+1] == 'H' && count < 3) 
	{
		command = 'r';
		speed = 'H';
	}
	else if (mycar_buffer[temp] == 'r' && mycar_buffer[temp+1] == 'M' && count < 3) 
	{
		command = 'r';
		speed = 'M';
	}
	//left
	else if (mycar_buffer[temp] == 'l' && mycar_buffer[temp+1] == 'H' && count < 3) 
	{
		command = 'l';
		speed = 'H';
	}
	else if (mycar_buffer[temp] == 'l' && mycar_buffer[temp+1] == 'M' && count < 3) 
	{
		command = 'l';
		speed = 'M';
	}
///////////////////////////////////

	//printk(KERN_ALERT "Kernel: command =%c\n",command);

	*f_pos += count;
	mycar_len = *f_pos;
	return count;
}

static ssize_t mycar_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{ 
	
	char tbuf[1012];

	//sprintf(tbuf,"%d\t%d\t%s\t%s\t%s", countVal, period, state, dir, bLevel);
	sprintf(tbuf,"apple\n");

	strcpy(mycar_buffer,tbuf);

	//do not go over then end
	if (count > 2560 - *f_pos)
		count = 2560 - *f_pos;
	
	if (copy_to_user(buf,mycar_buffer + *f_pos, count))
	{
		return -EFAULT;	
	}
	
	// Changing reading position as best suits 
	*f_pos += count; 
	return count; 
}




static void timer_handler(unsigned long data) 
{
	MOTORSetter(command);
	mod_timer(&the_timer[0],jiffies + msecs_to_jiffies(timePer/2));

}	


static void MOTORSetter(char d)
{
	//printk("command = %c, speed = %c\n",d,speed);

	if (d == 'r')
	{
		pxa_gpio_set_value(MOTOR[0],0);
		pxa_gpio_set_value(MOTOR[1],1);
		pxa_gpio_set_value(MOTOR[2],0);
		pxa_gpio_set_value(MOTOR[3],1);
	}
	else if (d == 'l')
	{
		pxa_gpio_set_value(MOTOR[0],1);
		pxa_gpio_set_value(MOTOR[1],0);
		pxa_gpio_set_value(MOTOR[2],1);
		pxa_gpio_set_value(MOTOR[3],0);
	}
	else if (d == 'b')
	{
		pxa_gpio_set_value(MOTOR[0],0);
		pxa_gpio_set_value(MOTOR[1],1);
		pxa_gpio_set_value(MOTOR[2],1);
		pxa_gpio_set_value(MOTOR[3],0);
	}
	else if (d == 'f')
	{
		pxa_gpio_set_value(MOTOR[0],1);
		pxa_gpio_set_value(MOTOR[1],0);
		pxa_gpio_set_value(MOTOR[2],0);
		pxa_gpio_set_value(MOTOR[3],1);
	}
	else if (d == 's')
	{
		pxa_gpio_set_value(MOTOR[0],0);
		pxa_gpio_set_value(MOTOR[1],0);
		pxa_gpio_set_value(MOTOR[2],0);
		pxa_gpio_set_value(MOTOR[3],0);
	}

	if (speed == 'L')
	{
		PWM_PWDUTY0 = 0x0000018f;
		PWM_PWDUTY1 = 0x0000018f;
	}
	else if (speed == 'M')
	{
		PWM_PWDUTY0 = 0x000002cf;
		PWM_PWDUTY1 = 0x000002cf;
	}
	else if (speed == 'H')
	{
		PWM_PWDUTY0 = 0x000003ff;
		PWM_PWDUTY1 = 0x000003ff;
	}
}


/*irqreturn_t gpio_irq_b0(int irq, void *dev_id, struct pt_regs *regs)
{
	command = 0; 
	printk("\ncommand 0");
	return IRQ_HANDLED;
		
}

irqreturn_t gpio_irq_b1(int irq, void *dev_id, struct pt_regs *regs)
{
	command = 1; 
	printk("\ncommand 1");
	return IRQ_HANDLED;
}

irqreturn_t gpio_irq_b2(int irq, void *dev_id, struct pt_regs *regs)
{
	command = 2; 
	printk("\ncommand 2");
	return IRQ_HANDLED;
		
}

irqreturn_t gpio_irq_b3(int irq, void *dev_id, struct pt_regs *regs)
{
	command = 3; 
	printk("\ncommand 3");
	return IRQ_HANDLED;		
}*/






