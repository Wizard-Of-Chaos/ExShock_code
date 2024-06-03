#pragma once
#ifndef RANDOMGENERATION_BAEDS_H
#define RANDOMGENERATION_BAEDS_H
#include <random>

//Utility class for generation of random numbers. Uses the std::random library.
//Seed is obtained from std::random_device and a mersenne twister engine is used, seeded with that number at initialization.
class _BaedsRandGen
{
public:
	_BaedsRandGen();
	//Returns a double in the range from min to max.
	const double drange(const double& min, const double& max);
	//Returns a float in the range from min to max.
	const float frange(const float& min, const float& max);
	//Returns an unsigned int in the range from min to max.
	const unsigned int urange(const unsigned int& min, const unsigned int& max);
	//Returns a signed int in the range from min to max.
	const int srange(const int& min, const int& max);
	//Returns a random unsigned int with a specified max value (default UINT_MAX).
	const unsigned int unum(const unsigned int& max = UINT_MAX);
	//Returns a random int with a specified max value (default INT_MAX). Can generate negatives.
	const int snum(const int& max = INT_MAX);
	//Returns a random float with a specified max value (default FLT_MAX). Can generate negatives.
	const float fnum(const float& max = FLT_MAX);
	//Returns a random double with a specified max value (default DBL_MAX). Can generate negatives.
	const double dnum(const double& max = DBL_MAX);
	//Returns a random unsigned int with a specified max value (default UINT_MAX, the max is excluded).
	const unsigned int unumEx(const unsigned int& max = UINT_MAX);
	//Returns a random int with a specified max value (default INT_MAX, the max/min is excluded). Can generate negatives.
	const int snumEx(const int& max = INT_MAX);
	//Returns a random float with a specified max value (default FLT_MAX, the max/min is excluded). Can generate negatives.
	const float fnumEx(const float& max = FLT_MAX);
	//Returns a random double with a specified max value (default DBL_MAX, the max/min is excluded). Can generate negatives.
	const double dnumEx(const double& max = DBL_MAX);
	//Flips a coin, returning true or false.
	const bool coinflip();
	//Returns a random unsigned int in the range 1-20. Critical hits can be dealt with at your discretion.
	const unsigned int d20();
	//Returns a random unsigned int in the range 1-100.
	const unsigned int d100();
	//Returns a random unsigned int in the range 1-6.
	const unsigned int d6();
	//Returns a random unsigned int in the range 1-4.
	const unsigned int d4();
	//Returns a random unsigned int in the range 1-8.
	const unsigned int d8();
	//Returns a random unsigned int in the range 1-10.
	const unsigned int d10();
	//Returns a random unsigned int in the range 1-12.
	const unsigned int d12();
	//Given a percentage chance (0-100) returns whether or not that chance succeeded.
	//Example: 25% chance for thing to happen. percentChance(25) returns false, meaning it did not happen.
	const bool percentChance(const float& percentage);
	//Given a percentage chance (0-100) returns whether or not that chance succeeded.
	//Example: 25% chance for thing to happen. percentChance(25) returns false, meaning it did not happen.
	const bool percentChance(const unsigned int& percentage);
private:
	std::random_device rd;
	std::mt19937 randGen;
};

#endif 