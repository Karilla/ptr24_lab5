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
#include <alchemy/event.h>

#define AUDIO_RUNNING 1
#define VIDEO_RUNNING 2
#define GREYSCALE_ACTIVATION 4
#define CONVOLUTION_ACTIVATION 8

typedef struct Ctl_data
{
    RT_EVENT *control_event;
    bool audio_running;
    bool video_running;
    bool running;
} Ctl_data_t;

#endif