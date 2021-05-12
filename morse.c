#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <linux/mm.h>
#include <linux/ktime.h>
#include <linux/input.h>
#include <linux/notifier.h>
#include <linux/keyboard.h>

MODULE_LICENSE("GPL");

static struct input_dev *dev;

static void report_key(char key) {
    switch (key) {
        case 'H':
            printk(KERN_INFO "Reporting H\n");
            input_report_key(dev, KEY_H, 1);
            input_report_key(dev, KEY_H, 0);
            break;
        case 'E':
            input_report_key(dev, KEY_E, 1);
            input_report_key(dev, KEY_E, 0);
            break;
        case 'L':
            input_report_key(dev, KEY_L, 1);
            input_report_key(dev, KEY_L, 0);
            break;
        case 'O':
            input_report_key(dev, KEY_O, 1);
            input_report_key(dev, KEY_O, 0);
            break;
        default:
            break;
    }
}

static int handle(void *arg) {

    int err;
    struct file *input;
    mm_segment_t fs;

    int left, middle, right;
    signed char x, y;
    unsigned char data[3];

    int prev;

    int morse_code[10];
    int index;
    char c;
    char message[100];
    int msg_index;

    int last;
    u64 last_time;
    u64 last_release;

    prev = 0;

    data[0] = 0;
    data[1] = 1;
    data[2] = 2;

    index = 0;
    msg_index = 0;

    last = 0;
    printk(KERN_INFO "opening fifo");
    input = filp_open("/home/leo/test.fifo", O_RDONLY, 0);
    if (IS_ERR(input)) {
        printk(KERN_ALERT "Failed to open input pseudofile!");
        err = PTR_ERR(input);
        return err;
    }


    printk(KERN_INFO "starting read");
    while (1) {
        kernel_read(input, data, 3, &input->f_pos);
        left = data[0] & 0x1;
        right = data[0] & 0x2;
        middle = data[0] & 0x4;


        if (prev == left)
            continue;

        prev = left;
        
        printk(KERN_INFO "Left: %d", left);
        
        x = data[1];
        y = data[2];

        if (left) {
            if (index > 0 && ktime_get_raw_ns() - last_release > 4000000000) {
                printk(KERN_INFO "Reading char (reading %d signals)\n", index);
                printk(KERN_INFO "Current buffer:\n");
                int i = 0;
                while (i < index) {
                    printk(KERN_INFO "[%d]: %d\n", i, morse_code[i]);
                    i += 1;
                }
                if (index == 1) {
                    if (morse_code[0] == 0) {
                        c = 'E';
                    }
                } else if (index == 2) {

                } else if (index == 3) {
                    if (morse_code[0] == 1 && morse_code[1] == 1 && morse_code[2] == 1)
                        c = 'O';
                } else if (index == 4) {
                    if (morse_code[0] == 0 && morse_code[1] == 0 && morse_code[2] == 0 && morse_code[3] == 0)
                        c = 'H';
                    else if (morse_code[0] == 0 && morse_code[1] == 1 && morse_code[2] == 0 && morse_code[3] == 0)
                        c = 'L';
                }

                report_key(c);

                message[msg_index] = c;
                msg_index += 1;

                message[msg_index] = '\0';
                printk(KERN_INFO "Morse code: %s\n", message);

                index = 0;

            }

            last = 1;
            last_time = ktime_get_raw_ns();
        } else {
            last_release = ktime_get_raw_ns();
            if (last) {
                printk(KERN_INFO "Time: %d\n", last_release - last_time);
                if (last_release - last_time > 1000000000) {
                   morse_code[index] = 1;
                   printk(KERN_INFO "long\n");
                   index += 1;
                } else {
                   morse_code[index] = 0;
                   printk(KERN_INFO "short\n");
                   index += 1;
                }
                
                last = 0;
            }
        }

    }

    

}

static int __init LKM_init(void) {
    struct task_struct *task;
    int err;

    printk(KERN_INFO "Hello World!\n");
   
    dev = input_allocate_device();
    if (!dev) {
        printk(KERN_ERR "Not enought memory\n");
        err = -ENOMEM;
        return err;
    }
    
    dev->name = "MorseMouse";
    dev->phys = "morse/input0";
    dev->id.bustype = BUS_VIRTUAL;
    dev->id.vendor = 0x0000;
    dev->id.product = 0x0000;
    dev->id.version = 0x0000;

    dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
    //dev->keybit[BIT_WORD(KEY_H)] = BIT_MASK(KEY_H);
    set_bit(KEY_H, dev->keybit);
    set_bit(KEY_E, dev->keybit);
    set_bit(KEY_L, dev->keybit);
    set_bit(KEY_O, dev->keybit);


    err = input_register_device(dev);
    if (err) {
        printk(KERN_ERR "Failed to register device.");
        input_free_device(dev);
        return err;
    }


    task = kthread_run(handle, NULL, "blub");

    if (IS_ERR(task)) {
        printk(KERN_INFO "Failed to create thread");
        err = PTR_ERR(task);
        return err;
    }

    return 0;
}

static void __exit LKM_exit(void) {
    printk(KERN_INFO "Bye\n");
}


module_init(LKM_init);
module_exit(LKM_exit);

