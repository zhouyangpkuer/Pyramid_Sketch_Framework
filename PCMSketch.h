#ifndef _PCMSKETCH_H
#define _PCMSKETCH_H

#include <algorithm>
#include <cstring>
#include <string.h>
#include "params.h"
#include "BOBHash.h"
#include <iostream>
#include <math.h>


using namespace std;

typedef unsigned long long int uint64;

class PCMSketch
{
private:
	int d;
	uint64 *counter[60];

	int word_index_size, counter_index_size;
	int word_num, counter_num;
	//word_num is the number of words in the first level.

	BOBHash * bobhash[MAX_HASH_NUM];


public:
	PCMSketch(int _word_num, int _d, int word_size);
	void Insert(const char * str);
	int Query(const char *str);
	void Delete(const char *str);


	//carry from the lower layer to the higher layer, maybe we will allocate the new memory;
	void carry(int index);
	int get_value(int index);
	void down_carry(int index);

	~PCMSketch();	
};

//Just for the consistency of the interface;
//For PCMSketch.h, the word_size must be 64;
PCMSketch::PCMSketch(int _word_num, int _d, int word_size)
{
	d = _d;
	word_num = _word_num;
	//for calculating the four hash value constrained in one certain word;
	word_index_size = 18;

	counter_index_size = (int)(log(word_size) / log(2)) - 2;//4-8->16-256 counters in one word;
	counter_num = (_word_num << counter_index_size);

	
	for(int i = 0; i < 15; i++)
	{
		counter[i] = new uint64[word_num >> i];
		memset(counter[i], 0, sizeof(uint64) * (word_num >> i));
	}

	for(int i = 0; i < d; i++)
		bobhash[i] = new BOBHash(i + 1000);
}

void PCMSketch::Insert(const char *str)
{

	int min_value = 1 << 30;

	int value[MAX_HASH_NUM];
	int index[MAX_HASH_NUM];
	int counter_offset[MAX_HASH_NUM];
	
	uint64 hash_value = (bobhash[0]->run(str, strlen(str)));
	int my_word_index = (hash_value & ((1 << word_index_size) - 1)) % word_num;
	hash_value >>= word_index_size;

	int flag = 0xFFFF;

	for(int i = 0; i < d; i++)
	{
		counter_offset[i] = (hash_value & 0xFFF) % (1 << counter_index_size);
		index[i] = ((my_word_index << counter_index_size) + counter_offset[i]) % counter_num;
		hash_value >>= counter_index_size;
	
		value[i] = (counter[0][my_word_index] >> (counter_offset[i] << 2)) & 0xF;

		if(((flag >> counter_offset[i]) & 1) == 0)
			continue;

		flag &= (~(1 << counter_offset[i]));

		if (value[i] == 15)
		{
			counter[0][my_word_index] &= (~((uint64)0xF << (counter_offset[i] << 2)));
			carry(index[i]);
		}
		else
		{
			counter[0][my_word_index] += ((uint64)0x1 << (counter_offset[i] << 2));
		}
	}
	return;
}

int PCMSketch::Query(const char *str)
{
	int min_value = 1 << 30;

	int value[MAX_HASH_NUM];
	int index[MAX_HASH_NUM];
	int counter_offset[MAX_HASH_NUM];
	
	uint64 hash_value = (bobhash[0]->run(str, strlen(str)));
	int my_word_index = (hash_value & ((1 << word_index_size) - 1)) % word_num;
	hash_value >>= word_index_size;

	for(int i = 0; i < d; i++)
	{
		counter_offset[i] = (hash_value & 0xFFF) % (1 << counter_index_size);
		index[i] = ((my_word_index << counter_index_size) + counter_offset[i]) % counter_num;
		hash_value >>= counter_index_size;

		value[i] = (counter[0][my_word_index] >> (counter_offset[i] << 2)) & 0xF;
		value[i] += get_value(index[i]);
		min_value = value[i] < min_value ? value[i] : min_value;
	}


	return min_value;

}
void PCMSketch::Delete(const char *str)
{

	int min_value = 1 << 30;

	int value[MAX_HASH_NUM];
	int index[MAX_HASH_NUM];
	int counter_offset[MAX_HASH_NUM];
	
	uint64 hash_value = (bobhash[0]->run(str, strlen(str)));
	int my_word_index = (hash_value & ((1 << word_index_size) - 1)) % word_num;
	hash_value >>= word_index_size;

	int flag = 0xFFFF;


	for(int i = 0; i < d; i++)
	{
		counter_offset[i] = (hash_value & 0xFFF) % (1 << counter_index_size);
		index[i] = ((my_word_index << counter_index_size) + counter_offset[i]) % counter_num;
		hash_value >>= counter_index_size;
	
		value[i] = (counter[0][my_word_index] >> (counter_offset[i] << 2)) & 0xF;
		// min_value = value[i] < min_value ? value[i] : min_value;
		if(((flag >> counter_offset[i]) & 1) == 0)
			continue;

		flag &= (~(1 << counter_offset[i]));

		if (value[i] == 0)
		{
			counter[0][my_word_index] |= ((uint64)0xF << (counter_offset[i] << 2));
			down_carry(index[i]);
		}
		else
		{
			counter[0][my_word_index] -= ((uint64)0x1 << (counter_offset[i] << 2));
		}
	}
	return;

}

void PCMSketch::down_carry(int index)
{
	int left_or_right, up_left_or_right;	
	
	int value, up_value;
	int word_index = index >> 4, up_word_index;
	int offset = index & 0xF;
	int up_offset = offset;

	for(int i = 1; i < 15; i++)
	{

		left_or_right = word_index & 1;
		word_index >>= 1;

		up_word_index = (word_index >> 1);
		up_left_or_right = up_word_index & 1;

		value = (counter[i][word_index] >> (offset << 2)) & 0xF;

		if((value & 3) >= 2)
		{
			counter[i][word_index] -= ((uint64)0x1 << (offset << 2));
			return;
		}
		else if((value & 3) == 1)
		{
			up_value = (counter[i + 1][up_word_index] >> (up_offset << 2)) & 0xF;

			//change this layer's flag bit;
			if(((up_value >> (2 + up_left_or_right)) & 1) == 0)
			{
				counter[i][word_index] &= (~((uint64)0x1 << (2 + left_or_right + (offset << 2))));
			}
			
			counter[i][word_index] -= ((uint64)0x1 << (offset << 2));
			return;
		}
		else
		{
			counter[i][word_index] |= ((uint64)0x3 << (offset << 2));
		}
	}
}

void PCMSketch::carry(int index)
{
	int left_or_right;	
	
	int value;
	int word_index = index >> 4;
	int offset = index & 0xF;

	for(int i = 1; i < 15; i++)
	{

		left_or_right = word_index & 1;
		word_index >>= 1;

		counter[i][word_index] |= ((uint64)0x1 << (2 + left_or_right + (offset << 2)));
		value = (counter[i][word_index] >> (offset << 2)) & 0xF;

		if((value & 3) != 3)
		{
			counter[i][word_index] += ((uint64)0x1 << (offset << 2));
			return;
		}
		counter[i][word_index] &= (~((uint64)0x3 << (offset << 2)));
	}
}

int PCMSketch::get_value(int index)
{
	int left_or_right;	
	int anti_left_or_right;
	
	int value;
	int word_index = index >> 4;
	int offset = index & 0xF;


	int high_value = 0;

	for(int i = 1; i < 15; i++)
	{
		
		left_or_right = word_index & 1;
		anti_left_or_right = (left_or_right ^ 1);

		word_index >>= 1;

		value = (counter[i][word_index] >> (offset << 2)) & 0xF;

		if(((value >> (2 + left_or_right)) & 1) == 0)
			return high_value;

		high_value += ((value & 3) - ((value >> (2 + anti_left_or_right)) & 1)) * (1 << (2 + 2 * i));
	}
}

#endif //_PCMSKETCH_H