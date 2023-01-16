#pragma once

#include <atomic>
#include <stdint.h>
#include <string>

#include "fifo.h"


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

	void set_samplerate(const uint32_t sr);

public:
	rtl_sdr_tcp(const std::string & host, const int port);
	virtual ~rtl_sdr_tcp();

	auto get_iqs() {
		return iqs;
	}

	int get_sample_rate();

	void set_frequency(const uint32_t f);

	void operator()();
};
