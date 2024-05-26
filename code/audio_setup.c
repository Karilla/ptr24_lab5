#include "audio_setup.h"
#include <alchemy/task.h>
#include "utils/fft_utils.h"
#include "control.h"
#include <complex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// send 8192 byte to treatment task
#define FFT_BUFFER_SIZE 8192

typedef double complex cplx;

void monitoring_task(void *cookie)
{
    rt_printf("On monitore ou quoi ?\n");
    Priv_audio_sub_args_t *priv = (Priv_audio_sub_args_t *)cookie;

    message_logging_t message;

    while (priv->ctl->running)
    {
        rt_queue_read(&priv->mailBox, &message, sizeof(message_logging_t), TM_INFINITE);

        rt_printf("Audio task execution time : %d Hz\n", message.processing_time);
        rt_printf("FFT result : %d Hz\n\n", message.principal_freq);
    }

    rt_printf("Terminate monitoring task\n");
}

void treatment_task_t_lol(void *cookie)
{ /*
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

    Priv_audio_sub_args_t *priv = (Priv_audio_sub_args_t *)cookie;

    cplx *out = (cplx *)malloc(sizeof(cplx) * FFT_BUFFER_SIZE); // Auxiliary array for fft function

    data_t *x; // NOTE : discrete time signal sent by the acquisition task
    cplx *buf = (cplx *)malloc(sizeof(cplx) * FFT_BUFFER_SIZE);
    double *power = (cplx *)malloc(sizeof(cplx) * FFT_BUFFER_SIZE);

    message_treatment_t message;

    while (priv->ctl->running)
    {

        rt_queue_read(&priv->mailBox, &message, sizeof(message_treatment_t), TM_INFINITE);

        memcpy(x, message.samples_buf, FFT_BUFFER_SIZE * sizeof(cplx)); // Copy the data to the buffer

        RTIME start = rt_timer_read();

        /* EXAMPLE using the fft function : */

        for (size_t i = 0; i < FFT_BUFFER_SIZE / 2; i++)
        {
            buf[i] = x[i * 2] + 0 * I; // Only take left channel !!!
        }
        fft(buf, out, FFT_BUFFER_SIZE);
        for (size_t i = 0; i < FFT_BUFFER_SIZE; i++)
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

        rt_queue_send(&message.mailbox_logging, &loggin_message, sizeof(message_logging_t), Q_NORMAL);
    }

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

    uint16_t fft_buffer[FFT_BUFFER_SIZE];

    uint32_t nb_byte_readen = 0;

    // Create messagebox for both treatment and logging task
    RT_QUEUE mailbox_treatment;
    RT_QUEUE mailbox_logging;

    if (rt_queue_create(&mailbox_treatment, "Audio Treatment Message Queue", sizeof(message_treatment_t), Q_UNLIMITED, Q_FIFO) != 0)
    {
        rt_printf("OPS\n");
        exit(EXIT_FAILURE);
    }
    if (rt_queue_create(&mailbox_logging, "Audio MonmailBoxitoring Message Queue", sizeof(message_logging_t), Q_UNLIMITED, Q_FIFO) != 0)
    {
        rt_printf("Error creating mailbox logging\n");
        exit(EXIT_FAILURE);
    }

    Priv_audio_sub_args_t treatment_task_arg = {0};
    treatment_task_arg.ctl = priv->ctl;
    treatment_task_arg.mailBox = mailbox_treatment;

    rt_printf("task.ctl = %p\n", &treatment_task_arg.ctl);
    rt_printf("task.mailBox = %p\n", &treatment_task_arg.mailBox);
    rt_printf("task.sub_task = %p\n", &treatment_task_arg.sub_task);

    if (rt_task_spawn(&treatment_task_arg.sub_task, "Treatment Task", 0, 50, T_JOINABLE, treatment_task_t_lol, &treatment_task_arg))
    {
        rt_printf("Error while launching treatment task\n");
        exit(EXIT_FAILURE);
    }
    rt_task_sleep(1000);

    Priv_audio_sub_args_t monitoring_task_args = {0};
    monitoring_task_args.ctl = priv->ctl;
    monitoring_task_args.mailBox = mailbox_logging;

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
        // Get the data from the buffer
        ssize_t read = read_samples(priv->samples_buf, FIFO_SIZE * NB_CHAN);
        write_samples(priv->samples_buf, read);

        if (read)
        {
            if (nb_byte_readen + read > FFT_BUFFER_SIZE)
            {
                // On demarre la tache traitement et on lui envoie le buffer plus la taille du buffer
                nb_byte_readen = 0;
                message.samples_buf = fft_buffer;
                message.mailbox_logging = mailbox_logging; // A FIX CEST HORRIBLE
                rt_queue_send(&mailbox_treatment, &message, sizeof(message_treatment_t), Q_NORMAL);
                memset(priv->samples_buf, 0, FFT_BUFFER_SIZE);
            }
            else
            {
                memcpy(fft_buffer + nb_byte_readen, priv->samples_buf, read);
                nb_byte_readen += read;
            }
            // Write the data to the audio output
            write_samples(priv->samples_buf, read);
        }

        rt_task_wait_period(NULL);
    }
    // rt_queue_free(mailbox_treatment, message);
    rt_printf("Terminating acquisition task\n");
}
