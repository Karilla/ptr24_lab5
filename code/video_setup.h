/*****************************************************************************
 * Author: A.Gabriel Catel Torres
 *
 * Version: 1.0
 *
 * Date: 30/04/2024
 *
 * File: video_setup.h
 *
 * Description: functions and defines used to launch Xenomai tasks to perform
 * the acquisition of the video data coming from a raw file and display it
 * on the VGA output.
 * The data is RGB0, so each pixel is coded on 32 bits.
 *
 ******************************************************************************/
#ifndef VIDEO_SETUP_H
#define VIDEO_SETUP_H

#include <alchemy/task.h>
#include <stdbool.h>
#include <stdint.h>

#include "control.h"

#define VIDEO_ACK_TASK_PRIORITY 50

// Video defines
#define WIDTH 320
#define HEIGHT 240
#define FRAMERATE 15
#define NB_FRAMES 300
#define BYTES_PER_PIXEL 4

#define VIDEO_FILENAME "output_video.raw"

#define S_IN_NS 1000000000UL

typedef struct Priv_video_args
{
    Ctl_data_t *ctl;
    RT_TASK rt_task;
    uint8_t *buffer;
} Priv_video_args_t;

/**
 * \brief Read the video data from a file and writes it to the
 * DE1-SoC VGA output to display the RGB0 data. The size of the
 * picture displayed is locked to 320x240 and is only RGB0.
 *
 * \param cookie pointer to private data. Can be anything
 */
void video_task(void *cookie);

#endif // VIDEO_SETUP_H
