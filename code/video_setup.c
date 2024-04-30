#include <stdio.h>

#include <alchemy/task.h>

#include "video_setup.h"
#include "video_utils.h"

void video_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;

    uint64_t period_in_ns = S_IN_NS / FRAMERATE;

    rt_task_set_periodic(NULL, TM_NOW, period_in_ns);

    // Loop that reads a file with raw data
    while (priv->ctl->running)
    {
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
            // Copy the data from the buffer to the video buffer
            memcpy(get_video_buffer(), priv->buffer, WIDTH * HEIGHT * BYTES_PER_PIXEL);

            rt_task_wait_period(NULL);
        }
    }
    rt_printf("Terminating video task.\n");
}
