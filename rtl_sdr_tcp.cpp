#include <atomic>
#include <math.h>
#include <sndfile.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <unistd.h>

#include "fifo.h"
#include "net.h"
#include "rtl_sdr_tcp.h"
#include "time.h"


void rtl_sdr_tcp::set_samplerate(const uint32_t sr)
{
	std::unique_lock<std::mutex> lck(fd_lock);

	uint8_t msg[] = { 0x02, uint8_t(sr >> 24), uint8_t(sr >> 16), uint8_t(sr >> 8), uint8_t(sr) };

	if (WRITE(fd, reinterpret_cast<const char *>(msg), sizeof msg) == -1) {
		close(fd);

		fd = -1;
	}
}

rtl_sdr_tcp::rtl_sdr_tcp(const std::string & host, const int port) : host(host), port(port)
{
	iqs = new fifo<iq_data>(256);

	th  = new std::thread(std::ref(*this));
}

rtl_sdr_tcp::~rtl_sdr_tcp()
{
	stop_flag = true;

	th->join();
	delete th;

	delete iqs;
}

int rtl_sdr_tcp::get_sample_rate()
{
	return sample_rate;
}

void rtl_sdr_tcp::set_frequency(const uint32_t f)
{
	frequency = f;

	std::unique_lock<std::mutex> lck(fd_lock);

	uint8_t msg[] = { 0x01, uint8_t(f >> 24), uint8_t(f >> 16), uint8_t(f >> 8), uint8_t(f) };

	if (WRITE(fd, reinterpret_cast<const char *>(msg), sizeof msg) == -1) {
		close(fd);

		fd = -1;
	}
}

void rtl_sdr_tcp::operator()()
{
	iq_data id;

	while(!stop_flag) {
		if (fd == -1) {
			fd = connect_to(host.c_str(), port);

			uint8_t dummy[12];
			if (READ(fd, dummy, 12) != 12) {  // header
				close(fd);

				fd = -1;
			}

			if (fd != -1)
				set_frequency(frequency);

			if (fd != -1)
				set_samplerate(sample_rate);
		}

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
