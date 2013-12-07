#include <cstdio>
#include <cassert>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <random>
#include <intrin.h>

const int ALIGN=16;
const unsigned D=16; // dimension of a vector with uchar elements
const unsigned N=1; // # of dictionary vectors

inline std::string to_binary(unsigned char b)
{
	std::string str("........");
	for(int i=0;i<8;++i)
		if(b&(1<<i))
			str[8-1-i]='#';
	return str;
}

inline void print_vectors(unsigned size,const unsigned char* vecs)
{
	for(unsigned i=0;i<size;++i)
	{
		printf("%2d:  ",i);
		for(unsigned d=0;d<D;++d)
			printf("%3d ",vecs[i*D+d]);
			//printf("%02X ",vecs[i*D+d]);
			//printf("%3s ",to_binary(vecs[i*D+d]).c_str());
		printf("\n");
	}
}

//bit count by the D&C algorithm
inline int popcount(unsigned int x)
{
	x=(x&0x55555555)+((x>> 1)&0x55555555);
	x=(x&0x33333333)+((x>> 2)&0x33333333);
	x=(x&0x0F0F0F0F)+((x>> 4)&0x0F0F0F0F);
	x=(x&0x00FF00FF)+((x>> 8)&0x00FF00FF);
	x=(x&0x0000FFFF)+((x>>16)&0x0000FFFF);
	return x;
}

inline int dist_l2(unsigned char* p,unsigned char* q)
{
	int result=0;
	for(unsigned d=0;d<D;++d)
		result+=(q[d]-p[d])*(q[d]-p[d]);
	return result;
}
inline int dist_l2_sse(unsigned char* p,unsigned char* q)
{
	throw std::domain_error("unimplemented function!");
	//assert(D%16==0);

	//__m128i t=_mm_setzero_si128();
	//for(unsigned d=0;d<D;d+=16)
	//{
	//	__m128i in1=_mm_load_si128((__m128i*)(p+d));
	//	__m128i in2=_mm_load_si128((__m128i*)(p+d));
	//	__m128i diff=_mm_sad_epu8(in1,in2);

	//	__m128i hi=_mm_unpackhi_epi8(diff,_mm_setzero_si128());
	//	__m128i lo=_mm_unpacklo_epi8(diff,_mm_setzero_si128());
	//	__m128i sq1=_mm_mulhi_epu16(hi,lo);
	//	__m128i sq2=_mm_mullo_epi16(hi,lo);
	//	t=_mm_add_epi16(sq1,sq2);
	//}
	//return t.m128i_i16[0]+t.m128i_i16[2];
}

inline int dist_l1(unsigned char* p,unsigned char* q)
{
	int result=0;
	for(unsigned d=0;d<D;++d)
		result+=std::abs(q[d]-p[d]);
	return result;
}
inline int dist_l1_sse(unsigned char* p,unsigned char* q)
{
	assert(D%16==0);

	__m128i t=_mm_setzero_si128();
	for(unsigned d=0;d<D;d+=16)
	{
		t=_mm_add_epi32(t,
			_mm_sad_epu8(
				_mm_load_si128((__m128i*)(p+d)),
				_mm_load_si128((__m128i*)(q+d))
			)
		);
	}
	return t.m128i_i32[0]+t.m128i_i32[2];
}

inline int dist_hamming(unsigned char* p,unsigned char* q)
{
	assert(D%sizeof(unsigned int)==0);
	const unsigned int* pi=reinterpret_cast<unsigned int*>(p);
	const unsigned int* qi=reinterpret_cast<unsigned int*>(q);

	int result=0;
	for(unsigned k=0;k<D/sizeof(unsigned int);++k)
		result+=popcount(pi[k]^qi[k]);
	return result;
}
inline int dist_hamming_sse(unsigned char* p,unsigned char* q)
{
	assert(D%sizeof(unsigned int)==0);
	const unsigned int* pi=reinterpret_cast<unsigned int*>(p);
	const unsigned int* qi=reinterpret_cast<unsigned int*>(q);
	
	int result=0;
	for(unsigned k=0;k<D/sizeof(unsigned int);++k)
		result+=_mm_popcnt_u32(pi[k]^qi[k]);
	return result;
}

inline std::pair<int,int> search(unsigned char* dict,unsigned char* query)
{
	int best_n=-1;
	int best_d=std::numeric_limits<int>::max();
	for(unsigned n=0;n<N;++n)
	{
//		int d=dist_l2    (&dict[n*D],query);
//		int d=dist_l1    (&dict[n*D],query);
//		int d=dist_l1_sse(&dict[n*D],query);
//		int d=dist_hamming    (&dict[n*D],query);
		int d=dist_hamming_sse(&dict[n*D],query);
		if(best_d<d)
			continue;
		best_n=n;
		best_d=d;
	}
	return std::make_pair(best_d,best_n);
}

int main()
{
	printf("Dimension of a vector (D): %d\n",D);
	printf("# of dictionary vectors (N): %d\n",N);
	printf("-----------------------------------------\n");

	std::mt19937 rng;
	std::uniform_real_distribution<unsigned char> dist(0,255);
	
	unsigned char* dict=reinterpret_cast<unsigned char*>(_aligned_malloc(N*D,ALIGN));
	unsigned char* query=reinterpret_cast<unsigned char*>(_aligned_malloc(D,ALIGN));
	
	// generate dictionary and query vectors randomly
	for(unsigned n=0;n<N;++n)
		for(unsigned d=0;d<D;++d)
			dict[n*D+d]=dist(rng);
	for(unsigned d=0;d<D;++d)
		query[d]=dist(rng);

	// print vectors
	printf("[dictionary vectors]\n");
	print_vectors(N,dict);
	printf("[query vectors]\n");
	print_vectors(1,query);

	// (full) nearest neighbor search
	clock_t tick0=clock();
	std::pair<int,int> result=search(dict,query);
	clock_t tick1=clock();
	double time=double(tick1-tick0)/CLOCKS_PER_SEC;
	printf("Nearest neighbor:  %d (distance=%d)\n",result.second,result.first);
	printf("Search time:  %6.2f [s]\n",time);
	
	_aligned_free(dict);
	_aligned_free(query);
	return 0;
}