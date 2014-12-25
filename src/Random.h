#ifndef _RANDOM_H
#define _RANDOM_H
#include "boost/shared_ptr.hpp"
#include "boost/random.hpp"

class Random{
public:
   typedef boost::shared_ptr< Random > randomPtr;
   typedef boost::mt19937 randomGeneratorType;

   static randomPtr Get(){
      static randomPtr randomGen( new RandomGenerator() );
      return randomGen;
   }

   void SetSeed(int seed){
      randomGenerator.seed( seed );
   }

   int Random( int lowerLimit, int upperLimit ){
   boost::uniform_int<> distribution( lowerLimit, upperLimit );
   boost::variate_generator< randomGeneratorType&, boost::uniform_int<> >
   LimitedInt( randomGenerator , distribution );
   return LimitedInt();
   }

private:
   // prevent creation of more than one object of the LogManager class
   // use the Get() method to get a shared_ptr to the object
  Random():
    randomGenerator() //initialize randomGenerator with default constructor
  {}

  RandomGenerator( const RandomGenerator& orig ){};

  randomGeneratorType randomGenerator;
};
#endif