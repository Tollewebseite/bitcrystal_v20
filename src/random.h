#ifndef RANDOM_H
#define RANDOM_H
#include <ctime>       /* time */
namespace myrand
{
	extern int __rand();
	extern void __srand(unsigned int seed);
	extern void _srand(int seed);
	extern int _rand();
	extern int _rand(unsigned int min, unsigned int max);
}
#endif