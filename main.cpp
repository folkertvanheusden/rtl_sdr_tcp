#include <atomic>
#include <math.h>
#include <samplerate.h>
#include <sndfile.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <unistd.h>

#include "fifo.h"
#include "net.h"


class rtl_sdr_tcp
{
private:
	struct iq_data {
		uint8_t data[4096];
	};

	fifo<iq_data>    *iqs       { nullptr };

	const std::string host;
	const int         port      { 1234    };

	std::thread      *th        { nullptr };
	std::atomic_bool  stop_flag { false   };

	int               fd        { -1      };

	std::atomic_uint32_t frequency { 96800000 };

	const int         sample_rate  { 1800000  };

	void set_samplerate(const uint32_t sr) {
		uint8_t msg[] = { 0x02, uint8_t(sr >> 24), uint8_t(sr >> 16), uint8_t(sr >> 8), uint8_t(sr) };

		if (WRITE(fd, reinterpret_cast<const char *>(msg), sizeof msg) == -1) {
			close(fd);

			fd = -1;
		}
	}

public:
	rtl_sdr_tcp(const std::string & host, const int port) : host(host), port(port) {
		iqs = new fifo<iq_data>(256);

		th  = new std::thread(std::ref(*this));
	}

	virtual ~rtl_sdr_tcp() {
		stop_flag = true;

		th->join();
		delete th;

		delete iqs;
	}

	auto get_iqs() { return iqs; }

	void set_frequency(const uint32_t f) {
		frequency = f;

		// TODO locking in case we were reconnecting concurrently

		uint8_t msg[] = { 0x01, uint8_t(f >> 24), uint8_t(f >> 16), uint8_t(f >> 8), uint8_t(f) };

		if (WRITE(fd, reinterpret_cast<const char *>(msg), sizeof msg) == -1) {
			close(fd);

			fd = -1;
		}
	}

	void operator()() {
		iq_data id;

		while(!stop_flag) {
			if (fd == -1) {
				fd = connect_to(host.c_str(), port);

				uint8_t dummy[12];
				if (READ(fd, dummy, 12) != 12) {  // header
					close(fd);

					fd = -1;
				}
			}

			if (fd != -1)
				set_frequency(frequency);

			if (fd != -1)
				set_samplerate(sample_rate);

			if (fd == -1)
				continue;

			if (READ(fd, id.data, sizeof id.data) != sizeof id.data) {
				close(fd);

				fd = -1;
			}

			if (!iqs->try_put(id))
				printf("buffer full\n");
		}

		close(fd);
	}
};

void resample(const float *const in_float, const int in_rate, const int n_samples, float **const out, const int out_rate, int *const out_n_samples)
{
        double ratio = out_rate / double(in_rate);
        *out_n_samples = ceil(n_samples * ratio);

        *out = new float[*out_n_samples];

        SRC_DATA sd;
        sd.data_in = in_float;
        sd.data_out = *out;
        sd.input_frames = n_samples;
        sd.output_frames = *out_n_samples;
        sd.input_frames_used = 0;
        sd.output_frames_gen = 0;
        sd.end_of_input = 0;
        sd.src_ratio = ratio;

        int rc = -1;
        if ((rc = src_simple(&sd, SRC_SINC_BEST_QUALITY, 1)) != 0)
                printf("SIP: resample failed: %s\n", src_strerror(rc));
}

int main(int argc, char *argv[])
{
	// rtl_sdr_tcp sdr_instance("10.208.30.7", 12345);
	rtl_sdr_tcp sdr_instance("192.168.64.226", 12444);

	sdr_instance.set_frequency(96000000);

	auto iqs = sdr_instance.get_iqs();

	for(;;) {
		auto   iq  = iqs->get();

		if (iq.has_value() == false)
			break;

		int    n_in = iqs->get_n_in();
		if (n_in)
			printf("%d\n", n_in);

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
	}

	return 0;
}
