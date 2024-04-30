#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <alchemy/task.h>
#include <sys/mman.h>

#include "video_utils.h"
#include "common.h"

static int video_fd;
static void *video_buf;

RT_TASK init_video_wrapper_task;

void init_video_wrapper(void *arg)
{
    video_fd = open(VIDEO_FILE, O_RDWR);
    if (video_fd < 0)
    {
        perror("Failed to open the video device file\n");
    }

    // Map the device memory to user space
    video_buf = mmap(NULL, VIDEO_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, 0);

    if (video_buf == MAP_FAILED)
    {
        perror("Failed to map video memory");
        close(video_fd);
    }
}

int init_video()
{
    int ret;

    ret = rt_task_create(&init_video_wrapper_task, NULL, 0, 50, T_JOINABLE);
    if (ret < 0)
    {
        rt_printf("Error creating task...\n");
        return ret;
    }

    ret = rt_task_start(&init_video_wrapper_task, &init_video_wrapper, 0);
    if (ret < 0)
    {
        rt_printf("Error creating task...\n");
        return ret;
    }

    rt_task_join(&init_video_wrapper_task);
    return 0;
}

void clear_video()
{
    close(video_fd);
}

void *get_video_buffer()
{
    return video_buf;
}

int write_frame(uint8_t *frame_data, unsigned size)
{
    ssize_t bytes_written = write(video_fd, frame_data, size);
    if (bytes_written < 0)
    {
        perror("Failed to write frame data");
        return -1;
    }
    else if ((size_t)bytes_written != size)
    {
        fprintf(stderr, "Incomplete write: expected %u bytes, wrote %zd bytes\n", size, bytes_written);
        return -1;
    }
    return 0;
}
