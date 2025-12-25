[N.B. initial versionis targetting windows althouggh eventually aim is to be completely cross platform compatible with no issues.]

i want to create the fastest C++ RNG possible for all possible machines that try to load the RNG. this mean the RNG has to adapt to everything from scalar right up to openCL high level gpu capability and still perform at its best.
the further complication is that i want to benchmark 2 versions of xorshiro128++ and 2 versions of WyRand code all together so can see which of the 4 code options performs the best in all different implementations of the various SIMD instructions sets to work out which is the all round best option universally.
so there ya have it - all i gotta do is get openCL and scalar operation working, sort out any odd bugs in the code and i can get on with benchmarking and then make my choice of  code implementation!
EASY! XD

i.e. i'm out to beat C++'s own engineers at their own gam and improve uupon the std library implementation supplied with a given default vesion of C++!!!


the absolute key factors in priority order are: 
1. speed.
2. randomness quality.
3. memory usage.
i know which is best for  randomness quality - that 's effectively out of our hands being dictated by the particular algorythm chosen, and in the case of xoroshiro and wyrand, both are very good PRNGs  altho not perfect, but by far good enough since xoroshiro128++ is already used by the C++ std default library and wyrand is in fact even better at randomness quality. as for memory usage? frankly unless there's some extremely unexpected high usage issues then memory just isn't going to raise it's head as an issue we're likely to have to worry about judging by how the tests have run  so far - so it's a straight up shoot out to see which can do raw speed the best. supposedly we can expect xoroshiro128+ the std 'slow' old C++ choice to be better than 128++ under basic scalar conditions and implementation and wyrand to be somewhere between 128+ and 128++ but the reality of which is actually faster depends on so many different factors that in fact it's basically down to us to shine with our coding skills and see how much performance we can squeeze out of the implementations and hence see quite how far we can push the thing as a whole. if i can outperform the std C++ current latest up-to-date implementations solidly i'll be thrilled because then i'll have something to submit to the wider community and say "hey! look! i've built you a faster bit of kit to work with - enjoy better performance forevermore :O"


SPEED? RNADOMNESS QUAL? OR MEM USAGE? OR ALL?

welp fren, neither wyrand or xoroshiro are cryptographically secure as RNGs so that's out the window straight away as an application. as far as scientific and gaming go, both are big interests in the RNG market, especially when considering C++ is the world we're living in, but it's worth noting for scientific purposes the quality of randomness has to be pretty damn high within relatively large bounds, whereas that's of somewhat lesser importance for gaming. otoh as far as gaming goes, sheer speed of generation on both sm0ll scales and large scales of volum e of numbers to be generated has to be extreme so basically if i can successfully make the RNG perform at least as well as the current C++ RNG [i.e.  xoroshiro128+ minimum] then it'll be good enough for scientific uses - the current C++ std RNG already having proved acceptable for most uses and so ours should all be fine, leaving it purely down to whether we can outperform the current C++ std library on speed terms - hence i'm prioritizing speed, straight up whatever can knock the std xoroshiro128+ off the charts for speed performance whilst retaining the degree of randomness quality and memory usage present then is gonna be an easy winner. i think we can do it no trouble - we've made immense progress already!



Claude Haiku's Summary:

Project Overview: Universal High-Performance RNG
Primary Goals:

Create the fastest possible RNG across multiple hardware architectures
Support scalar to OpenCL/GPU implementations
Benchmark multiple RNG algorithms (xoroshiro128++ and WyRand)
Outperform the C++ standard library implementation

Key Performance Priorities (in order):

Speed
Randomness Quality
Memory Usage

Implementation Strategy
Current Strengths:

Comprehensive SIMD support (SSE2, AVX, AVX2, AVX-512)
Multiple algorithm implementations (xoroshiro128++, WyRand)
Runtime detection of hardware capabilities
Modular design allowing easy algorithm compariso

