
#include <alsa/asoundlib.h>
#include <alsa/seq.h>
#include <sys/time.h>
#include <sched.h>

void
callback(snd_seq_t *handle)
{
    snd_seq_event_t *ev = 0;

    do {
        if (snd_seq_event_input(handle, &ev) > 0) {

            if (ev->type == SND_SEQ_EVENT_NOTEON) {

                struct timeval tv;
                static long last_usec = 0;
                int pitch = ev->data.note.note;

                snd_seq_timestamp_t evt = ev->time;

                gettimeofday(&tv, 0);
                printf("pitch %d at %ld sec %ld usec, off by %ld usec\n",
                        pitch, tv.tv_sec, tv.tv_usec, tv.tv_usec - ((last_usec + 500000) % 1000000));

                last_usec = tv.tv_usec;
            }
        }

    } while (snd_seq_event_input_pending(handle, 0) > 0);
}

int
main(int argc, char **argv)
{
    snd_seq_t *handle;
    int portid;
    int npfd;
    struct pollfd *pfd;
    struct sched_param param;

    if (snd_seq_open(&handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        fprintf(stderr, "failed to open ALSA sequencer interface\n");
        return 1;
    }

    snd_seq_set_client_name(handle, "complainer");

    if ((portid = snd_seq_create_simple_port
            (handle, "complainer",
                    SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, 0)) < 0) {
        fprintf(stderr, "failed to create ALSA sequencer port\n");
        return 1;
    }

    npfd = snd_seq_poll_descriptors_count(handle, POLLIN);
    pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
    snd_seq_poll_descriptors(handle, pfd, npfd, POLLIN);

    param.sched_priority = 99;
    if (sched_setscheduler(0, SCHED_FIFO, &param)) {
        perror("failed to set high-priority scheduler");
    }

    printf("ready... npfd: %d\n", npfd);

    while (1) {
        if (poll(pfd, npfd, 100000) > 0) {
            callback(handle);
        }
    }
}

