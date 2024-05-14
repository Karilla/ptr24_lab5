#include "audio_setup.h"
#include "control.h"
#include <string.h>
#include <stdlib.h>

// send 8192 byte to treatment task
#define FFT_BUFFER_SIZE 8192

void monitoring_task(void *cookie)
{
    Priv_audio_args_t *priv = (Priv_audio_args_t *)cookie;

    message_logging_t message;

    while (*priv->running)
    {
        rt_queue_read(&priv->mailbox_logging, &message, sizeof(log_t), TM_INFINITE);

        rt_printf("Audio task execution time : %d Hz\n", message->processing_time);
        rt_printf("FFT result : %d Hz\n\n", message->principal_frequency);
    }

    rt_printf("Terminate monitoring task\n");
}

void treatment_task(void *cookie)
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
     * échantillon sur deux. */

    Priv_audio_args_t *priv = (Priv_audio_args_t *)cookie;

    cplx *out = malloc(sizeof(cplx) * FFT_BINS); // Auxiliary array for fft function
    data_t *x;                                   // NOTE : discrete time signal sent by the acquisition task
    cplx *buf = malloc(sizeof(cplx) * FFT_BINS);
    double *power = malloc(sizeof(double) * FFT_BINS);

    buf = memcpy(buf, priv->samples_buf, FFT_BINS * sizeof(cplx)); // Copy the data to the buffer

    message_treatment_t message;

    while (*priv->running)
    {
        rt_queue_read(&priv->mailbox_treatment, &message, sizeof(log_t), TM_INFINITE);

        buf = memcpy(buf, message->samples_buf, FFT_BINS * sizeof(cplx)); // Copy the data to the buffer

        RTIME start = rt_timer_read();

        /* EXAMPLE using the fft function : */
        for (size_t i = 0; i < FFT_BINS; i++)
        {
            buf[i] = x[i * 2] + 0 * I; // Only take left channel !!!
        }
        fft(buf, out, FFT_BINS);
        for (size_t i = 0; i < FFT_BINS; i++)
        {
            double re = crealf(buf[i]);
            double im = cimagf(buf[i]);
            power[i] = re * re + im * im;
        }

        size_t freq_max = 0; //dominant_freq(power, FFT_BINS);
        message->max_freqency = freq_max;

        RTIME stop = rt_timer_read();
        message->processing_time = stop - start;

        rt_queue_send(&priv->mailbox_treatment, message, sizeof(log_t), Q_NORMAL);
    }

    rt_printf("Terminate treatment task\n");

    free(out);
    free(buf);
    free(power);
}

void acquisition_task(void *cookie)
{
    Priv_audio_args_t *priv = (Priv_audio_args_t *)cookie;

    RT_TASK treatment;

    int err;
    // Calculate the minimum frequency of the task
    uint32_t audio_task_freq = (SAMPLING * sizeof(data_t)) / (FIFO_SIZE);
    // Calculate the period of the task with a margin to ensure not any data is lost
    uint64_t period_in_ns = S_IN_NS / (audio_task_freq + PERIOD_MARGIN);

    uint16_t fft_buffer[8192];

    uint32_t nb_byte_readen = 0;

    if (rt_task_create(&treatment, "Treatement Task", 0, 99, T_JOINABLE))
    {
        rt_printf("Error during creation of treatement task\nExiting.....\n");
        exit(EXIT_SUCCESS);
    }

    if ((err = rt_task_set_periodic(NULL, TM_NOW, period_in_ns)) < 0)
    {
        rt_printf("Could not set period of acquisition task\n");
        return;
    }

    while (priv->ctl->running)
    {
        // Check if task is running before waiting to avoid going into waiting state
        if (!ctl->audio_running)
        {
            rt_event_wait(priv->control_event, AUDIO_RUNNING, NULL, TM_INFINITE);
        }
        // Get the data from the buffer
        ssize_t read = read_samples(priv->samples_buf, FIFO_SIZE * NB_CHAN);
        if (read)
        {
            if (nb_byte_readen + read > FFT_BUFFER_SIZE)
            {
                // On demarre la tache traitement et on lui envoie le buffer plus la taille du buffer
                nb_byte_readen = 0;
                rt_task_start(&treatement, &treatment_task, priv);
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
    rt_printf("Terminating acquisition task\n");
}
