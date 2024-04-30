#include "audio_setup.h"

// send 8192 byte to treatment task

RT_QUEUE mail_box;

void monitoring_task(void *cookie)
{
}

void treatment_task(void *cookie)
{
}

void acquisition_task(void *cookie)
{
    Priv_audio_args_t *priv = (Priv_audio_args_t *)cookie;

    int err;
    // Calculate the minimum frequency of the task
    uint32_t audio_task_freq = (SAMPLING * sizeof(data_t)) / (FIFO_SIZE);
    // Calculate the period of the task with a margin to ensure not any data is lost
    uint64_t period_in_ns = S_IN_NS / (audio_task_freq + PERIOD_MARGIN);

    if ((err = rt_task_set_periodic(NULL, TM_NOW, period_in_ns)) < 0)
    {
        rt_printf("Could not set period of acquisition task\n");
        return;
    }

    while (priv->ctl->running)
    {
        if (!ctl->audio_running)
        {
            // wait
        }
        // Get the data from the buffer
        ssize_t read = read_samples(priv->samples_buf, FIFO_SIZE * NB_CHAN);
        if (read)
        {
            // Write the data to the audio output
            write_samples(priv->samples_buf, read);
        }

        rt_task_wait_period(NULL);
    }
    rt_printf("Terminating acquisition task\n");
}
