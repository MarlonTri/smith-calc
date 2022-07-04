// smithHelper2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
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

//(a*10**b + 1)**c
void gen_bi_power(mpz_t ret, long a, long b, long c) {
	mpz_ui_pow_ui(ret, 10, b);
	mpz_mul_ui(ret, ret, a);
	mpz_add_ui(ret, ret, 1);
	mpz_pow_ui(ret, ret, c);
}

//Calculates R_n = 11...11 (n digits)
void gen_repunit(mpz_t ret, long n) {
	mpz_ui_pow_ui(ret, 10, n);
	mpz_sub_ui(ret, ret, 1);
	mpz_divexact_ui(ret, ret, 9);
}

//Calculates c^t_k = (c  k) a^k
void pow_coef(mpz_t ret, long long a, long long k, long long c) {
	mpz_t hold;
	mpz_init(hold);
	mpz_bin_uiui(hold, c, k);

	mpz_ui_pow_ui(ret, a, k);
	mpz_mul(ret, ret, hold);

	mpz_clear(hold);
}

long long digit_sum(mpz_t num) {
	long long sum = 0;
	char* num_str = mpz_get_str(NULL, 10, num);
	for (char* t = num_str; *t != '\0'; t++) {
		sum += *t - '0';
	}
	free(num_str);
	return sum;
}

long k_t(long t) {
	//ceil(x/y) = (x + y - 1) / y
	
	//ceil( (3*t-1)/4 )
	return (3 * t + 2) / 4;
}

long long bi_digit_sum_GT(long a, long b, long c) {
	mpz_t ret;
	mpz_init(ret);
	gen_bi_power(ret, a, b, c);
	long long out = digit_sum(ret);
	mpz_clear(ret);
	return out;
}



long long bi_digit_sum_COEF(long a, long b, long c) {
	long long sum = 0;

	mpz_t hold;
	mpz_init(hold);

	for (int k = 0; k <= c;k++) {
		pow_coef(hold, a, k, c);
		sum += digit_sum(hold);
	}

	mpz_clear(hold);

	return sum;
}

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

std::vector<long long> bi_digit_lst(long a, long b, long c) {
	long long sum = 0;

	std::vector<long long> ret(c + 1, 0);

	mpz_t hold;
	mpz_init(hold);

	for (int k = 0; k <= c;k++) {
		pow_coef(hold, a, k, c);
		ret.at(k) =  digit_sum(hold);
	}

	mpz_clear(hold);

	return ret;
}

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

				return (long long) 1;
			});
		mf.get();
	}

	return ret;
}

template<typename F, typename... Args>
double funcTime(F func, Args&&... args) {
	TimeVar t1 = timeNow();
	func(std::forward<Args>(args)...);
	return duration(timeNow() - t1);
}

template<typename T, typename... Args>
std::string sjoin(T arg) {
	std::string ret = "";
	ret += std::to_string(arg);
	return ret;
}

template<typename T, typename... Args>
std::string sjoin(T arg, Args&&... args) {
	std::string ret = "";
	ret = std::to_string(arg) + "," + sjoin(args...);
	return ret;
}

template<typename F, typename... Args>
std::string funcDebug(F func, Args&&... args) {
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

template<typename F, typename... Args>
std::string funcDebugNoOutput(F func, Args&&... args) {
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

void mpz_print_sci(mpz_t integer) {
	/* Print integer's value in scientific notation without rounding */
	char* buff = mpz_get_str(NULL, 10, integer);
	printf("%.*s", 1, buff);
	std::cout << ".";
	printf("%.*s", 6, buff + 1);
	std::cout << "...*10^" << strlen(buff)-1;
}

template<typename T>
void save_coefs_to_csv(std::vector<T> vec, std::string path) {
	std::ofstream output_file(path);
	for (long i = 0; i < vec.size(); i++) {
		output_file << i << "," << vec.at(i);
		if (i != vec.size() - 1) { 
			output_file << "\n"; 
		}
	}
}


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

void section1() {
	std::cout << "Section 1:\n";
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
	std::cout << "Section 2:\n";	
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
	std::cout << "Section 3:\n";

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
	std::cout << "Sp(M^{t0}) = " << 4* t0 << "\n";
	std::cout << "S(M^{t0}) - Sp(M^{t0}) = " << (sum - 4 * t0) % 7 << " mod 7\n";
	std::cout << "(S(M^{t0}) - Sp(M^{t0}))/7 = " << (sum - 4 * t0) / 7 << "\n\n";
}

int main() {

	section1();
	section2();
	section3();

	//long long c_easy = 81519;
	//long long c_hard = 1105923;

	//std::cout << funcDebugNoOutput(bi_digit_lst_MT, c1, c2, 10000) << "\n";
	//std::cout << funcDebugNoOutput(bi_digit_lst_MT, c1, c2, 20000) << "\n";
	//std::cout << funcDebugNoOutput(bi_digit_lst_MT, c1, c2, 40000) << "\n";

	
	
	/*
	c1,c2 = 3,665829
    c_easy = 81519
    c_hard = 1105923

    #c2=10000
    #c_easy=8000

    print('Running benchmarks')
    #test(     bi_digit_sum,  (3,800,100), 'bi_digit_sum       (Python implementation)')
    #test(fast_bi_digit_sum,  (3,800,100), 'fast_bi_digit_sum  (CPython C++ extension)')
    #test(fast_bi_digit_sum2, (3,800,100), 'fast_bi_digit_sum2 (CPython C++ extension)')
    #test(fast_bi_digit_sum3, (3,800,100), 'fast_bi_digit_sum3 (CPython C++ extension)')
    test(fast_bi_digit_sum4, (3,800,100), 'fast_bi_digit_sum4 (CPython C++ extension)')
    test(fast_bi_digit_sum5, (3,800,100), 'fast_bi_digit_sum5 (CPython C++ extension)')
    test(fast_bi_digit_sum6, (3,800,100), 'fast_bi_digit_sum6 (CPython C++ extension)')    

    test(fast_bi_digit_sum5, (c1,c2,c_easy), 'fast_bi_digit_sum5 (CPython C++ extension) EASY')
    test(fast_bi_digit_sum6, (c1,c2,c_easy), 'fast_bi_digit_sum6 (CPython C++ extension) EASY')

    test(fast_bi_digit_sum5, (c1,c2,c_hard), 'fast_bi_digit_sum5 (CPython C++ extension) HARD')
	*/
}
