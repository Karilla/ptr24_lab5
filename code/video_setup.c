#include <stdio.h>

#include <alchemy/task.h>

#include "video_setup.h"
#include "video_utils.h"

void convolution_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;

    struct img_1D_t *conv_img = malloc(sizeof(struct img_1D_t));

    convolution_grayscale(priv->img->data, conv_img->data, WIDTH, HEIGHT);

    // Copy the convoluted image to the buffer
    memcpy(priv->buffer, conv_img->data, WIDTH * HEIGHT);

    free(conv_img);
}

void greyscale_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;

    struct img_1D_t *greyscale_img = malloc(sizeof(struct img_1D_t));

    rgba_to_grayscale8(priv->img, greyscale_img->data);

    // Copy the greyscale image to the buffer
    memcpy(priv->buffer, greyscale_img->data, WIDTH * HEIGHT);

    free(greyscale_img);
}

void video_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;

    uint64_t period_in_ns = S_IN_NS / FRAMERATE;

    rt_task_set_periodic(NULL, TM_NOW, period_in_ns);

    // Loop that reads a file with raw data
    while (priv->ctl->running)
    {
        // Check if task is running before waiting to avoid going into waiting state
        if (!ctl->audio_running)
        {
            rt_event_wait(priv->control_event, VIDEO_RUNNING, NULL, TM_INFINITE);
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

            if (rt_event_wait(priv->control_event, GREYSCALE_ACTIVATION, NULL, TM_NONBLOCK) == 1)
            {
                // If greyscale is activated, activate the greyscale task
                rt_task_start(&priv->greyscale_task, greyscale_task, priv);

                if (rt_event_wait(priv->control_event, CONVOLUTION_ACTIVATION, NULL, TM_NONBLOCK) == 1)
                {
                    // If convolution is activated, activate the convolution task
                    rt_task_start(&priv->convolution_task, convolution_task, priv);

                    rt_task_join(&priv->convolution_task);
                }

                rt_task_join(&priv->greyscale_task);
            }

            // Copy the data from the buffer to the video buffer
            memcpy(get_video_buffer(), priv->buffer, WIDTH * HEIGHT * BYTES_PER_PIXEL);

            rt_task_wait_period(NULL);
        }
    }
    rt_printf("Terminating video task.\n");
}
