#include "audio_setup.h"
#include <alchemy/task.h>
#include <alchemy/heap.h>
#include "utils/fft_utils.h"
#include "control.h"
#include <complex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// send 8192 byte to treatment task
#define FFT_BUFFER_SIZE 8192
#define FFT_BUFFER_SIZE_LEFT_CANAL 8192 / 2

typedef double complex cplx;

void monitoring_task(void *cookie)
{
    rt_printf("On monitore ou quoi ?\n");
    Priv_audio_logging_args_t *priv = (Priv_audio_logging_args_t *)cookie;

    message_logging_t message;

    while (priv->ctl->running)
    {
        rt_queue_read(&priv->mailbox_logging, &message, sizeof(message_logging_t), TM_INFINITE);

        rt_printf("Audio task execution time : %d \n", message.processing_time);
        rt_printf("FFT result : %d Hz\n\n", message.principal_freq);
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

    Priv_audio_treatment_args_t *priv = (Priv_audio_treatment_args_t *)cookie;

    cplx *out;
    rt_heap_alloc(priv->heap, FFT_BUFFER_SIZE_LEFT_CANAL * sizeof(cplx), TM_INFINITE, (void **)&out);

    data_t *x; // NOTE : discrete time signal sent by the acquisition task
    rt_heap_alloc(priv->heap, FFT_BUFFER_SIZE_LEFT_CANAL * sizeof(data_t), TM_INFINITE, (void **)&x);
    cplx *buf;
    rt_heap_alloc(priv->heap, FFT_BUFFER_SIZE_LEFT_CANAL * sizeof(cplx), TM_INFINITE, (void **)&buf);
    double *power;
    rt_heap_alloc(priv->heap, FFT_BUFFER_SIZE_LEFT_CANAL * sizeof(double), TM_INFINITE, (void **)&power);

    message_treatment_t message;

    while (priv->ctl->running)
    {
        rt_printf("On attends lol\n");
        rt_queue_read(&priv->mailbox_treatment, &message, sizeof(message_treatment_t), TM_INFINITE);
        rt_printf("On attends mais c'est fini\n");
        memcpy(x, message.samples_buf, FFT_BUFFER_SIZE * sizeof(data_t)); // Copy the data to the buffer

        RTIME start = rt_timer_read();

        /* EXAMPLE using the fft function : */

        for (size_t i = 0; i < FFT_BUFFER_SIZE_LEFT_CANAL; i++)
        {
            buf[i] = x[i * 2] + 0 * I; // Only take left channel !!!
        }
        fft(buf, out, FFT_BUFFER_SIZE_LEFT_CANAL);
        for (size_t i = 0; i < FFT_BUFFER_SIZE_LEFT_CANAL; i++)
        {
            double re = crealf(buf[i]);
            double im = cimagf(buf[i]);
            power[i] = re * re + im * im;
        }

        message_logging_t loggin_message;

        size_t freq_max = 0; // dominant_freq(power, FFT_BINS);
        loggin_message.principal_freq = freq_max;

        RTIME stop = rt_timer_read();
        loggin_message.processing_time = stop - start;

        rt_queue_send(&priv->mailbox_logging, &loggin_message, sizeof(message_logging_t), Q_NORMAL);
    }
    rt_heap_free(priv->heap, x);
    rt_heap_free(priv->heap, buf);
    rt_heap_free(priv->heap, power);
    rt_printf("Terminate treatment task\n");
}

void acquisition_task(void *cookie)
{
    Priv_audio_args_t *priv = (Priv_audio_args_t *)cookie;

    int err;
    // Calculate the minimum frequency of the task
    uint32_t audio_task_freq = (SAMPLING * sizeof(data_t)) / (FIFO_SIZE);
    // Calculate the period of the task with a margin to ensure not any data is lost
    uint64_t period_in_ns = S_IN_NS / (audio_task_freq + PERIOD_MARGIN);
    uint16_t *fft_buffer;
    rt_heap_alloc(priv->heap, FFT_BUFFER_SIZE * sizeof(uint16_t), TM_INFINITE, (void **)&fft_buffer);

    uint32_t nb_byte_readen = 0;

    // Create messagebox for both treatment and logging task
    RT_QUEUE mailbox_treatment;
    RT_QUEUE mailbox_logging;

    if (rt_queue_create(&mailbox_treatment, "Audio Treatment Message Queue", sizeof(message_treatment_t), Q_UNLIMITED, Q_FIFO) != 0)
    {
        rt_printf("Error creating mailbox treatment\n");
        exit(EXIT_FAILURE);
    }
    if (rt_queue_create(&mailbox_logging, "Audio MonmailBoxitoring Message Queue", sizeof(message_logging_t), Q_UNLIMITED, Q_FIFO) != 0)
    {
        rt_printf("Error creating mailbox logging\n");
        exit(EXIT_FAILURE);
    }

    Priv_audio_treatment_args_t treatment_task_arg = {0};
    treatment_task_arg.ctl = priv->ctl;
    treatment_task_arg.mailbox_logging = mailbox_logging;
    treatment_task_arg.mailbox_treatment = mailbox_treatment;

    if (rt_task_spawn(&treatment_task_arg.sub_task, "Treatment Task", 0, 50, T_JOINABLE, treatment_task_t_lol, &treatment_task_arg))
    {
        rt_printf("Error while launching treatment task\n");
        exit(EXIT_FAILURE);
    }
    rt_task_sleep(1000);

    Priv_audio_logging_args_t monitoring_task_args = {0};
    monitoring_task_args.ctl = priv->ctl;
    monitoring_task_args.mailbox_logging = mailbox_logging;

    if (rt_task_spawn(&monitoring_task_args.sub_task, "Monitoring Task", 0, 50, T_JOINABLE, monitoring_task, &monitoring_task_args))
    {
        rt_printf("Error while launching monitoring task\n");
        exit(EXIT_FAILURE);
    }
    if ((err = rt_task_set_periodic(NULL, TM_NOW, period_in_ns)) < 0)
    {
        rt_printf("Could not set period of acquisition task\n");
        return;
    }

    message_treatment_t message;
    if (rt_queue_alloc(&mailbox_treatment, sizeof(message_treatment_t)) == NULL)
    {
        rt_printf("Oups \n");
        exit(EXIT_FAILURE);
    }

    while (priv->ctl->running)
    {
        // Check if task is running before waiting to avoid going into waiting state
        if (!priv->ctl->audio_running)
        {
            rt_event_wait(priv->ctl->control_event, AUDIO_RUNNING, NULL, EV_ANY, TM_INFINITE);
        }
#if MEASURE
        previous = rt_timer_read();
#endif
        // Get the data from the buffer
        ssize_t read = read_samples(priv->samples_buf, FIFO_SIZE * NB_CHAN);
        write_samples(priv->samples_buf, read);

        if (read)
        {
            if (nb_byte_readen + read > FFT_BUFFER_SIZE)
            {
                // On complete le buffer
                int leftover = FFT_BUFFER_SIZE - read;
                memcpy(fft_buffer + nb_byte_readen, priv->samples_buf, leftover);
                // On demarre la tache traitement et on lui envoie le buffer plus la taille du buffer
                nb_byte_readen = 0;
                message.samples_buf = fft_buffer;
                rt_queue_send(&mailbox_treatment, &message, sizeof(message_treatment_t), Q_NORMAL);
                memset(priv->samples_buf, 0, FFT_BUFFER_SIZE);
            }
            else
            {
                memcpy(fft_buffer + nb_byte_readen, priv->samples_buf, read);
                nb_byte_readen += read;
            }
        }

        rt_task_wait_period(NULL);
    }
    rt_heap_free(priv->heap, fft_buffer);
    // rt_queue_free(mailbox_treatment, message);
    rt_task_join(&treatment_task_arg.sub_task);
    rt_task_join(&monitoring_task_args.sub_task);
    rt_printf("Terminating acquisition task\n");
}
