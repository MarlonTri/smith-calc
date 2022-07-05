#include <Windows.h>
#include <cmath>
#include <mpir.h>
#include <fstream>
#include <iterator>
#include <vector>
#include <iostream>
#include "BS_thread_pool.hpp"
#include <chrono>
#include <utility>
#include <string>
typedef std::chrono::high_resolution_clock::time_point TimeVar;

#define duration(a) std::chrono::duration_cast<std::chrono::nanoseconds>(a).count()
#define timeNow() std::chrono::high_resolution_clock::now()

#define BLOCK_PRINT true

/**
 * Explicitly calculates (a*10**b + 1)**c and sets variable ret to that value.
 *
 * This function is unoptimized for large values.
 */
void gen_bi_power(mpz_t ret, long a, long b, long c) {
	mpz_ui_pow_ui(ret, 10, b);
	mpz_mul_ui(ret, ret, a);
	mpz_add_ui(ret, ret, 1);
	mpz_pow_ui(ret, ret, c);
}

/**
 * Explicitly calculates the repunit R_n = 11...11 (n digits) and sets variable ret to that value.
 */
void gen_repunit(mpz_t ret, long n) {
	mpz_ui_pow_ui(ret, 10, n);
	mpz_sub_ui(ret, ret, 1);
	mpz_divexact_ui(ret, ret, 9);
}

/**
 * Explicitly calculates the coefficient c_{t,k} = binom(c  k) a^k and sets variable ret to that value.
 * c_{t,k} is the k^th coefficient of the power expansion (a*10**b + 1)**t.
 */
void pow_coef(mpz_t ret, long long a, long long k, long long c) {
	mpz_t hold;
	mpz_init(hold);
	mpz_bin_uiui(hold, c, k);

	mpz_ui_pow_ui(ret, a, k);
	mpz_mul(ret, ret, hold);

	mpz_clear(hold);
}

/**
 * Sum digits of an integer.
 *
 * Example: S(1232) = 1 + 2 + 3 + 2 = 8
 *
 * @param num number to be digit summed.
 * @return sum of digits
 */
long long digit_sum(mpz_t num) {
	long long sum = 0;
	char *num_str = mpz_get_str(NULL, 10, num);
	for (char *t = num_str; *t != '\0'; t++) {
		sum += *t - '0';
	}
	free(num_str);
	return sum;
}

/**
 * Calculates k(t) = ceil( (3*t-1)/4 )
 *
 * For a=3, k_t(t) will be largest coefficient of all c_{t,k} coefficients of (a*10**b + 1)**t expansion.
 */
long k_t(long t) {
	// Identity: ceil(x/y) = (x + y - 1) / y
	// Therefor, below is ceil( (3*t-1)/4 )
	return (3 * t + 2) / 4;
}

/**
 * Calculates digit sum of (a*10**b + 1)**c explicitly (GROUND TRUTH)
 *
 * This function is slowest and most memory intensive, but is a ground truth to compare other faster digit sum functions.
 */
long long bi_digit_sum_GT(long a, long b, long c) {
	mpz_t ret;
	mpz_init(ret);
	gen_bi_power(ret, a, b, c);
	long long out = digit_sum(ret);
	mpz_clear(ret);
	return out;
}

/**
 * Calculates digit sum of (a*10**b + 1)**c by summing coefficient c_{t,k} digit sums.
 *
 * This function is fast and memory-efficient but requires each coefficient to be smaller than 10**b for accuracy.
 */
long long bi_digit_sum_COEF(long a, long b, long c) {
	long long sum = 0;

	mpz_t hold;
	mpz_init(hold);

	for (int k = 0; k <= c; k++) {
		pow_coef(hold, a, k, c);
		sum += digit_sum(hold);
	}

	mpz_clear(hold);

	return sum;
}

/**
 * Calculates digit sum of (a*10**b + 1)**c by summing coefficient c_{t,k} digit sums (MULTI THREADED).
 *
 * This function is fastest, as it is a multi-threaded implementation of bi_digit_sum_COEF.
 */
long long bi_digit_sum_COEF_MT(long a, long b, long c) {
	BS::thread_pool pool;
	long long PARALLEL_SIZE = 10000;
	long long sum = 0;

	for (long long block = 0; block <= c; block += PARALLEL_SIZE) {
		if (BLOCK_PRINT) {
			std::cout << "Block: " << block << "\n";
		}
		long long topCap = min(block + PARALLEL_SIZE, c + 1);
		BS::multi_future<long long> mf = pool.parallelize_loop(block, topCap,
			[a, c](const long long ai, const long long bi) {
				mpz_t hold;
				mpz_init(hold);

				long long block_total = 0;
				for (int k = ai; k < bi; ++k) {
					pow_coef(hold, a, k, c);
					block_total += digit_sum(hold);
				}

				mpz_clear(hold);

				return block_total;
			});
		std::vector<long long> totals = mf.get();
		for (const long long t : totals) {
			sum += t;
		}
	}

	return sum;
}

/**
 * Calculates digit sums of each coefficient of (a*10**b + 1)**c.
 *
 * This function is internally identical to bi_digit_sum_COEF but returns the coefficient digit sums as a vector.
 */
std::vector<long long> bi_digit_lst(long a, long b, long c) {
	long long sum = 0;

	std::vector<long long> ret(c + 1, 0);

	mpz_t hold;
	mpz_init(hold);

	for (int k = 0; k <= c; k++) {
		pow_coef(hold, a, k, c);
		ret.at(k) = digit_sum(hold);
	}

	mpz_clear(hold);

	return ret;
}

/**
 * Calculates digit sums of each coefficient of (a*10**b + 1)**c (MULTI THREADED).
 *
 * This function is internally identical to bi_digit_sum_COEF_MT but returns the coefficient digit sums as a vector.
 */
std::vector<long long> bi_digit_lst_MT(long a, long b, long c) {
	BS::thread_pool pool;
	long long PARALLEL_SIZE = 10000;

	std::vector<long long> ret(c + 1, 0);

	for (long long block = 0; block <= c; block += PARALLEL_SIZE) {
		if (BLOCK_PRINT) {
			std::cout << "Block: " << block << "\n";
		}
		long long topCap = min(block + PARALLEL_SIZE, c + 1);
		BS::multi_future<long long> mf = pool.parallelize_loop(block, topCap,
			[a, c, &ret](const long long ai, const long long bi) {
				mpz_t hold;
				mpz_init(hold);

				for (int k = ai; k < bi; ++k) {
					pow_coef(hold, a, k, c);
					ret.at(k) = digit_sum(hold);
				}

				mpz_clear(hold);

				return (long long)1;
			});
		mf.get();
	}

	return ret;
}

/**
 * Times a function call with given arguments
 *
 * @return Duration in seconds
 */
template <typename F, typename... Args>
double funcTime(F func, Args &&...args) {
	TimeVar t1 = timeNow();
	func(std::forward<Args>(args)...);
	return duration(timeNow() - t1);
}

// funcDebug helper function.
template <typename T, typename... Args>
std::string sjoin(T arg) {
	std::string ret = "";
	ret += std::to_string(arg);
	return ret;
}

// funcDebug helper function.
template <typename T, typename... Args>
std::string sjoin(T arg, Args &&...args) {
	std::string ret = "";
	ret = std::to_string(arg) + "," + sjoin(args...);
	return ret;
}

/**
 * Prints debug and performance info about a function call with given arguments.
 *
 * @return Debug information as string
 */
template <typename F, typename... Args>
std::string funcDebug(F func, Args &&...args) {
	std::string ret = "\tInputs:";
	ret += sjoin(args...);
	ret = ret + "\tOutput:";

	TimeVar t1 = timeNow();
	ret = ret + std::to_string(func(std::forward<Args>(args)...));
	double dur = duration(timeNow() - t1);
	ret = ret + "\tTime:";
	ret = ret + std::to_string(dur / 1000000000) + "s";
	return ret;
}

template <typename F, typename... Args>
std::string funcDebugNoOutput(F func, Args &&...args) {
	std::string ret = "\tInputs:";
	ret += sjoin(args...);
	ret = ret + "\t";

	TimeVar t1 = timeNow();
	func(std::forward<Args>(args)...);
	double dur = duration(timeNow() - t1);
	ret = ret + "\tTime:";
	ret = ret + std::to_string(dur / 1000000000) + "s";
	return ret;
}

/**
 * Prints mpz_t integer in scientific notation with truncation and 6 sig-figs.
 */
void mpz_print_sci(mpz_t integer) {
	/* Print integer's value in scientific notation without rounding */
	char *buff = mpz_get_str(NULL, 10, integer);
	printf("%.*s", 1, buff);
	std::cout << ".";
	printf("%.*s", 6, buff + 1);
	std::cout << "...*10^" << strlen(buff) - 1;
}

/**
 * Saves a vector of coefficients to a .csv file.
 */
template <typename T>
void save_coefs_to_csv(std::vector<T> vec, std::string path) {
	std::ofstream output_file(path);
	for (long i = 0; i < vec.size(); i++) {
		output_file << i << "," << vec.at(i);
		if (i != vec.size() - 1) {
			output_file << "\n";
		}
	}
}

/**
 * Loads a vector of coefficients from a .csv file.
 */
std::vector<long long> load_coefs_from_csv(std::string path) {
	std::vector<long long> vec;
	std::ifstream file(path);
	if (file.is_open()) {
		std::string line;
		while (std::getline(file, line)) {
			// using printf() in all tests for consistency
			std::string coef = line.substr(line.find(",") + 1, line.size());
			vec.push_back(std::stoll(coef));
		}
		file.close();
	}
	return vec;
}

/**
 * Binary search to find a value t so that
 * c_{t,k(t)} <= upper_lim <= c_{t+1,k(t+1)}
 */
long long search_coef(long long a, long long start_t, mpz_t upper_lim) {
	long long bottom = 0;
	long long top = start_t;

	mpz_t hold;
	mpz_init(hold);

	while (true) {
		pow_coef(hold, a, k_t(top), top);
		if (0 < mpz_cmp(hold, upper_lim)) {
			break;
		}
		top = top * 2;
	}

	long long new_hit = 123123;
	while (1 < top - bottom) {
		new_hit = (top + bottom) / 2;
		pow_coef(hold, a, k_t(new_hit), new_hit);
		int disc = mpz_cmp(hold, upper_lim);
		if (disc < 0) {
			bottom = new_hit;
		}
		else if (0 < disc) {
			top = new_hit;
		}
		else {
			return new_hit;
		}
	}
	return bottom;

	mpz_clear(hold);
}

void section1() {
	std::cout << "Section 1: Update on Costello 2002\n";
	long long c1 = 665829;
	long long t0 = 81525;

	mpz_t op1, op2;
	mpz_init(op1);
	mpz_init(op2);

	pow_coef(op1, 3, k_t(t0), t0);
	std::cout << "c_{81525,k(81525)} =\t";
	mpz_print_sci(op1);
	std::cout << "\n";

	gen_repunit(op2, 49081);
	mpz_mul_ui(op2, op2, 9);
	std::cout << "R49081 =\t\t";
	mpz_print_sci(op2);
	std::cout << "\n";

	pow_coef(op1, 3, k_t(t0 + 1), t0 + 1);
	std::cout << "c_{81526,k(81526)} =\t";
	mpz_print_sci(op1);
	std::cout << "\n\n";

	mpz_clear(op1);
	mpz_clear(op2);
}

void section2() {
	std::cout << "Section 2: t0=1105923 Bounding\n";
	long long c1 = 665829;
	long long t0 = 1105923;

	std::cout << "Let t0 = " << t0 << "\n";

	mpz_t op1, op2;
	mpz_init(op1);
	mpz_init(op2);

	pow_coef(op1, 3, k_t(t0), t0);
	std::cout << "c_{t0,k(t0)} =\t\t";
	mpz_print_sci(op1);
	std::cout << "\n";

	mpz_ui_pow_ui(op2, 10, c1);
	std::cout << "10^665829 =\t\t";
	mpz_print_sci(op2);
	std::cout << "\n";

	pow_coef(op1, 3, k_t(t0 + 1), t0 + 1);
	std::cout << "c_{t0+1,k(t0 + 1)} =\t";
	mpz_print_sci(op1);
	std::cout << "\n\n";

	mpz_clear(op1);
	mpz_clear(op2);
}

void section3() {
	std::cout << "Section 3: t0=1105923 Digit Sum\n";

	long long c1 = 3;
	long long c2 = 665829;
	long long t0 = 1105923;
	std::string path = "./3-665829-1105923.csv";

	bool RECALCULATE = false;

	std::vector<long long> coefs;
	std::cout << "Let t0 = " << t0 << "\n";
	std::cout << std::boolalpha << "Recalculating Coefficients? = " << RECALCULATE << "\n";
	if (RECALCULATE) {
		coefs = bi_digit_lst_MT(c1, c2, t0);
		save_coefs_to_csv(coefs, path);
	}
	else {
		std::cout << "Loading Coefficients from " << path << "\n";
		coefs = load_coefs_from_csv(path);
	}

	long long sum = 0;
	for (auto coef : coefs) {
		sum += coef;
	}
	std::cout << "S(M^{t0}) = " << sum << "\n";
	std::cout << "Sp(M^{t0}) = " << 4 * t0 << "\n";
	std::cout << "S(M^{t0}) - Sp(M^{t0}) = " << (sum - 4 * t0) % 7 << " mod 7\n";
	std::cout << "(S(M^{t0}) - Sp(M^{t0}))/7 = " << (sum - 4 * t0) / 7 << "\n\n";
}

void section4() {
	std::cout << "Section 4: Afterword\n";
	long long c0 = 665829;
	long long c1 = c0;
	long long t0;

	mpz_t op1, op2;
	mpz_init(op1);
	mpz_init(op2);

	mpz_ui_pow_ui(op2, 10, c1);
	t0 = search_coef(3, 1, op2);
	std::cout << "Let t0 = " << t0 << "\n";
	pow_coef(op1, 3, k_t(t0), t0);
	std::cout << "c_{t0,k(t0)} =\t\t";
	mpz_print_sci(op1);
	std::cout << "\n";
	mpz_ui_pow_ui(op2, 10, c1);
	std::cout << "10^665829 =\t\t";
	mpz_print_sci(op2);
	std::cout << "\n";
	pow_coef(op1, 3, k_t(t0 + 1), t0 + 1);
	std::cout << "c_{t0+1,k(t0 + 1)} =\t";
	mpz_print_sci(op1);
	std::cout << "\n";

	c1 = 2 * c0;
	mpz_ui_pow_ui(op2, 10, c1);
	t0 = search_coef(3, 1, op2);
	std::cout << "Let t0 = " << t0 << "\n";
	pow_coef(op1, 3, k_t(t0), t0);
	std::cout << "c_{t0,k(t0)} =\t\t";
	mpz_print_sci(op1);
	std::cout << "\n";
	std::cout << "10^(2*665829)=\t\t";
	mpz_print_sci(op2);
	std::cout << "\n";
	pow_coef(op1, 3, k_t(t0 + 1), t0 + 1);
	std::cout << "c_{t0+1,k(t0 + 1)} =\t";
	mpz_print_sci(op1);
	std::cout << "\n";

	c1 = 3 * c0;
	mpz_ui_pow_ui(op2, 10, c1);
	t0 = search_coef(3, 1, op2);
	std::cout << "Let t0 = " << t0 << "\n";
	pow_coef(op1, 3, k_t(t0), t0);
	std::cout << "c_{t0,k(t0)} =\t\t";
	mpz_print_sci(op1);
	std::cout << "\n";
	std::cout << "10^(3*665829)=\t\t";
	mpz_print_sci(op2);
	std::cout << "\n";
	pow_coef(op1, 3, k_t(t0 + 1), t0 + 1);
	std::cout << "c_{t0+1,k(t0 + 1)} =\t";
	mpz_print_sci(op1);
	std::cout << "\n\n";

	mpz_clear(op1);
	mpz_clear(op2);
}

int main() {

	section1();
	section2();
	section3();
	// section4();
}
