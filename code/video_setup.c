#include <stdio.h>
#include <fcntl.h>

#include <alchemy/task.h>
#include <alchemy/timer.h>

#include "video_setup.h"
#include "utils/video_utils.h"
#include "utils/convolution.h"
#include "utils/grayscale.h"
#include "utils/image.h"

#define MEASURE 1

void convolution_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;

    uint8_t *conv_data = malloc(VIDEO_RESOLUTION);
    uint8_t *grey_8 = malloc(VIDEO_RESOLUTION);
    struct img_1D_t *result;
    result->width = WIDTH;
    result->height = HEIGHT;
    result->components = 4;
    result->data = malloc(VIDEO_RESOLUTION);

    if (!conv_data || !grey_8)
    {
        rt_printf("Error while allocating memory for convolution image\n");
        exit(EXIT_FAILURE);
    }

    while (priv->ctl->running)
    {
        rt_event_wait(priv->video_task_event, CONVOLUTION_RUNNING, NULL, EV_ANY, TM_INFINITE);

        rgba_to_grayscale8(&priv->img, grey_8);
        convolution_grayscale(grey_8, conv_data, WIDTH, HEIGHT);
        grayscale_to_rgba(conv_data, result);

        memcpy(get_video_buffer(), result->data, VIDEO_RESOLUTION);
        
        rt_event_clear(priv->video_task_event, CONVOLUTION_RUNNING, NULL);
        //rt_event_signal(priv->video_task_event, CONVOLUTION_STOP);
    }

    free(conv_data);
    free(grey_8);
}

void greyscale_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;

    struct img_1D_t *greyscale_img;
    greyscale_img->data = malloc(VIDEO_RESOLUTION);

    if (!greyscale_img)
    {
        rt_printf("Error while allocating memory for greyscale image\n");
        exit(EXIT_FAILURE);
    }

    while (priv->ctl->running)
    {
        rt_event_wait(priv->video_task_event, GREYSCALE_RUNNING, NULL, EV_ANY, TM_INFINITE);

        rgba_to_grayscale32(&priv->img, greyscale_img);

        // Copy the greyscale image to the buffer
        memcpy(get_video_buffer(), greyscale_img->data, VIDEO_RESOLUTION);

        rt_event_clear(priv->video_task_event, GREYSCALE_RUNNING, NULL);
        rt_event_signal(priv->video_task_event, GREYSCALE_STOP);
    }
    
    free(greyscale_img->data);
}

void video_task(void *cookie)
{
    Priv_video_args_t *priv = (Priv_video_args_t *)cookie;

    int err;
    uint64_t period_in_ns = S_IN_NS / FRAMERATE;

    // Create the video task event to synchronize the greyscale and convolution tasks
    if (rt_event_create(&priv->video_task_event, "Video Task Event", 0, EV_FIFO))
    {
        perror("Error while creating video task event");
        exit(EXIT_FAILURE);
    }

    
    // Create the greyscale tasks
    Priv_video_args_t greyscale_task_args;
    greyscale_task_args.ctl = priv->ctl;
    greyscale_task_args.img = priv->img;
    greyscale_task_args.video_task_event = &priv->video_task_event;

   
    if (rt_task_spawn(&greyscale_task_args.rt_task, "Greyscale Task", 0, VIDEO_TASK_PRIORITY, T_JOINABLE, greyscale_task, &greyscale_task_args))
    {
        rt_printf("Error while launching greyscale task\n");
        exit(EXIT_FAILURE);
    }
    
    // Create the convolution tasks
    Priv_video_args_t convolution_task_args;
    convolution_task_args.ctl = priv->ctl;
    convolution_task_args.img = priv->img;
    convolution_task_args.video_task_event = &priv->video_task_event;
    
    if (rt_task_spawn(&convolution_task_args.rt_task, "Convolution Task", 0, VIDEO_TASK_PRIORITY, T_JOINABLE, convolution_task, &convolution_task_args))
    {
        rt_printf("Error while launching convolution task\n");
        exit(EXIT_FAILURE);
    }
    
    // Set the period of the video task
    if ((err = rt_task_set_periodic(NULL, TM_NOW, rt_timer_ns2ticks(period_in_ns))) < 0)
    {
        rt_printf("Error while setting period of video task\n");
        return;
    }

    int fd = open("/usr/resources/output_video.raw", O_RDONLY);
    if (fd < 0)
    {
        rt_printf("Error: Couldn't open raw video file.\n");
        exit(EXIT_FAILURE);
    }

    RTIME now, previous;

    // Loop that reads a file with raw data
    while (priv->ctl->running)
    {
#if MEASURE
        previous = rt_timer_read();
#endif

        // Check if task is running before waiting to avoid going into waiting state
        if (!priv->ctl->video_running)
        {
            rt_printf("Video Wait\n");
            rt_event_wait(priv->ctl->control_event, VIDEO_RUNNING, NULL, EV_ANY, TM_INFINITE);
            rt_printf("Video Start\n");
        }

        // Read the frame from the file
        //err = read(fd, get_video_buffer(), VIDEO_RESOLUTION);
        err = read(fd, priv->img.data, VIDEO_RESOLUTION);

        if (err < 0)
        {
            rt_printf("Error: Couldn't read raw video file.\n");
            exit(EXIT_FAILURE);
        }

        if (err == 0)
        {
            // Replay the video
            lseek(fd, 0, SEEK_SET);
            continue;
        }

        if (rt_event_wait(priv->ctl->control_event, CONVOLUTION_ACTIVATION, NULL, EV_ANY, TM_NONBLOCK) != -EWOULDBLOCK)
        {
            // If convolution is activated, activate the convolution task
            rt_event_signal(convolution_task_args.video_task_event, CONVOLUTION_RUNNING);
        }
        else if (rt_event_wait(priv->ctl->control_event, GREYSCALE_ACTIVATION, NULL, EV_ANY, TM_NONBLOCK) != -EWOULDBLOCK)
        {
            // If greyscale is activated, activate the greyscale task
            rt_event_signal(greyscale_task_args.video_task_event, GREYSCALE_RUNNING);
        } 
        else
        {
            // Show the video without any treatment
            memcpy(get_video_buffer(), priv->img.data, VIDEO_RESOLUTION);
        }


#if MEASURE
        now = rt_timer_read();
        rt_printf("%ld.%04ld\n",
		(long)(now - previous) / 1000000,
		(long)(now - previous) % 1000000);
        previous = now;
#endif

        rt_task_wait_period(NULL);
    }

    close(fd);

    rt_printf("Terminating video task.\n");
}
