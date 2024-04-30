#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stdint.h>
#include <stdbool.h>

#define IOCTL_FILE      "/dev/rtdm/ioctl"

#define REG_SIZE                4

#define HEX_DIGIT_0             ~0x40
#define HEX_DIGIT_1             ~0xf9
#define HEX_DIGIT_2             ~0x24
#define HEX_DIGIT_3             ~0x30
#define HEX_DIGIT_4             ~0x19
#define HEX_DIGIT_5             ~0x12
#define HEX_DIGIT_6             ~0x02
#define HEX_DIGIT_7             ~0x78
#define HEX_DIGIT_8             ~0x00
#define HEX_DIGIT_9             ~0x10

#define IO_SWITCH               0x0
#define IO_KEYS                 0x4
#define IO_LEDS                 0x8
#define IO_HEX0                 0xC
#define IO_HEX1                 0x10
#define IO_HEX2                 0x14
#define IO_HEX3                 0x18
#define IO_HEX4                 0x1C
#define IO_HEX5                 0x20

#define GPIO_BANK_SIZE          0x10
#define IO_GPIO_EN_B0_0         0x24
#define IO_GPIO_EN_B0_1         0x28
#define IO_GPIO_VAL_B0_0        0x2C
#define IO_GPIO_VAL_B0_1        0x30
#define IO_GPIO_EN_B1_0         0x34
#define IO_GPIO_EN_B1_1         0x38
#define IO_GPIO_VAL_B1_0        0x3C
#define IO_GPIO_VAL_B1_1        0x40
#define IO_END                  0x44

#define NB_HEX                  6

typedef enum 
{ 
    REG_LOW, 
    REG_HIGH
} Reg_sel_t;

typedef enum 
{ 
    IOCTL, 
    MMAP
} Access_type;

/**
 * @brief map the buffer address and open the char device
 * @return 0 on success
 */
int init_ioctl();

/**
 * @brief free everything that was setup during the init
 */
void clear_ioctl();

/**
 * \brief Read key value
 * 
 * bits[31..4] -> unused
 * bits[3..0]  -> KEY[3..0]
 * 
 * \param access value can be MMAP|IOCTL
 * 
 * \returns key value or negative value if error
 */
int read_key(Access_type access);

/**
 * \brief Read switch value
 * 
 * bits[31..10] -> unused
 * bits[9..0]   -> SW[9..0]
 * 
 * \param access value can be MMAP|IOCTL
 * 
 * \returns switches value or negative value if error
 */
int read_switch(Access_type access);

/**
 * \brief Write leds value
 * 
 * bits[31..10] -> unused
 * bits[9..0]   -> LEDR[9..0]
 * 
 * \param access value can be MMAP|IOCTL
 * \param value value of the leds
 */
void write_led(Access_type access, unsigned value);

/**
 * \brief Read leds value
 * 
 * bits[31..10] -> unused
 * bits[9..0]   -> LEDR[9..0]
 * 
 * \param access value can be MMAP|IOCTL
 * 
 * \returns switches value or negative value if error
 */
int read_led(Access_type access);

/**
 * \brief Write hex value
 * 
 * 6 Hex available, each with 7 segments
 * bits[31..7] -> unused
 * bits[6..0]  -> SEG[6..0]
 * 
 * \param access value can be MMAP|IOCTL
 * \param hex hex to write to [5..0]
 * \param value value of the 7 segments
 */
void write_hex(Access_type access, unsigned hex, unsigned value);

/**
 * \brief Read hex value
 * 
 * bits[31..7] -> unused
 * bits[6..0]  -> SEG[6..0]
 * 
 * \param access value can be MMAP|IOCTL
 * \param hex hex to write to [5..0]
 * 
 * \returns switches value or negative value if error
 */
int read_hex(Access_type access, unsigned hex);

/**
 * \brief Write GPIO enable value
 * 
 * \param access value can be MMAP|IOCTL
 * \param bank index of GPIO bank
 * \param value value of enable, each bit is 1 GPIO
 */
void write_gpio_en(Access_type access, unsigned bank, Reg_sel_t sel, unsigned value);

/**
 * \brief Read GPIO enable value
 * 
 * \param access value can be MMAP|IOCTL
 * \param bank index of GPIO bank
 */
int read_gpio_en(Access_type access, unsigned bank, Reg_sel_t sel);

/**
 * \brief Write GPIO pin values
 * 
 * \param access value can be MMAP|IOCTL
 * \param bank index of GPIO bank
 * \param value value of enable, each bit is 1 GPIO
 */
void write_gpio_val(Access_type access, unsigned bank, Reg_sel_t sel, unsigned value);

/**
 * \brief Read GPIO pin values
 * 
 * \param access value can be MMAP|IOCTL
 * \param bank index of GPIO bank
 * \param sel select high or low part of the reg
 */
int read_gpio_val(Access_type access, unsigned bank, Reg_sel_t sel);

#endif // IO_UTILS_H
