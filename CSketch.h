#ifndef _CSKETCH_H
#define _CSKETCH_H

#include <algorithm>
#include <cstring>
#include <string.h>
#include "params.h"
#include "BOBHash.h"
#include <iostream>
#include <algorithm>
using namespace std;

class CSketch
{	
private:
	BOBHash * bobhash[MAX_HASH_NUM * 2];
	int index[MAX_HASH_NUM];
	int *counter[MAX_HASH_NUM];
	int w, d;
	int MAX_CNT, MIN_CNT;
	int counter_index_size;
	uint64_t hash_value;

public:
	CSketch(int _w, int _d)
	{
		counter_index_size = 20;
		w = _w;
		d = _d;
		
		for(int i = 0; i < d; i++)	
		{
			counter[i] = new int[w];
			memset(counter[i], 0, sizeof(int) * w);
		}

		MAX_CNT = (1 << (COUNTER_SIZE - 1)) - 1;
		MIN_CNT = (-(1 << (COUNTER_SIZE - 1)));


		for(int i = 0; i < d * 2; i++)
		{
			bobhash[i] = new BOBHash(i + 1000);
		}
	}
	void Insert(const char * str)
	{
		int g = 0;
		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, strlen(str))) % w;
			g = (bobhash[i + d]->run(str, strlen(str))) % 2;

			if(g == 0)
			{
				if(counter[i][index[i]] != MAX_CNT)
				{
					counter[i][index[i]]++;
				}
			}
			else
			{
				if(counter[i][index[i]] != MIN_CNT)
				{
					counter[i][index[i]]--;
				}
			}
		}
	}
	void Delete(const char * str)
	{
		int g = 0;
		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, strlen(str))) % w;
			g = (bobhash[i + d]->run(str, strlen(str))) % 2;

			if(g == 1)
			{
				if(counter[i][index[i]] != MAX_CNT)
				{
					counter[i][index[i]]++;
				}
			}
			else
			{
				if(counter[i][index[i]] != MIN_CNT)
				{
					counter[i][index[i]]--;
				}
			}
		}
	}
	int Query(const char *str)
	{
		int temp;
		int res[MAX_HASH_NUM];
		int g;
		for(int i = 0; i < d; i++)
		{
			index[i] = (bobhash[i]->run(str, strlen(str))) % w;
			temp = counter[i][index[i]];
			g = (bobhash[i + d]->run(str, strlen(str))) % 2;

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
	~CSketch()
	{
		for(int i = 0; i < d; i++)	
		{
			delete []counter[i];
		}


		for(int i = 0; i < d * 2; i++)
		{
			delete bobhash[i];
		}
	}
};
#endif//_CSKETCH_H