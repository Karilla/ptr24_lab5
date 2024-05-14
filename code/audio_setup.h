/*****************************************************************************
 * Author: A.Gabriel Catel Torres
 *
 * Version: 1.0
 *
 * Date: 30/04/2024
 *
 * File: audio_setup.h
 *
 * Description: functions and defines used to launch Xenomai tasks to perform
 * the acquisition of the audio data coming from the microphone input and write
 * it to the audio output.
 *
 ******************************************************************************/

#ifndef AUDIO_SETUP_H
#define AUDIO_SETUP_H

#include <alchemy/task.h>
#include <stdbool.h>

#include "audio_utils.h"
#include "control.h"

#define AUDIO_ACK_TASK_PRIORITY 50

#define AUDIO_FILENAME "output.wav"

// Audio defines
#define SAMPLING 48000.
#define FIFO_SIZE 256
#define NB_CHAN 2
#define PERIOD_MARGIN 25

#define S_IN_NS 1000000000UL

typedef struct Priv_audio_args
{
    Ctl_data_t *ctl;
    RT_TASK acquisition_rt_task;
    data_t *samples_buf;
    RT_QUEUE* mailBox;
} Priv_audio_args_t;

typedef struct message_treatement
{
    data_t *samples_buf;
}message_treatement_t;

typedef struct message_logging
{
    double principal_freq;
    RT_TIME processing_time;
}message_logging_t;

/**
 * \brief Read the data from the audio input and writes it to the
 * audio output.
 *
 * \param cookie pointer to private data. Can be anything
 */
void acquisition_task(void *cookie);

#endif // AUDIO_SETUP_H
