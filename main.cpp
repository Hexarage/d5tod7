// used https://algo.inria.fr/seminars/sem94-95/hoshi.pdf for reference

#include <iostream>
#include <cstdint>
#include <limits>
#include <queue>
#include <random>
#include <array>

/*
	n rolls of the d5 produce a uniform integer in [0, 5^n); 
	Decompose it into k = floor(log_7 5^n) independent uniform base-7 digits by repeated division;
	the unused remainder is kept as recycled state, so we don't lose entropy
	Amortized cost approaches the information-theoretic floor of log_5(7) = 1.2091 d5 rolls per d7 roll.
	
	Make sure all of the math is on uint64_t and not any float/double so it is exact
	The state is a pair (v, c) meaning "a uniform value v in [0, c)"

	Extraction has to be done only when c lies in [7^21, 5*7^21): then c = m*7^21 + r
	  - v >= m*7^21:  keep (v - m*7^21) uniform in [0, r)      -> recycled state
	  - v <  m*7^21:  s = v % 7^21 gives 21 i.i.d. uniform d7 digits
	                  and v / 7^21, uniform in [0, m), is carried over as state.

*/

class D5toD7
{
public:
	explicit D5toD7(std::uint64_t seed) : rngSeed(seed) {}
	int roll() // get a single uniform roll of a d7 [1, 7]
	{
		while(buffer.empty())
		{
			refill();
		}

		const int r = buffer.front();
		buffer.pop();
		return r;
	}

	inline double rollsPerOutput()
	{
		return static_cast<double>(rollCount) / static_cast<double>(outCount);
	}

private:
	// the largest power of 7 that can fit in a uint64_t that still alows a *5 is 7^21
    static constexpr std::uint64_t kPow7 = 558545864083284007ULL;   // 7^21
	static_assert(kPow7 * 5 < std::numeric_limits<std::uint64_t>::max());

	int d5()
	{
		std::uint64_t r;
		do
		{
			r = rngSeed();
		} while(r == std::numeric_limits<uint64_t>::max());
		// 2^64 = 5*q + 1, meaning that a direct mod 5 is biased, value to be discarded is arbitrary, max is chosen due to
		// keeping the sequence continuous [0, uint64_t::max())
		++rollCount;
		return static_cast<int>(r % 5);
	}

	void refill()
	{
		while(c < kPow7) // append base 5 digits into c in [7^21, 5*7^21)
		{
			v = v * 5 + static_cast<std::uint64_t>(d5());
			c *= 5;
		}

		const std::uint64_t m = c / kPow7;
		const std::uint64_t r = c % kPow7;

		if(v >= m * kPow7) // ended up in remainder chunk, it is to be recycled so as to not lose entropy
		{
			v -= m * kPow7;
			c = r; // (v, c) stay uniform  in [0, c)
			return; // no digit is emitted this pass
		}

		std::uint64_t s = v % kPow7; // confirmed uniform in [0,  7^21) -> emit 21 base-7 digits
		v /= kPow7; // uniform in [0, m), carry it over
		c = m;
		for(int i = 0; i < 21; ++i) // emit digits
		{
			buffer.push(static_cast<short>(s % 7) + 1);
			s /= 7;
			++outCount;
		}
	}

	std::mt19937_64 rngSeed; // TODO: consider just using seed_seq to fill entropy
	std::uint64_t v = 0;
	std::uint64_t c = 1;
	std::queue<short> buffer;
	std::uint64_t rollCount = 0;
	std::uint64_t outCount = 0;
};



int main(int argc, char* argv[])
{
	std::random_device rd; // can produce same output every time every run depending on build, TODO: consider OS level entropy
	D5toD7 d7((std::uint64_t{rd()} <<  32) ^ rd()); // pack 2 rolls into a 64 bit uint,  usually rd is 32

	constexpr int ITERATIONS = 100'000'000; // should be enough

	std::array<std::uint64_t, 7> hist{};
	for(size_t n = 0; n < ITERATIONS; ++n)
	{
		++hist[static_cast<size_t>(d7.roll()) - 1];
	}

	double chisq = 0.0; // chi squared stat
	double expected = static_cast<double>(ITERATIONS) / 7.0; // expected bin count

	for(const auto h : hist)
	{
		chisq += ((h - expected) * (h - expected)) / expected; // Σ(O − E)^2 / E
	}

	std::cout << "histogram:\t";
	for(const auto e :  hist)
	{
		std::cout << e << ' ';
	}
	std::cout << "Chi Squared  = "  << chisq << " (df=6, 99% critical value = 16.81)";
	// degrees of freedom = k - 1; for d7 that is 6
	// 99th percentile is about 16.81 (X^2(6) ~~  16.81); a perfectly uniform generator would exceed this value about 1% of the time
	// if less than 16.81 then it should be fine; if more than that then there is very likely bias
	// for a d7 anything more than 1 mil rolls should be perfectly fine; expected deviation at 1 mil is roughly 368
	// formula: √(N·p·(1−p)) 
	// X^2 should land roughly around 6 +- √12 or 6+-3.5
	std::cout << "\nd5 rolls per d7 output: " << d7.rollsPerOutput() << " (entropy floor 1.2091)\n"; // ratio vs theoretical minimum from Yao/Han-Hoshi paper

	return 0;
}