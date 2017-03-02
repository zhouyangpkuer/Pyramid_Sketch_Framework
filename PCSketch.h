#ifndef _PCSKETCH_H
#define _PCSKETCH_H

#include <algorithm>
#include <cstring>
#include <string.h>
#include "params.h"
#include "BOBHash.h"
#include <iostream>
#include <math.h>


using namespace std;

typedef unsigned long long int uint64;

class PCSketch
{
private:
	int d;
	uint64 *counter[60];
	bool *flag[60];


	int word_index_size, counter_index_size;
	int word_num, counter_num;
	//word_num is the number of words in the first level.

	BOBHash * bobhash[MAX_HASH_NUM];


public:

	PCSketch(int _word_num, int _d, int word_size);
	void Insert(const char * str);
	int Query(const char *str);
	void Delete(const char *str);


	//carry from the lower layer to the higher layer, maybe we will allocate the new memory;
	void carry(int index);
	int get_value(int index);
	void down_carry(int index);

	~PCSketch();	
};

//Just for the consistency of the interface;
//For PCSketch.h, the word_size must be 64;
PCSketch::PCSketch(int _word_num, int _d, int word_size)
{

	d = _d;
	word_num = _word_num * 4.0 / 5.0;
	//for calculating the four hash value constrained in one certain word;
	word_index_size = 18;

	counter_index_size = (int)(log(word_size) / log(2)) - 2;//4-8->16-256 counters in one word;
	counter_num = (_word_num << counter_index_size);

	
	for(int i = 0; i < 15; i++)
	{
		counter[i] = new uint64[word_num >> i];
		flag[i] = new bool[counter_num >> i];

		memset(counter[i], 0, sizeof(uint64) * (word_num >> i));
		memset(flag[i], false, sizeof(bool) * (counter_num >> i));
	}

	for(int i = 0; i < d * 2; i++)
		bobhash[i] = new BOBHash(i + 1000);
}

void PCSketch::Insert(const char *str)
{

	int min_value = 1 << 30;
	int value[MAX_HASH_NUM], index[MAX_HASH_NUM];
	
	int flag_t = 0xFFFF;


	int word_index, offset, hash_value;
	
	hash_value = (bobhash[0]->run(str, strlen(str)));
	word_index = (hash_value & ((1 << word_index_size) - 1)) % word_num;
	hash_value >>= word_index_size;

	for(int i = 0; i < 2; i++)
	{
		offset = (hash_value & 0xFFF) % (1 << counter_index_size);
		index[i] = (word_index << counter_index_size) + offset;

		hash_value >>= counter_index_size;
	}

	hash_value = (bobhash[1]->run(str, strlen(str)));
	word_index = (hash_value & ((1 << word_index_size) - 1)) % word_num;
	hash_value >>= word_index_size;

	for(int i = 2; i < 4; i++)
	{
		offset = (hash_value & 0xFFF) % (1 << counter_index_size);
		index[i] = (word_index << counter_index_size) + offset;

		hash_value >>= counter_index_size;
	}

	for(int i = 0; i < d; i++)
	{	
		word_index = (index[i] >> 4);
		offset = (index[i] & 0xF);


		if(((flag_t >> offset) & 1) == 0)
			continue;

		flag_t &= (~(1 << offset));



		value[i] = (counter[0][word_index] >> (offset << 2)) & 0xF;
		int	g = (bobhash[i + d]->run(str, strlen(str))) % 2;

		//++
		if(g == 0)
		{
			//posi
			if(flag[0][index[i]] == false)
			{
				if(value[i] == 15)
				{
					counter[0][word_index] &= (~((uint64)0xF << (offset << 2)));
					carry(index[i]);
				}
				else
				{
					counter[0][word_index] += ((uint64)0x1 << (offset << 2));
				}
			}
			//nega
			else
			{
				if(value[i] == 1)
				{
					counter[0][word_index] &= (~((uint64)0xF << (offset << 2)));
					flag[0][index[i]] = false;
				}
				else
				{
					counter[0][word_index] -= ((uint64)0x1 << (offset << 2));
				}
			}
		}
		//--
		else
		{
			//posi
			if(flag[0][index[i]] == false)
			{
				if(value[i] == 0)
				{
					counter[0][word_index] += ((uint64)0x1 << (offset << 2));
					flag[0][index[i]] = true;
				}
				else
				{
					counter[0][word_index] -= ((uint64)0x1 << (offset << 2));
				}
			}
			else
			{
				if(value[i] == 15)
				{
					counter[0][word_index] &= (~((uint64)0xF << (offset << 2)));

					down_carry(index[i]);
				}
				else
				{
					counter[0][word_index] += ((uint64)0x1 << (offset << 2));

				}
			}
		}
	}

	return;
}

int PCSketch::Query(const char *str)
{

	int temp, temp2;
	int res[MAX_HASH_NUM], value[MAX_HASH_NUM], index[MAX_HASH_NUM];
	int flag_t = 0xFFFF;
	int hash_value;

	int word_index, offset;
	hash_value = (bobhash[0]->run(str, strlen(str)));
	word_index = (hash_value & ((1 << word_index_size) - 1)) % word_num;
	hash_value >>= word_index_size;

	for(int i = 0; i < 2; i++)
	{
		offset = (hash_value & 0xFFF) % (1 << counter_index_size);
		index[i] = (word_index << counter_index_size) + offset;

		hash_value >>= counter_index_size;
	}

	hash_value = (bobhash[1]->run(str, strlen(str)));
	word_index = (hash_value & ((1 << word_index_size) - 1)) % word_num;
	hash_value >>= word_index_size;

	for(int i = 2; i < 4; i++)
	{
		offset = (hash_value & 0xFFF) % (1 << counter_index_size);
		index[i] = (word_index << counter_index_size) + offset;

		hash_value >>= counter_index_size;
	}
	
	

	for(int i = 0; i < d; i++)
	{	
		word_index = (index[i] >> 4);
		offset = (index[i] & 0xF);


		value[i] = (counter[0][word_index] >> (offset << 2)) & 0xF;

		int	g = (bobhash[i + d]->run(str, strlen(str))) % 2;
		
		if(flag[0][index[i]] == false)
			temp = value[i] + get_value(index[i]);
		else
			temp = 0 - value[i] + get_value(index[i]);
		
		res[i] = (g == 0 ? temp : -temp);
	}

	sort(res, res + d);
	int r;
	if(d % 2 == 0)
	{
		r = (res[d / 2] + res[d / 2 - 1]) / 2;
	}
	else
	{
		r = res[d / 2];
	}
	return r;


}
void PCSketch::Delete(const char *str)
{

	int min_value = 1 << 30;
	int value[MAX_HASH_NUM], index[MAX_HASH_NUM];
	
	int flag_t = 0xFFFF;


	int word_index, offset, hash_value;
	
	hash_value = (bobhash[0]->run(str, strlen(str)));
	word_index = (hash_value & ((1 << word_index_size) - 1)) % word_num;
	hash_value >>= word_index_size;

	for(int i = 0; i < 2; i++)
	{
		offset = (hash_value & 0xFFF) % (1 << counter_index_size);
		index[i] = (word_index << counter_index_size) + offset;

		hash_value >>= counter_index_size;
	}

	hash_value = (bobhash[1]->run(str, strlen(str)));
	word_index = (hash_value & ((1 << word_index_size) - 1)) % word_num;
	hash_value >>= word_index_size;

	for(int i = 2; i < 4; i++)
	{
		offset = (hash_value & 0xFFF) % (1 << counter_index_size);
		index[i] = (word_index << counter_index_size) + offset;

		hash_value >>= counter_index_size;
	}


	for(int i = 0; i < d; i++)
	{	
		word_index = (index[i] >> 4);
		offset = (index[i] & 0xF);


		if(((flag_t >> offset) & 1) == 0)
			continue;

		flag_t &= (~(1 << offset));



		value[i] = (counter[0][word_index] >> (offset << 2)) & 0xF;
		int	g = (bobhash[i + d]->run(str, strlen(str))) % 2;

		//++
		if(g == 1)
		{
			//posi
			if(flag[0][index[i]] == false)
			{
				if(value[i] == 15)
				{
					counter[0][word_index] &= (~((uint64)0xF << (offset << 2)));
					carry(index[i]);
				}
				else
				{
					counter[0][word_index] += ((uint64)0x1 << (offset << 2));
				}
			}
			//nega
			else
			{
				if(value[i] == 1)
				{
					counter[0][word_index] &= (~((uint64)0xF << (offset << 2)));
					flag[0][index[i]] = false;
				}
				else
				{
					counter[0][word_index] -= ((uint64)0x1 << (offset << 2));
				}
			}
		}
		//--
		else
		{
			//posi
			if(flag[0][index[i]] == false)
			{
				if(value[i] == 0)
				{
					counter[0][word_index] += ((uint64)0x1 << (offset << 2));

					flag[0][index[i]] = true;
				}
				else
				{
					counter[0][word_index] -= ((uint64)0x1 << (offset << 2));
				}
			}
			else
			{
				if(value[i] == 15)
				{
					counter[0][word_index] &= (~((uint64)0xF << (offset << 2)));

					down_carry(index[i]);
				}
				else
				{
					counter[0][word_index] += ((uint64)0x1 << (offset << 2));

				}
			}
		}
	}
	return;
}

void PCSketch::down_carry(int index)
{
	int left_or_right;	
	
	int value;
	int word_index = index >> 4;
	int offset = index & 0xF;
	int counter_index;

	for(int i = 1; i < 15; i++)
	{

		left_or_right = word_index & 1;
		word_index >>= 1;

		counter_index = (word_index << 4) + offset;

		counter[i][word_index] |= ((uint64)0x1 << (2 + left_or_right + (offset << 2)));
		value = (counter[i][word_index] >> (offset << 2)) & 0x3;

		//posi
		if(flag[i][counter_index] == false)
		{
			if(value == 0)
			{
				counter[i][word_index] += ((uint64)0x1 << (offset << 2));
				flag[i][counter_index] = true;
				return;
			}
			else
			{
				counter[i][word_index] -= ((uint64)0x1 << (offset << 2));
				return;
			}
		}
		//nega
		else
		{
			if(value == 3)
			{
				counter[i][word_index] &= (~((uint64)0x3 << (offset << 2)));
			}
			else
			{
				counter[i][word_index] += ((uint64)0x1 << (offset << 2));
				return;
			}	
		}

	}
}

void PCSketch::carry(int index)
{
	int left_or_right;	
	
	int value;
	int word_index = index >> 4;
	int offset = index & 0xF;
	int counter_index;

	for(int i = 1; i < 15; i++)
	{

		left_or_right = word_index & 1;
		word_index >>= 1;

		counter_index = (word_index << 4) + offset;

		counter[i][word_index] |= ((uint64)0x1 << (2 + left_or_right + (offset << 2)));
		value = (counter[i][word_index] >> (offset << 2)) & 0x3;

		//posi
		if(flag[i][counter_index] == false)
		{
			if(value == 3)
			{
				counter[i][word_index] &= (~((uint64)0x3 << (offset << 2)));
			}
			else
			{
				counter[i][word_index] += ((uint64)0x1 << (offset << 2));
				return;
			}
		}
		//nega
		else
		{
			if(value == 1)
			{
				counter[i][word_index] -= ((uint64)0x1 << (offset << 2));

				flag[i][counter_index] = false;
				return;
			}
			else
			{
				counter[i][word_index] -= ((uint64)0x1 << (offset << 2));
				return;
			}	
		}
	}
}

int PCSketch::get_value(int index)
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

		int t = ((value & 3) - ((value >> (2 + anti_left_or_right)) & 1)) * (1 << (2 + 2 * i));

		high_value += (flag[i][(word_index << 4) + offset] == false) ? t : -t;
	}
}

#endif //_PCSKETCH_H