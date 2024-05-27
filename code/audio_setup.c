#include <alchemy/task.h>
#include <alchemy/heap.h>

#include <complex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils/fft_utils.h"
#include "audio_setup.h"
#include "control.h"

// send 8192 byte to treatment task
#define FFT_BUFFER_SIZE 8192
#define FFT_BUFFER_SIZE_LEFT_CANAL 8192 / 2

typedef double complex cplx;

void monitoring_task(void *cookie)
{
    Priv_audio_args_t *priv = (Priv_audio_args_t *)cookie;
    message_logging_t message;
    RTIME now, previous;
    now = rt_timer_read();
    while (priv->ctl->running)
    {
        rt_queue_read(&priv->mailbox_logging, &message, sizeof(message_logging_t), TM_INFINITE);
        previous = rt_timer_read();
        rt_printf("Audio task execution time : %f \n", (double)message.processing_time);
        rt_printf("FFT result : %f Hz\n\n", message.principal_freq);
        rt_queue_free(&priv->mailbox_logging, &message);
        now = rt_timer_read();
    }
    rt_printf("Terminate monitoring task\n");
}

void treatment_task_t_lol(void *cookie)
{
    /* TODO : compléter la tâche processing de telle sorte qu'elle recoive les
     * échantillons de la tâche acquisition. Une fois reçu les échantillons,
     * appliquer une FFT à l'aide de la fonction fft fournie, puis trouver
     * la fréquence à laquelle la puissance est maximale.
     * Enfin, envoyer à la tâche *log* le résultat, ainsi
     * que le temps qui a été nécessaire pour effectuer le calcul.
     *
     * Notez bien que le codec audio échantillone à 48 Khz.
     * Notez aussi que le driver audio renvoie des échantillons stéréo interlacés.
     * Vous n'effectuerez une FFT que sur un seul canal. En conséquence, prenez un
     * échantillon sur deux.
     */

    Priv_audio_args_t *priv = (Priv_audio_args_t *)cookie;
    cplx *out = (cplx *)malloc(sizeof(cplx) * FFT_BUFFER_SIZE_LEFT_CANAL);
    data_t *x = (data_t *)malloc(sizeof(data_t) * FFT_BUFFER_SIZE); // NOTE : discrete time signal sent by the acquisition task
    cplx *buf = (cplx *)malloc(sizeof(cplx) * FFT_BUFFER_SIZE_LEFT_CANAL);
    double *power = (double *)malloc(sizeof(double) * FFT_BUFFER_SIZE_LEFT_CANAL);
    if (rt_queue_create(&priv->mailbox_logging, "Monitoring queue", sizeof(message_logging_t), Q_UNLIMITED, Q_FIFO) != 0)
    {
        rt_printf("Could not create the audio queue\n");
        return;
    }
    data_t *message;
    RTIME now, previous;
    int bytes_received, size = 0;
    now = rt_timer_read();
    while (priv->ctl->running)
    {
        if (priv->ctl->audio_running)
        {
            // rt_event_wait(priv->ctl->control_event, AUDIO_RUNNING, NULL, EV_ANY, TM_INFINITE);
            previous = now;
            if ((bytes_received = rt_queue_receive(&priv->mailbox_treatment, (void **)&message, TM_INFINITE)) <= 0)
            {
                rt_printf("Could not receive data from the audio queue\n");
            }
            memcpy(x + size, message, bytes_received);
            rt_queue_free(&priv->mailbox_treatment, message);
            size += bytes_received;
            bytes_received = 0;
            if (size >= FFT_BUFFER_SIZE)
            {
                previous = rt_timer_read();
                size = 0;
                for (size_t i = 0; i < FFT_BUFFER_SIZE_LEFT_CANAL; i++)
                {
                    buf[i] = x[i * 2] + 0 * I; // Only take left channel !!!
                }
                fft(buf, out, FFT_BUFFER_SIZE_LEFT_CANAL);
                double max_power = 0;
                int max_index = 0;
                for (size_t i = 1; i < (FFT_BUFFER_SIZE_LEFT_CANAL / 2); i++) // Skip the first element
                {
                    double re = crealf(buf[i]);
                    double im = cimagf(buf[i]);
                    power[i] = re * re + im * im;

                    if (power[i] > max_power)
                    {
                        max_power = power[i];
                        max_index = i;
                    }
                }

                message_logging_t *message = rt_queue_alloc(&priv->mailbox_logging, sizeof(message_logging_t));
                if (message == NULL)
                {
                    rt_printf("Could not allocate memory for the frequency\n");
                    break;
                }
                now = rt_timer_read();

                message->processing_time = now - previous;
                message->principal_freq = ((double)max_index * (double)SAMPLING) / (double)FFT_BUFFER_SIZE_LEFT_CANAL;
                rt_queue_send(&priv->mailbox_logging, message, sizeof(message_logging_t), Q_NORMAL);
                max_power = 0;
                max_index = 0;
            }
        }
    }
    free(out);
    free(x);
    free(buf);
    free(power);
    rt_queue_delete(&priv->mailbox_logging);
    rt_printf("Terminate treatment task\n");
}

void acquisition_task(void *cookie)
{
    Priv_audio_args_t *priv = (Priv_audio_args_t *)cookie;

    if (rt_queue_create(&priv->mailbox_treatment, "Audio Treatment Message Queue", FFT_BUFFER_SIZE, Q_UNLIMITED, Q_FIFO) != 0)
    {
        rt_printf("Error creating mailbox treatment\n");
        exit(EXIT_FAILURE);
    }

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
    RTIME previous, now;
    data_t *message;
    while (priv->ctl->running)
    {

        // Check if task is running before waiting to avoid going into waiting state
        message = rt_queue_alloc(&priv->mailbox_treatment, FIFO_SIZE * NB_CHAN);
        // Get the data from the buffer
        ssize_t read = read_samples(message, FIFO_SIZE * NB_CHAN);
        // write_samples(message, read);
        if (read)
        {
            // Write the data to the audio output
            rt_queue_send(&priv->mailbox_treatment, message, read, Q_NORMAL); // mettre en Q_NORMAL
        }

        rt_task_wait_period(NULL);
    }
    rt_queue_delete(&priv->mailbox_treatment);
    rt_printf("Terminating acquisition task\n");
}
