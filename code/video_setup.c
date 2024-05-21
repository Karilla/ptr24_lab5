#include <stdio.h>

#include <alchemy/task.h>

#include "video_setup.h"
#include "utils/video_utils.h"
#include "utils/convolution.h"
#include "utils/grayscale.h"
#include "utils/image.h"

void convolution_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;

    //TODO : struct img_1D_t *conv_img = rt_heap_alloc(rt_heap_self(), sizeof(struct img_1D_t), TM_INFINITE);
    struct img_1D_t *conv_img = malloc(sizeof(struct img_1D_t));

    if (!conv_img)
    {
        rt_printf("Error while allocating memory for convolution image\n");
        exit(EXIT_FAILURE);
    }

    while (priv->ctl->running)
    {
        rt_event_wait(priv->video_task_event, CONVOLUTION_RUNNING, NULL, EV_ANY, TM_INFINITE);
        
        convolution_grayscale(priv->buffer, conv_img->data, WIDTH, HEIGHT);

        // Copy the convoluted image to the buffer
        memcpy(priv->buffer, conv_img->data, WIDTH * HEIGHT);
        
        rt_event_clear(priv->video_task_event, CONVOLUTION_RUNNING, NULL);
        rt_event_signal(priv->video_task_event, CONVOLUTION_STOP);
    }

    free(conv_img);
}

void greyscale_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;

    struct img_1D_t *greyscale_img = malloc(sizeof(struct img_1D_t));

    if (!greyscale_img)
    {
        rt_printf("Error while allocating memory for greyscale image\n");
        exit(EXIT_FAILURE);
    }

    while (priv->ctl->running)
    {
        rt_event_wait(priv->video_task_event, GREYSCALE_RUNNING, NULL, EV_ANY, TM_INFINITE);
        
        rgba_to_grayscale8(priv->buffer, greyscale_img->data);

        // Copy the greyscale image to the buffer
        memcpy(priv->buffer, greyscale_img->data, WIDTH * HEIGHT);

        rt_event_clear(priv->video_task_event, GREYSCALE_ACTIVATION, NULL);
        rt_event_signal(priv->video_task_event, GREYSCALE_STOP);
    }
    
    free(greyscale_img);
}

void video_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;

    int err;
    uint64_t period_in_ns = S_IN_NS / FRAMERATE;

    // Create the video task event to synchronize the greyscale and convolution tasks
    if (rt_event_create(&priv->video_task_event, "Video Task Event", 0, EV_PRIO))
    {
        perror("Error while creating video task event");
        exit(EXIT_FAILURE);
    }

    // Create the greyscale tasks
    Priv_video_args_t greyscale_task_args;
    greyscale_task_args.ctl = priv->ctl;
    greyscale_task_args.buffer = priv->buffer;

    if (rt_task_spawn(&greyscale_task_args.rt_task, "Greyscale Task", 0, VIDEO_ACK_TASK_PRIORITY, T_JOINABLE, greyscale_task, &greyscale_task_args))
    {
        rt_printf("Error while launching greyscale task\n");
        exit(EXIT_FAILURE);
    }

    // Create the convolution tasks
    Priv_video_args_t convolution_task_args;
    convolution_task_args.ctl = priv->ctl;
    convolution_task_args.buffer = priv->buffer;

    if (rt_task_spawn(&convolution_task_args.rt_task, "Convolution Task", 0, VIDEO_ACK_TASK_PRIORITY, T_JOINABLE, convolution_task, &convolution_task_args))
    {
        rt_printf("Error while launching convolution task\n");
        exit(EXIT_FAILURE);
    }

    // Set the period of the video task
    if ((err = rt_task_set_periodic(NULL, TM_NOW, period_in_ns)) < 0)
    {
        rt_printf("Error while setting period of video task\n");
        return;
    }

    // Loop that reads a file with raw data
    while (priv->ctl->running)
    {
        // Check if task is running before waiting to avoid going into waiting state
        if (!priv->ctl->audio_running)
        {
            rt_event_wait(priv->ctl->control_event, VIDEO_RUNNING, NULL, EV_ANY, TM_INFINITE);
        }

        FILE *file = fopen(VIDEO_FILENAME, "rb");
        if (!file)
        {
            printf("Error: Couldn't open raw video file.\n");
            return;
        }

        // Read each frame from the file
        for (int i = 0; i < NB_FRAMES; i++)
        {
            if (!priv->ctl->running)
            {
                break;
            }

            // Copy the data from the file to a buffer
            fread(priv->buffer, WIDTH * HEIGHT * BYTES_PER_PIXEL, 1, file);

            // TODO: Check if greyscale and convolution are activated ????

            if (rt_event_wait(priv->ctl->control_event, GREYSCALE_ACTIVATION, NULL, EV_ANY, TM_NONBLOCK) == 1)
            {
                // If greyscale is activated, activate the greyscale task
                rt_event_signal(priv->video_task_event, GREYSCALE_ACTIVATION);
                rt_event_wait(priv->video_task_event, GREYSCALE_STOP, NULL, EV_ANY, TM_INFINITE);

                if (rt_event_wait(priv->ctl->control_event, CONVOLUTION_ACTIVATION, NULL, EV_ANY, TM_NONBLOCK) == 1)
                {
                    // If convolution is activated, activate the convolution task
                    rt_event_signal(priv->video_task_event, CONVOLUTION_ACTIVATION);
                    rt_event_wait(priv->video_task_event, CONVOLUTION_STOP, NULL, EV_ANY, TM_INFINITE);
                }
            }

            // Copy the data from the buffer to the video buffer
            memcpy(get_video_buffer(), priv->buffer, WIDTH * HEIGHT * BYTES_PER_PIXEL);

            rt_task_wait_period(NULL);
        }
    }

    fclose(file);

    rt_printf("Terminating video task.\n");
}
