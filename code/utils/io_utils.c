// #include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <cobalt/stdio.h>

#include <alchemy/task.h>

#include "io_utils.h"
#include "common.h"

#define HEX0            0
#define HEX1            1
#define HEX2            2
#define HEX3            3
#define HEX4            4
#define HEX5            5
#define NB_HEX          6

#define GPIO_BANK_0     0
#define GPIO_BANK_1     1
#define GPIO_NB_BANKS   2

#define HEX_READ        0
#define HEX_WRITE       1

static int rtioctl_fd;
static void *ioctrls;

RT_TASK init_wrapper_task;

void init_wrapper(void *arg)
{
  rtioctl_fd = open(IOCTL_FILE, O_RDWR);
    if (rtioctl_fd < 0) {
        perror("Failed to open the IOCTL device file\n");
    }

    // Map the device memory to user space
    ioctrls = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, rtioctl_fd, 0);
    if (ioctrls == MAP_FAILED) {
        perror("Failed to map device memory");
        close(rtioctl_fd);
    }

}

int init_ioctl()
{
    rt_task_create(&init_wrapper_task, NULL, 0, 50, T_JOINABLE);
    rt_task_start(&init_wrapper_task, &init_wrapper, 0);
    rt_task_join(&init_wrapper_task);
    return 0;
}


void clear_ioctl()
{
    close(rtioctl_fd);
    if (munmap(ioctrls, 4096) == -1) {
        perror("Unmapping");
        exit(EXIT_FAILURE);
    }
}


int read_key(Access_type access)
{
    int value = 0; 
    if(access == IOCTL) {
        ioctl(rtioctl_fd, IOCTL_READ_KEY, &value);
    } else if(access == MMAP) {
        value = *((unsigned *)(ioctrls + IO_KEYS));
    }
    return value;
}


int read_switch(Access_type access)
{
    int value = 0; 
    if(access == IOCTL) {
        ioctl(rtioctl_fd, IOCTL_READ_SWITCH, &value);
    } else if(access == MMAP) {
        value = *((unsigned *)(ioctrls + IO_SWITCH));
    }
    return value;
}


void write_led(Access_type access, unsigned value)
{
    if(access == IOCTL) {
        ioctl(rtioctl_fd, IOCTL_WRITE_LED, &value);
    } else if(access == MMAP) {
        *((unsigned *)(ioctrls + IO_LEDS)) = value;
    }
    
}


int read_led(Access_type access)
{
    int value = 0; 
    if(access == IOCTL) {
        ioctl(rtioctl_fd, IOCTL_READ_LED, &value);
    } else if(access == MMAP) {
        value = *((unsigned *)(ioctrls + IO_LEDS));
    }
    return value;
}


void write_hex(Access_type access, unsigned hex, unsigned value)
{
    if(access == IOCTL) {
        switch(hex){
            case HEX0: ioctl(rtioctl_fd, IOCTL_WRITE_HEX0, &value); break;
            case HEX1: ioctl(rtioctl_fd, IOCTL_WRITE_HEX1, &value); break;
            case HEX2: ioctl(rtioctl_fd, IOCTL_WRITE_HEX2, &value); break;
            case HEX3: ioctl(rtioctl_fd, IOCTL_WRITE_HEX3, &value); break;
            case HEX4: ioctl(rtioctl_fd, IOCTL_WRITE_HEX4, &value); break;
            case HEX5: ioctl(rtioctl_fd, IOCTL_WRITE_HEX5, &value); break;
            default: break;
        }
    } else if(access == MMAP) {
        *((unsigned *)(ioctrls + IO_HEX0 + hex*REG_SIZE)) = value;
    }
}


int read_hex(Access_type access, unsigned hex)
{
    int value = 0;

    if(access == IOCTL) {
        switch(hex) {
            case HEX0: ioctl(rtioctl_fd, IOCTL_READ_HEX0, &value); break;
            case HEX1: ioctl(rtioctl_fd, IOCTL_READ_HEX1, &value); break;
            case HEX2: ioctl(rtioctl_fd, IOCTL_READ_HEX2, &value); break;
            case HEX3: ioctl(rtioctl_fd, IOCTL_READ_HEX3, &value); break;
            case HEX4: ioctl(rtioctl_fd, IOCTL_READ_HEX4, &value); break;
            case HEX5: ioctl(rtioctl_fd, IOCTL_READ_HEX5, &value); break;
            default: break;
        }
    } else if(access == MMAP) {
        value = *((unsigned *)(ioctrls + IO_HEX0 + hex*REG_SIZE));
    }

    return value;
}


void write_gpio_en(Access_type access, unsigned bank, Reg_sel_t sel, unsigned value)
{
    if(access == IOCTL) {
        switch(bank) {
        case GPIO_BANK_0: 
            if(sel == REG_HIGH) {
                ioctl(rtioctl_fd, IOCTL_WRITE_GPIO_EN_B0_1, &value); 
            } else {
                ioctl(rtioctl_fd, IOCTL_WRITE_GPIO_EN_B0_0, &value); 
            }
            break;

        case GPIO_BANK_1: 
            if(sel == REG_HIGH) {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_EN_B1_1, &value); 
            } else {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_EN_B1_0, &value); 
            }
            break;

            default: break;
        }
    } else if(access == MMAP) {
        *((unsigned *)(ioctrls + IO_GPIO_EN_B0_0 + bank*GPIO_BANK_SIZE +
                                    sel*REG_SIZE)) = value;
    }

}


int read_gpio_en(Access_type access, unsigned bank, Reg_sel_t sel)
{
    int value = 0; 

    if(access == IOCTL) {

        switch(bank) {
        case GPIO_BANK_0: 
            if(sel == REG_HIGH) {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_EN_B0_1, &value); 
            } else {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_EN_B0_0, &value); 
            }
            break;

        case GPIO_BANK_1: 
            if(sel == REG_HIGH) {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_EN_B1_1, &value); 
            } else {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_EN_B1_0, &value); 
            }
            break;

            default: break;
        } 
        
    } else if(access == MMAP) {

        value = *((unsigned *)(ioctrls + IO_GPIO_EN_B0_0 + bank*GPIO_BANK_SIZE 
                               + sel*REG_SIZE));
    }

    return value;
}


void write_gpio_val(Access_type access, unsigned bank, Reg_sel_t sel, unsigned value)
{
    if(access == IOCTL) {
        switch(bank) {
        case GPIO_BANK_0: 
            if(sel == REG_HIGH) {
                ioctl(rtioctl_fd, IOCTL_WRITE_GPIO_VAL_B0_1, &value); 
            } else {
                ioctl(rtioctl_fd, IOCTL_WRITE_GPIO_VAL_B0_0, &value); 
            }
            break;

        case GPIO_BANK_1: 
            if(sel == REG_HIGH) {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_VAL_B1_1, &value); 
            } else {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_VAL_B1_0, &value); 
            }
            break;

            default: break;
        } 

    } else if(access == MMAP) {

        *((unsigned *)(ioctrls + IO_GPIO_VAL_B0_0 + bank*GPIO_BANK_SIZE +
                                sel*REG_SIZE)) = value;
    }
}


int read_gpio_val(Access_type access, unsigned bank, Reg_sel_t sel)
{
    int value = 0; 

    if(access == IOCTL) {
        switch(bank) {
        case GPIO_BANK_0: 
            if(sel == REG_HIGH) {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_VAL_B0_1, &value); 
            } else {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_VAL_B0_0, &value); 
            }
            break;

        case GPIO_BANK_1: 
            if(sel == REG_HIGH) {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_VAL_B1_1, &value); 
            } else {
                ioctl(rtioctl_fd, IOCTL_READ_GPIO_VAL_B1_0, &value); 
            }
            break;

            default: break;
        }

    } else if(access == MMAP) {

        value = *((unsigned *)(ioctrls + IO_GPIO_VAL_B0_0 + bank*GPIO_BANK_SIZE 
                               + sel*REG_SIZE));
    }
    return value;
}
