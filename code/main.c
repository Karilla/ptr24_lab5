/*****************************************************************************
 * Author: A.Gabriel Catel Torres
 *
 * Version: 1.0
 *
 * Date: 30/04/2024
 *
 * File: main.c
 *
 * Description: setup of audio, video and control tasks that will run in
 * parallel. This application is used to overload the CPU and analyse the
 * tasks execution time.
 *
 ******************************************************************************/

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <cobalt/stdio.h>

#include <alchemy/task.h>

#include "audio_utils.h"
#include "video_utils.h"
#include "io_utils.h"
#include "audio_setup.h"
#include "video_setup.h"
#include "control.h"

// IOCTL defines
#define KEY0 0x0

#define SWITCH_0 0x01
#define SWITCH_1 (0x01 << 1)
#define SWITCH_8 (0x01 << 8)
#define SWITCH_9 (0x01 << 9)

// Control task defines
#define CTL_TASK_PERIOD 100000000 // 10HZ
#define CTL_TASK_PRIORITY 50

void ioctl_ctl_task(void *cookie)
{
    Ctl_data_t *priv = (Ctl_data_t *)cookie;

    rt_task_set_periodic(NULL, TM_NOW, CTL_TASK_PERIOD);

    while (priv->running)
    {
        unsigned keys = read_key(MMAP);
        unsigned switch_value = read_switch(MMAP);

        if (switch_value & SWITCH_0)
        {
            // activation audio
            priv->audio_running = true;
            rt_event_signal(priv->control_event, AUDIO_RUNNING);
        }
        else
        {
            // descativation audio
            priv->audio_running = false;
            rt_event_clear(priv->control_event, AUDIO_RUNNING, NULL);
        }

        if (switch_value & SWITCH_1)
        {
            // activation video
            priv->video_running = true;
            rt_event_signal(priv->control_event, VIDEO_RUNNING);
        }
        else
        {
            // descativation video
            priv->video_running = false;
            rt_event_clear(priv->control_event, VIDEO_RUNNING, NULL);
        }

        if (switch_value & SWITCH_8)
        {
            // activation grey
            rt_event_signal(priv->control_event, GREYSCALE_ACTIVATION);
        }
        else
        {
            // descativation grey
            rt_event_clear(priv->control_event, GREYSCALE_ACTIVATION, NULL);
        }

        if (switch_value & SWITCH_9)
        {
            // activation convolution
            rt_event_signal(priv->control_event, CONVOLUTION_ACTIVATION);
        }
        else
        {
            // descativation convolution
            rt_event_clear(priv->control_event, CONVOLUTION_ACTIVATION, NULL);
        }
        // Check if the key0 is pressed
        if (keys | KEY0)
        {
            priv->running = false;
        }
        rt_task_wait_period(NULL);
    }
}

int main(int argc, char *argv[])
{
    int ret;

    printf("----------------------------------\n");
    printf("PTR24 - lab05\n");
    printf("----------------------------------\n");

    mlockall(MCL_CURRENT | MCL_FUTURE);

    // Ioctl setup
    if (init_ioctl())
    {
        perror("Could not init IOCTL...\n");
        exit(EXIT_FAILURE);
    }

    RT_HEAP audio_heap;
    if (rt_heap_create(&audio_heap, "Audio Heap", 100000, H_PRIO) != 0)
    {
        perror("Error while creating heap...\n");
        exit(EXIT_FAILURE);
    }

    RT_EVENT event;

    if (rt_event_create(&event, "Control Event", 0, EV_PRIO))
    {
        perror("Error while creating control event");
        exit(EXIT_FAILURE);
    }

    // Init structure used to control the program flow
    Ctl_data_t ctl;
    ctl.running = true;
    ctl.control_event = &event;

    // Create the IOCTL control task
    RT_TASK ioctl_ctl_rt_task;
    if (rt_task_spawn(&ioctl_ctl_rt_task, "program control task", 0,
                      CTL_TASK_PRIORITY, T_JOINABLE,
                      ioctl_ctl_task, &ctl) != 0)
    {
        perror("Error while starting acquisition_task");
        exit(EXIT_FAILURE);
    }
    printf("Launched audio acquisition task\n");

    // Audio setup
    if (init_audio())
    {
        perror("Could not init the audio...\n");
        exit(EXIT_FAILURE);
    }

    // Init private data used for the audio tasks
    Priv_audio_args_t priv_audio;
    priv_audio.samples_buf = (data_t *)malloc(FIFO_SIZE * NB_CHAN);
    priv_audio.ctl = &ctl;
    priv.heap = &audio_heap;

    // Create the audio acquisition task
    if (rt_task_spawn(&priv_audio.acquisition_rt_task, "audio task", 0,
                      AUDIO_ACK_TASK_PRIORITY, T_JOINABLE,
                      acquisition_task, &priv_audio) != 0)
    {
        perror("Error while starting treatment task\n");
        exit(EXIT_FAILURE);
    }
    printf("Launched audio acquisition task\n");

    // Video setup
    ret = init_video();
    if (ret < 0)
    {
        perror("Could not init the video...\n");
        return ret;
    }

    // Init private data used for the video tasks
    Priv_video_args_t priv_video;
    priv_video.img.width = WIDTH;
    priv_video.img.height = HEIGHT;
    priv_video.img.components = 4;
    priv_video.img.data = (uint8_t *)malloc(HEIGHT * WIDTH * BYTES_PER_PIXEL);
    priv_video.ctl = &ctl;

    // Create the video acquisition task
    if (rt_task_spawn(&priv_video.rt_task, "video task", 0, VIDEO_ACK_TASK_PRIORITY,
                      T_JOINABLE, video_task, &priv_video) != 0)
    {
        perror("Error while starting video_function");
        exit(EXIT_FAILURE);
    }
    printf("Launched video acquisition task\n");

    printf("----------------------------------\n");
    printf("Press KEY0 to exit the program\n");
    printf("----------------------------------\n");

    // Waiting for the end of the program (coming from ctrl + c)
    rt_task_join(&priv_audio.acquisition_rt_task);
    rt_task_join(&priv_video.rt_task);
    rt_task_join(&ioctl_ctl_rt_task);

    // Free all resources of the video/audio/ioctl
    clear_ioctl();
    clear_audio();
    clear_video();

    rt_heap_delete(&audio_heap);

    // Free everything else
    free(priv_audio.samples_buf);
    free(priv_video.buffer);

    munlockall();

    rt_printf("Application has correctly been terminated.\n");

    return EXIT_SUCCESS;
}
