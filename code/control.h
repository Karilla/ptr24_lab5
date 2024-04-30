/*****************************************************************************
 * Author: A.Gabriel Catel Torres
 *
 * Version: 1.0
 *
 * Date: 30/04/2024
 *
 * File: control.h
 *
 * Description: structure declaration used to synchronize the Xenomai tasks
 *
 ******************************************************************************/

#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>

typedef struct Ctl_data
{
    bool running;
} Ctl_data_t;

#endif