#include <math.h>
#include <samplerate.h>
#include <sndfile.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "filter.h"
#include "net.h"


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

	printf("%ld %ld %d\n", sd.input_frames_used, sd.output_frames_gen, sd.end_of_input);
}

void set_frequency(const int fd, const uint32_t f)
{
	uint8_t msg[] = { 0x01, f >> 24, f >> 16, f >> 8, f };

	WRITE(fd, reinterpret_cast<const char *>(msg), sizeof msg);
}

void set_samplerate(const int fd, const uint32_t sr)
{
	uint8_t msg[] = { 0x02, sr >> 24, sr >> 16, sr >> 8, sr };

	WRITE(fd, reinterpret_cast<const char *>(msg), sizeof msg);
}

int main(int argc, char *argv[])
{
	printf("init\n");

	// host, ip of the rtl_tcp server
	int fd = connect_to("192.168.64.226", 12444);
	//int fd = connect_to("10.208.30.7", 12345);
	if (fd == -1) {
		fprintf(stderr, "Cannot connect to host\n");
		return 1;
	}

	uint8_t dummy[12];
	READ(fd, dummy, 12);  // header

	//uint64_t f = 172274300;
	uint64_t f = 96000000;
	uint64_t f_target = 12000;

	set_frequency(fd, f);  // tune

	constexpr int samplerate = 1800000;  // 1.8MHz
	constexpr int duration = 10;  // 60s

	set_samplerate(fd, samplerate);

	uint8_t *iq = new uint8_t[samplerate * 2 * duration];

	printf("starting sampling\n");

	printf("read: %zd\n", READ(fd, iq, samplerate * 2 * duration));

	printf("sampled\n");

	float *buffer = new float[samplerate * duration]();

	constexpr float gain = 1.f;

	FilterButterworth hp(100.0, samplerate, true, sqrt(2.0));
	FilterButterworth lp(48000.0, samplerate, false, sqrt(2.0));

	for(int k=1; k<samplerate * duration; k++) {
		double y_t = sin(2. * M_PI * f_target * k / samplerate);

		int o = k * 2;

		double in = (iq[o + 0] - 128) / 128.0;
		double qn = (iq[o + 1] - 128) / 128.0;

		double in1 = (iq[o + 0 - 2] - 128) / 128.0;
		double qn1 = (iq[o + 1 - 2] - 128) / 128.0;

		buffer[k] = (in1 * qn - in * qn1) / (pow(in, 2.) + pow(qn, 2.));

		//buffer[k] = lp.apply(hp.apply(shifted));
		//buffer[k] = shifted;
	}

	printf("converted\n");

	close(fd);

	constexpr int samplerate_audio = 48000;

	SF_INFO si;
	si.frames = 0;
	si.samplerate = samplerate_audio;
	si.channels = 1;
	si.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	si.sections = 0;
	si.seekable = 0;

	SNDFILE *sf = sf_open("test.wav", SFM_WRITE, &si);

	int out_n = 0;
	float *out = nullptr;
	resample(buffer, samplerate, samplerate * duration, &out, samplerate_audio, &out_n);

	printf("resampled %d -> %d\n", samplerate * duration, out_n);

	sf_writef_float(sf, out, out_n);

	sf_close(sf);

	return 0;
}
