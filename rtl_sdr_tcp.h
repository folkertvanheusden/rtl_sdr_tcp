#pragma once

#include <atomic>
#include <stdint.h>
#include <string>

#include "fifo.h"


constexpr const int sdr_sample_rate = 1800000;
constexpr const int fragment_size   = sdr_sample_rate / 10;

class rtl_sdr_tcp
{
private:
	struct iq_data {
		uint8_t data[fragment_size];
	};

	fifo<iq_data>    *iqs       { nullptr };

	const std::string host;
	const int         port      { 1234    };

	std::thread      *th        { nullptr };
	std::atomic_bool  stop_flag { false   };

	int               fd        { -1      };

	std::atomic_uint32_t frequency { 96800000 };

	const int         sample_rate  { sdr_sample_rate  };

	void set_samplerate(const uint32_t sr);

public:
	rtl_sdr_tcp(const std::string & host, const int port);
	virtual ~rtl_sdr_tcp();

	auto get_iqs() {
		return iqs;
	}

	int get_sample_rate();

	int get_fragment_size() { return fragment_size; }

	void set_frequency(const uint32_t f);

	void operator()();
};
