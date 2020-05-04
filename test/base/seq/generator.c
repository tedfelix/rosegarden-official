
#include <alsa/asoundlib.h>
#include <alsa/seq.h>
#include <sys/time.h>

int
main(int argc, char **argv)
{
    snd_seq_t *handle;
    int portid;
    int npfd;
    struct pollfd *pfd;
    int queue;
    int i;
    int rval;
    int target;
    snd_seq_queue_timer_t *timer;
    snd_timer_id_t *timerid;

    if (argc != 2) {
	fprintf(stderr, "usage: generator <target-client-id>\n");
	exit(2);
    }
    target = atoi(argv[1]);

    if (snd_seq_open(&handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
	fprintf(stderr, "failed to open ALSA sequencer interface\n");
	return 1;
    }

    snd_seq_set_client_name(handle, "generator");

    if ((portid = snd_seq_create_simple_port
	 (handle, "generator",
	  SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ, 0)) < 0) {
	fprintf(stderr, "failed to create ALSA sequencer port\n");
	return 1;
    }

    if ((queue = snd_seq_alloc_queue(handle)) < 0) {
	fprintf(stderr, "failed to create ALSA sequencer queue\n");
	return 1;
    }
/*
    snd_seq_queue_timer_alloca(&timer);
    snd_seq_get_queue_timer(handle, queue, timer);
    snd_timer_id_alloca(&timerid);
    snd_timer_id_set_class(timerid, SND_TIMER_CLASS_PCM);
    snd_timer_id_set_sclass(timerid, SND_TIMER_SCLASS_NONE);
    snd_timer_id_set_card(timerid, 0);
    snd_timer_id_set_device(timerid, 0);
    snd_timer_id_set_subdevice(timerid, 0);
    snd_seq_queue_timer_set_id(timer, timerid);
    snd_seq_set_queue_timer(handle, queue, timer);
*/
    snd_seq_start_queue(handle, queue, 0);

    // stuff two minutes worth of events on the queue
    for (i = 0; i < 240; ++i) {
	snd_seq_real_time_t rtime;
	rtime.tv_sec = i / 2;
	rtime.tv_nsec = (i % 2) * 500000000;
	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, portid);
	snd_seq_ev_set_dest(&ev, target, 0);
	snd_seq_ev_schedule_real(&ev, queue, 0, &rtime);
	snd_seq_ev_set_noteon(&ev, 0, 64, 127);
	if ((rval = snd_seq_event_output(handle, &ev)) < 0) {
	    fprintf(stderr, "failed to write event: %s", snd_strerror(rval));
	}
    }

    snd_seq_drain_output(handle);

    for (i = 0; i < 120; ++i) {
	snd_seq_queue_status_t *status;
	const snd_seq_real_time_t *rtime;
	struct timeval tv;

	snd_seq_queue_status_alloca(&status);

	snd_seq_get_queue_status(handle, queue, status);
	rtime = snd_seq_queue_status_get_real_time(status);

	gettimeofday(&tv, 0);

	fprintf(stderr, " real time: %ld sec, %ld usec\nqueue time: %u sec, %u usec (diff to real time %ld sec %ld usec)\n",
		tv.tv_sec, tv.tv_usec,
		rtime->tv_sec, rtime->tv_nsec / 1000,
		tv.tv_sec - rtime->tv_sec, tv.tv_usec - (rtime->tv_nsec / 1000));

	sleep(1);
    }
}

