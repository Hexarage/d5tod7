#include <iostream>

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


int main(int argc, char* argv[])
{
	std::cout << "Hello there\n";

	return 0;
}