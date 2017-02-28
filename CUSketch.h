#ifndef _CUSKETCH_H
#define _CUSKETCH_H

#include <algorithm>
#include <cstring>
#include <string.h>
#include "params.h"
#include "BOBHash.h"
#include <iostream>

using namespace std;

class CUSketch
{	
private:
	BOBHash * bobhash[MAX_HASH_NUM];
	int index[MAX_HASH_NUM];
	int *counter[MAX_HASH_NUM];
	int w, d;
	int MAX_CNT;
	int counter_index_size;
	uint64_t hash_value;

public:
	CUSketch(int _w, int _d)
	{
		counter_index_size = 20;
		w = _w;
		d = _d;
		
		for(int i = 0; i < d; i++)	
		{
			counter[i] = new int[w];
			memset(counter[i], 0, sizeof(int) * w);
		}

		MAX_CNT = (1 << COUNTER_SIZE) - 1;

		for(int i = 0; i < d; i++)
		{
			bobhash[i] = new BOBHash(i + 1000);
		}
	}
	void Insert(const char * str)
	{
		int min_value = 1 << 30;
		int temp;

		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, strlen(str))) % w;
			temp = counter[i][index[i]];
			min_value = temp < min_value ? temp : min_value;

		}
		if(min_value == MAX_CNT)
			return;

		for(int i = 0; i < d; i++)
		{
			if(counter[i][index[i]] == min_value)
				counter[i][index[i]] ++;
		}
	}
	int Query(const char *str)
	{
		int min_value = 1 << 30;
		int temp;
		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, strlen(str))) % w;
			temp = counter[i][index[i]];
			min_value = temp < min_value ? temp : min_value;
		}
		return min_value;
	}
	~CUSketch()
	{
		for(int i = 0; i < d; i++)	
		{
			delete []counter[i];
		}
		for(int i = 0; i < d; i++)
		{
			delete bobhash[i];
		}
	}
};
#endif//_CUSKETCH_H