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
#include "rtl_sdr_tcp.h"


void resample(const float *const in_float, const int in_rate, const int n_samples, float **const out, const int out_rate, int *const out_n_samples)
{
        double ratio = out_rate / double(in_rate);

        *out_n_samples = ceil(n_samples * ratio);

        *out = new float[*out_n_samples]();

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

int main(int argc, char *argv[])
{
	// rtl_sdr_tcp sdr_instance("10.208.30.7", 12345);
	rtl_sdr_tcp sdr_instance("192.168.64.226", 12444);

	sdr_instance.set_frequency(96000000);

	int  sample_rate = sdr_instance.get_sample_rate();

	auto iqs         = sdr_instance.get_iqs();

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

		float *resampled   = nullptr;
		int    n_resampled = 0;

		resample(decoded, sample_rate, n_values, &resampled, 8000, &n_resampled);

		// TODO

		delete [] resampled;
	}

	return 0;
}
