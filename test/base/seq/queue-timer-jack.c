
#include <alsa/asoundlib.h>
#include <alsa/seq.h>
#include <jack/jack.h>
#include <sys/time.h>

static jack_nframes_t sample_frames = 0;

void normalize(struct timeval *tv)
{
    if (tv->tv_sec == 0) {
	while (tv->tv_usec <= -1000000) { tv->tv_usec += 1000000; --tv->tv_sec; }
	while (tv->tv_usec >=  1000000) { tv->tv_usec -= 1000000; ++tv->tv_sec; }
    } else if (tv->tv_sec < 0) {
	while (tv->tv_usec <= -1000000) { tv->tv_usec += 1000000; --tv->tv_sec; }
	while (tv->tv_usec > 0) { tv->tv_usec -= 1000000; ++tv->tv_sec; }
    } else { 
	while (tv->tv_usec >= 1000000) { tv->tv_usec -= 1000000; ++tv->tv_sec; }
	while (tv->tv_usec < 0) { tv->tv_usec += 1000000; --tv->tv_sec; }
    }
}

int
jack_process(jack_nframes_t nframes, void *arg)
{
    sample_frames += nframes;
}

jack_nframes_t
rt_to_frame(struct timeval tv, jack_nframes_t sample_rate)
{
    if (tv.tv_sec < 0) tv.tv_sec = -tv.tv_sec;
    if (tv.tv_usec < 0) tv.tv_usec = -tv.tv_usec;
    return
	tv.tv_sec * sample_rate +
	((tv.tv_usec / 1000) * sample_rate) / 1000 +
	((tv.tv_usec - 1000 * (tv.tv_usec / 1000)) * sample_rate) / 1000000;
}

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
    struct timeval starttv;
    int countdown = -1;
    snd_seq_queue_timer_t *timer;
    snd_timer_id_t *timerid;
    jack_client_t *jclient;
    jack_nframes_t sample_rate;
    
    if ((jclient = jack_client_new("queue-timer-jack")) == 0) {
	fprintf(stderr, "failed to connect to JACK server\n");
	return 1;
    }

    jack_set_process_callback(jclient, jack_process, 0);

    sample_rate = jack_get_sample_rate(jclient);

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

    snd_seq_queue_timer_alloca(&timer);
    snd_seq_get_queue_timer(handle, queue, timer);
    snd_timer_id_alloca(&timerid);

    /* To test a PCM timer: */
/*
    snd_timer_id_set_class(timerid, SND_TIMER_CLASS_PCM);
    snd_timer_id_set_sclass(timerid, SND_TIMER_SCLASS_NONE);
    snd_timer_id_set_card(timerid, 0);
    snd_timer_id_set_device(timerid, 0);
    snd_timer_id_set_subdevice(timerid, 0);
*/

    /* To test the system timer: */
    snd_timer_id_set_class(timerid, SND_TIMER_CLASS_GLOBAL);
    snd_timer_id_set_sclass(timerid, SND_TIMER_SCLASS_NONE);
    snd_timer_id_set_device(timerid, SND_TIMER_GLOBAL_SYSTEM);

    snd_seq_queue_timer_set_id(timer, timerid);
    snd_seq_set_queue_timer(handle, queue, timer);

    if (jack_activate(jclient)) {
        fprintf (stderr, "cannot activate jack client");
        exit(1);
    }

    snd_seq_start_queue(handle, queue, 0);
    snd_seq_drain_output(handle);

    gettimeofday(&starttv, 0);

    while (countdown != 0) {

	snd_seq_queue_status_t *status;
	const snd_seq_real_time_t *rtime;
	struct timeval tv, qtv, jtv, diff, jdiff;
	jack_nframes_t frames_now;

	snd_seq_queue_status_alloca(&status);

	snd_seq_get_queue_status(handle, queue, status);
	rtime = snd_seq_queue_status_get_real_time(status);

	gettimeofday(&tv, 0);

	frames_now = sample_frames;
	fprintf(stderr, "    frames: %ld\n", frames_now);

	qtv.tv_sec = rtime->tv_sec;
	qtv.tv_usec = rtime->tv_nsec / 1000;

	tv.tv_sec -= starttv.tv_sec;
	tv.tv_usec -= starttv.tv_usec;
	normalize(&tv);

	jtv.tv_sec = frames_now / sample_rate;
	frames_now -= jtv.tv_sec * sample_rate;
	jtv.tv_usec = (int)(((float)frames_now * 1000000) / sample_rate);

	diff.tv_sec = tv.tv_sec - qtv.tv_sec;
	diff.tv_usec = tv.tv_usec - qtv.tv_usec;
	normalize(&diff);

	jdiff.tv_sec = jtv.tv_sec - qtv.tv_sec;
	jdiff.tv_usec = jtv.tv_usec - qtv.tv_usec;
	normalize(&jdiff);

	fprintf(stderr, " real time: %12ld sec %8ld usec /%12ld frames\nqueue time: %12ld sec %8ld usec /%12ld frames\n jack time: %12ld sec %8ld usec /%12ld frames\n   rq diff: %12ld sec %8ld usec /%12ld frames\n   jq diff: %12ld sec %8ld usec /%12ld frames\n",
		tv.tv_sec, tv.tv_usec, rt_to_frame(tv, sample_rate),
		qtv.tv_sec, qtv.tv_usec, rt_to_frame(qtv, sample_rate),
		jtv.tv_sec, jtv.tv_usec, rt_to_frame(jtv, sample_rate),
		diff.tv_sec, diff.tv_usec, rt_to_frame(diff, sample_rate),
		jdiff.tv_sec, jdiff.tv_usec, rt_to_frame(jdiff, sample_rate));

	fprintf(stderr, "\n");
	struct timespec ts;
	ts.tv_sec = 1;
	ts.tv_nsec = 0;
	nanosleep(&ts, 0);
    }
}

