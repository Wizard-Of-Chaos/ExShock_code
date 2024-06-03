#include "RandomGeneration.h"

_BaedsRandGen::_BaedsRandGen()
{
	randGen = std::mt19937(rd());
}
//Returns a double in the range from min to max.
const double _BaedsRandGen::drange(const double& min, const double& max)
{
	return std::uniform_real_distribution<double>(min, max)(randGen);
}
//Returns a float in the range from min to max.
const float _BaedsRandGen::frange(const float& min, const float& max)
{
	return std::uniform_real_distribution<float>(min, max)(randGen);
}
//Returns an unsigned int in the range from min to max.
const unsigned int _BaedsRandGen::urange(const unsigned int& min, const unsigned int& max)
{
	return std::uniform_int_distribution<unsigned int>(min, max)(randGen);
}
//Returns a signed int in the range from min to max.
const int _BaedsRandGen::srange(const int& min, const int& max)
{
	return std::uniform_int_distribution<int>(min, max)(randGen);
}
//Returns a random unsigned int with a specified max value (default UINT_MAX).
const unsigned int _BaedsRandGen::unum(const unsigned int& max)
{
	return std::uniform_int_distribution<unsigned int>(0U, max)(randGen);
}
//Returns a random int with a specified max value (default INT_MAX). Can generate negatives.
const int _BaedsRandGen::snum(const int& max)
{
	return std::uniform_int_distribution<int>(-max, max)(randGen);
}
//Returns a random float with a specified max value (default FLT_MAX). Can generate negatives.
const float _BaedsRandGen::fnum(const float& max)
{
	return std::uniform_real_distribution<float>(-max, max)(randGen);
}
//Returns a random double with a specified max value (default DBL_MAX). Can generate negatives.
const double _BaedsRandGen::dnum(const double& max)
{
	return std::uniform_real_distribution<double>(-max, max)(randGen);
}
const unsigned int _BaedsRandGen::unumEx(const unsigned int& max)
{
	return std::uniform_int_distribution<unsigned int>(0U, std::clamp(max, 0U, max-1U))(randGen);
}
//Returns a random int with a specified max value (default INT_MAX, the max is excluded). Can generate negatives.
const int _BaedsRandGen::snumEx(const int& max)
{
	return std::uniform_int_distribution<int>(-max+1, max-1)(randGen);
}
//Returns a random float with a specified max value (default FLT_MAX, the max is excluded). Can generate negatives.
const float _BaedsRandGen::fnumEx(const float& max)
{
	return std::uniform_real_distribution<float>(-max+1.f, max-1.f)(randGen);
}
//Returns a random double with a specified max value (default DBL_MAX, the max is excluded). Can generate negatives.
const double _BaedsRandGen::dnumEx(const double& max )
{
	return std::uniform_real_distribution<double>(-max+1.0, max-1.0)(randGen);
}
//Flips a coin, returning true or false.
const bool _BaedsRandGen::coinflip()
{
	return !!urange(0U, 1U);
}
//Returns a random unsigned int in the range 1-20. Critical hits can be dealt with at your discretion.
const unsigned int _BaedsRandGen::d20()
{
	return urange(1U, 20U);
}
//Returns a random unsigned int in the range 1-100.
const unsigned int _BaedsRandGen::d100()
{
	return urange(1U, 100U);
}
//Returns a random unsigned int in the range 1-6.
const unsigned int _BaedsRandGen::d6()
{
	return urange(1U, 6U);
}
//Returns a random unsigned int in the range 1-4.
const unsigned int _BaedsRandGen::d4()
{
	return urange(1U, 4U);
}
//Returns a random unsigned int in the range 1-8.
const unsigned int _BaedsRandGen::d8()
{
	return urange(1U, 8U);
}
//Returns a random unsigned int in the range 1-10.
const unsigned int _BaedsRandGen::d10()
{
	return urange(1U, 10U);
}
//Returns a random unsigned int in the range 1-12.
const unsigned int _BaedsRandGen::d12()
{
	return urange(1U, 12U);
}
//Given a percentage chance (0-100) returns whether or not that chance succeeded.
//Example: 25% chance for thing to happen. percentChance(25) returns false, meaning it did not happen.
const bool _BaedsRandGen::percentChance(const unsigned int& percentage)
{
	return (urange(0U, 100U) <= percentage);
}
//Given a percentage chance (0-100) returns whether or not that chance succeeded.
//Example: 25% chance for thing to happen. percentChance(25) returns false, meaning it did not happen.
const bool _BaedsRandGen::percentChance(const float& percentage)
{
	return (urange(0.f, 100.f) <= percentage);
}