#ifndef _PASKETCH_H
#define _PASKETCH_H

#include <algorithm>
#include <cstring>
#include <string.h>
#include "params.h"
#include "BOBHash.h"
#include <iostream>
#include <math.h>


using namespace std;

typedef unsigned long long int uint64;

class PASketch
{
private:
	int d;
	uint64 *counter[60];

	int *new_count;
	int *old_count;
	char **items;
	


	int word_index_size, counter_index_size;
	int word_num, counter_num;
	//word_num is the number of words in the first level.

	BOBHash * bobhash[MAX_HASH_NUM];


public:

	PASketch(int _word_num, int _d, int word_size);
	void Insert(const char * str);
	void PC_Insert(const char * str);
	void PC_Insert_num(const char * str, int num);

	int my_get_value(int index);

	int InsertAndQuery(const char *str);

	int Query(const char *str);
	int PC_Query(const char *str);



	//carry from the lower layer to the higher layer, maybe we will allocate the new memory;
	void carry(int index);
	int get_value(int index);

	int find_element_in_filter(const char *str);
	int find_empty_in_filter();


	~PASketch();	
};

int PASketch::find_element_in_filter(const char *str)
{
	for(int i = 0; i < FILTER_SIZE; i++)
	{
		if(strcmp(str, items[i]) == 0)
			return i;
	}
	return -1;
}
//can finish in finding element in filter
int PASketch::find_empty_in_filter()
{
	for(int i = 0; i < FILTER_SIZE; i++)
	{
		if(strlen(items[i]) == 0)
			return i;
	}
	return -1;
}
//Just for the consistency of the interface;
//For PASketch.h, the word_size must be 64;
PASketch::PASketch(int _word_num, int _d, int word_size)
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


	items = new char *[FILTER_SIZE];
	for(int i = 0; i < FILTER_SIZE; i++)
	{
		items[i] = new char[100];
		items[i][0] = '\0';
	}

	new_count = new int[FILTER_SIZE];
	old_count = new int[FILTER_SIZE];
	memset(new_count, 0, sizeof(int) * FILTER_SIZE);
	memset(old_count, 0, sizeof(int) * FILTER_SIZE);
		
	
}
void PASketch::Insert(const char * str)
{	
	int index = find_element_in_filter(str);
	int index_empty = find_empty_in_filter();
	int estimate_value, min_index, min_value, hash_value, temp;

	if(index != -1)
	{
		new_count[index] += 1;
		return;
	}
	else if(index_empty != -1)
	{
		strcpy(items[index_empty], str);
		new_count[index_empty] = 1;
		old_count[index_empty] = 0;
	}
	else
	{
		estimate_value = InsertAndQuery(str);



		min_index = 0;
		min_value = (1 << 30);
		for(int i = 0; i < FILTER_SIZE; i++)
		{
			if(strlen(items[i]) != 0 && min_value > new_count[i])
			{
				min_value = new_count[i];
				min_index = i;
			}
		}
		if(estimate_value > min_value)
		{
			temp = new_count[min_index] - old_count[min_index];
			if(temp > 0)
			{
				for(int i = 0; i < temp; i++)
				{
					PC_Insert(items[min_index]);
				}
			}
			strcpy(items[min_index], str);
			new_count[min_index] = estimate_value;
			old_count[min_index] = estimate_value;
		}
	}

}
int PASketch::Query(const char *str)
{

	int index = find_element_in_filter(str);
	if(index != -1)
	{
		return new_count[index];
	}
	int hash_value;
	int estimate_value = PC_Query(str);
	return estimate_value;
}

void PASketch::PC_Insert(const char *str)
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
int PASketch::PC_Query(const char *str)
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
int PASketch::InsertAndQuery(const char *str)
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
		
		if(((flag >> counter_offset[i]) & 1) == 0)
			continue;

		flag &= (~(1 << counter_offset[i]));


		value[i] = (counter[0][my_word_index] >> (counter_offset[i] << 2)) & 0xF;
		if (value[i] == 15)
		{
			counter[0][my_word_index] &= (~((uint64)0xF << (counter_offset[i] << 2)));
			value[i] == my_get_value(index[i]);
		}
		else
		{
			counter[0][my_word_index] += ((uint64)0x1 << (counter_offset[i] << 2));
			value[i] += get_value(index[i]) + 1;
		}
		min_value = value[i] < min_value ? value[i] : min_value;
	}
	return min_value;
}
int PASketch::my_get_value(int index)
{
	int left_or_right, anti_left_or_right;	
	int high_value = 0;
	
	int value;
	int word_index = index >> 4;
	int offset = index & 0xF;

	bool flag = true;
	for(int i = 1; i < 15; i++)
	{

		left_or_right = word_index & 1;
		anti_left_or_right = (left_or_right ^ 1);

		word_index >>= 1;

		if(flag)
		{
			counter[i][word_index] |= ((uint64)0x1 << (2 + left_or_right + (offset << 2)));
			value = (counter[i][word_index] >> (offset << 2)) & 0xF;

			if((value & 3) != 3)
			{
				counter[i][word_index] += ((uint64)0x1 << (offset << 2));
				flag = false;
			}
			else
			{
				counter[i][word_index] &= (~((uint64)0x3 << (offset << 2)));
			}
		}

		value = (counter[i][word_index] >> (offset << 2)) & 0xF;

		if(((value >> (2 + left_or_right)) & 1) == 0)
			return high_value;

		high_value += ((value & 3) - ((value >> (2 + anti_left_or_right)) & 1)) * (1 << (2 + 2 * i));

	}
}

void PASketch::carry(int index)
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

int PASketch::get_value(int index)
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

#endif //_PASKETCH_H