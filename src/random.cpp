#include "random.h"
namespace myrand
{
	static unsigned long int nextBitcrystalInt = 1;
	int __rand() // RAND_MAX assumed to be 32767
	{
		nextBitcrystalInt = nextBitcrystalInt * 1103515245 + 12345;
		return (unsigned int)(nextBitcrystalInt/65536) % 32768;
	}

	void __srand(unsigned int seed)
	{
		nextBitcrystalInt = seed;
	}
	
	void _srand(int seed)
	{
		if(seed==0)
		{
			__srand(1420297560);
		} else if (seed < 0) {
			__srand(time(NULL));
		} else {
			__srand(seed);
		}
	}

	int _rand()
	{
		return __rand();
	}

	int _rand(unsigned int min, unsigned int max)
	{
		if(min>=max)
			return -1;
		int number=0;
		while(true)
		{
			number=__rand()%max;
			if(number>min)
				break;
		}
		return number;
	}
}