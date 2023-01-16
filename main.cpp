// (C) 2023 by folkert van heusden <mail@vanheusden.com>, released under MIT License

#include <atomic>
#include <math.h>
#include <samplerate.h>
#include <sndfile.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <thread>
#include <unistd.h>

#include "fifo.h"
#include "net.h"
#include "rtl_sdr_tcp.h"
#include "sip.h"


static void resample(const float *const in_float, const int in_rate, const int n_samples, float **const out, const int out_rate, int *const out_n_samples)
{
        double ratio = out_rate / double(in_rate);

        *out_n_samples = ceil(n_samples * ratio);

        *out           = new float[*out_n_samples]();

        SRC_DATA sd;
        sd.data_in           = in_float;
        sd.data_out          = *out;
        sd.input_frames      = n_samples;
        sd.output_frames     = *out_n_samples;
        sd.input_frames_used = 0;
        sd.output_frames_gen = 0;
        sd.end_of_input      = 0;
        sd.src_ratio         = ratio;

        int rc = -1;
        if ((rc = src_simple(&sd, SRC_SINC_BEST_QUALITY, 1)) != 0)
                printf("SIP: resample failed: %s\n", src_strerror(rc));
}

static void * duplicate(const void *const org, const size_t size)
{
        void *temp = malloc(size);

        memcpy(temp, org, size);

        return temp;
}

class audio_source
{
private:
	rtl_sdr_tcp *const rst { nullptr };
	const int sample_rate  { 0       };

	std::mutex lock;
	float     *samples     { nullptr };
	size_t     n_samples   { 0       };
	std::condition_variable cv;
	uint64_t   t           { 0       };

	std::thread *th        { nullptr };

public:
	audio_source(rtl_sdr_tcp *const rst, const int sample_rate) : rst(rst), sample_rate(sample_rate) {
		th = new std::thread(std::ref(*this));
	}

	virtual ~audio_source() {
		th->join();
		delete th;
	}

	void operator()() {
		int  sample_rate = rst->get_sample_rate();

		auto iqs         = rst->get_iqs();

		int  max_n_in    = 1;

		for(;;) {
			auto   iq  = iqs->get();

			if (iq.has_value() == false)
				break;

			int    n_in = iqs->get_n_in();
			if (n_in >= max_n_in) {
				max_n_in = n_in;

				printf("%d\n", n_in);
			}

			auto   values = iq.value().data;

			int    n_values = sizeof(values) / 2;

			float  decoded[n_values];

			double in1    = 0;
			double qn1    = 0;

			for(int i=0; i<n_values; i++) {
				int o = i * 2;

				double in = (values[o + 0] - 128) / 128.0;
				double qn = (values[o + 1] - 128) / 128.0;

				decoded[i] = (in1 * qn - in * qn1) / (in * in + qn * qn);

				in1 = in;
				qn1 = qn;
			}

			float *resampled   = nullptr;
			int    n_resampled = 0;

			resample(decoded, sample_rate, n_values, &resampled, sample_rate, &n_resampled);

			{
				std::unique_lock<std::mutex> lck(lock);

				delete [] samples;

				samples   = resampled;
				n_samples = n_resampled;

				t++;

				cv.notify_all();
			}
		}
	}

	void wait_and_get(const uint64_t after, float **const samples, size_t *const n_samples, uint64_t *const t)
	{
		std::unique_lock<std::mutex> lck(lock);

		while(after >= this->t)
			cv.wait(lck);
		
		*samples   = reinterpret_cast<float *>(duplicate(this->samples, this->n_samples * sizeof(float)));
		*n_samples = this->n_samples;
		*t         = this->t;
	}
};

typedef struct {
	rtl_sdr_tcp  *sdr_instance;
	audio_source *as;
} context_t;

typedef struct {
	uint8_t     prev_key;
	std::string keys_pressed;
	uint64_t    t;
} session_t;

bool cb_new_session(sip_session_t *const session, const std::string & from)
{
	printf("cb_new_session, call-id: %s, caller: %s\n", session->call_id.c_str(), from.c_str());

	session->private_data = new session_t();

	return true;
}

// no audio, just dtmf

// invoked when the peer produces audio and which is then
// received by us
bool cb_recv(const short *const samples, const size_t n_samples, sip_session_t *const session)
{
	// ignore
	return true;
}

// invoked when the library wants to send audio to
// the peer
bool cb_send(short **const samples, size_t *const n_samples, sip_session_t *const session)
{
	session_t *p = reinterpret_cast<session_t *>(session->private_data);

	float *f   = nullptr;
	size_t n_f = 0;
	reinterpret_cast<context_t *>(session->global_private_data)->as->wait_and_get(p->t, &f, &n_f, &p->t);

        *samples   = new short[n_f];
	*n_samples = n_f;
        src_float_to_short_array(f, *samples, *n_samples);

	free(f);

	return true;
}

// called when we receive a 'BYE' from the peer (and
// the session thus ends)
void cb_end_session(sip_session_t *const session)
{
	printf("cb_end_session, call-id: %s\n", session->call_id.c_str());

	session_t *p = reinterpret_cast<session_t *>(session->private_data);

	delete p;
}

bool cb_dtmf(const uint8_t dtmf_code, const bool is_end, const uint8_t volume, sip_session_t *const session)
{
	session_t *p = reinterpret_cast<session_t *>(session->private_data);

	if (is_end && dtmf_code != p->prev_key) {
		printf("DTMF pressed: %d\n", dtmf_code);

		if (dtmf_code == 11) {  // '#'
			uint32_t f = std::atol(p->keys_pressed.c_str()) * 1000l;

			printf("Set frequency to %d\n", f);

			reinterpret_cast<context_t *>(session->global_private_data)->sdr_instance->set_frequency(f);

			p->keys_pressed.clear();
		}
		else if (dtmf_code < 10) {
			p->keys_pressed += char(dtmf_code + '0');
		}

		p->prev_key = dtmf_code;
	}

	if (!is_end)
		p->prev_key = 255;

	return true;
}

int main(int argc, char *argv[])
{
	// rtl_sdr_tcp sdr_instance("10.208.30.7", 12345);
	rtl_sdr_tcp sdr_instance("192.168.64.226", 12444);

	sdr_instance.set_frequency(96000000);

	audio_source as(&sdr_instance, 44100);

	context_t c { &sdr_instance, &as };

	// sip s("10.208.11.13", "3737", "1234", { }, 0, 60, 44100, cb_new_session, cb_recv, cb_send, cb_end_session, cb_dtmf, &c);
	sip s("192.168.64.13", "3737", "1234", { }, 0, 60, 44100, cb_new_session, cb_recv, cb_send, cb_end_session, cb_dtmf, &c);

	printf("started\n");

	pause();

	return 0;
}
