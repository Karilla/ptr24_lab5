#include "audio_setup.h"
#include "control.h"
#include <string.h>
#include <stdlib.h>

// send 8192 byte to treatment task
#define FFT_BUFFER_SIZE 8192

RT_QUEUE mail_box;

void monitoring_task(void *cookie)
{
    rt_printf("=== Audio Monitoring ===\n");
    rt_printf("Audio task execution time : 0 Hz\n");
    rt_printf("FFT result : 0 Hz\n\n");
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

        /* DO SOMETHING with those values */
    }
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
