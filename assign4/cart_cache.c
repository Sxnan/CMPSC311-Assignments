////////////////////////////////////////////////////////////////////////////////
//
//  File           : cart_cache.c
//  Description    : This is the implementation of the cache for the CART
//                   driver.
//
//  Author         : Xuannan Su
//  Last Modified  : 10/30/2016
//

// Includes
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

// Project includes
#include <cart_controller.h>
#include <cart_cache.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>

// Defines
typedef struct{
	char data[CART_FRAME_SIZE];	// the text in the frame
	unsigned int indicator;		// indicator of last use or hit times base on cache mode
	int cart;
	int frame;
}CacheFrame;

// Global data
uint32_t max;

CacheFrame *cache;	// All the cache frame

int cache_map_table[CART_MAX_CARTRIDGES][CART_CARTRIDGE_SIZE];	// Idx of the cach frame for each frame

unsigned int count;	// The number of frame left

ReplacementPolicy replacement_policy = LRU;

//
// Functions

// Determine the frame to replace from cache
int frame_to_replace(int *cart, int *frame);

// Update the indicator base on the replacement policy
int update_indicator(CacheFrame *cache);

////////////////////////////////////////////////////////////////////////////////
//
// Function	: frame_to_replace
// Description	: Determine the frame to replace
// 
// Input	: cart - Output parameter for the cart of the frame
// 		: frame - Output parameter for the frame number
// Output	: 0 if successful

int frame_to_replace(int *cart, int *frame){

	//Check the replacement policy
	if (replacement_policy == LRU){
		//LRU replacement policy
		unsigned int min = 0;
		min = ~min;		//flip the bit set to max

		//find the frame with minmum indicator
		for (unsigned int i = 0; i < count; ++i){
			if (min > cache[i].indicator){
				*cart = cache[i].cart;
				*frame = cache[i].frame;
				min = cache[i].indicator;
			}
		}
	} else if (replacement_policy == LFU){
		//LFU relacement policy
		int set = 0;		// Flag indicate if the victim is determined
		unsigned int min = 0;
		min = ~min;
		
		//find the frame with minmum indicator and greater than 100
		for (unsigned int i = 0; i  < count; ++i){
			// Check if the indicator greater than 100
			if (cache[i].indicator >= 100){
				if (min > cache[i].indicator){
					*cart = cache[i].cart;
					*frame = cache[i].frame;
					min = cache[i].indicator;	
					set = 1;
				}
			}

		
		}
		
		// Check if the victim is ont determined
		if (set == 0){
			for (unsigned int i = 0; i < count; ++i){
				if (min > cache[i].indicator){
					*cart = cache[i].cart;
					*frame = cache[i].frame;
					min = cache[i].indicator;
				}
			
			}
			
		} 
		
	} else {
		// Random replacement policy
		int idx;
		idx = getRandomValue(0, count-1);
		*cart = cache[idx].cart;
		*frame = cache[idx].frame;

	}

	return 0;

}

////////////////////////////////////////////////////////////////////////////////
//
// Function	: update_indicator
// Description	: Update the indicator base on the replacement policy etermine the frame to replace
// 
// Input	: cache - the frame whose indicator need to be updated
// Output	: 0 if successful

int update_indicator(CacheFrame *cache){

	//Check the replacement policy
	if (replacement_policy == LRU){
		//LRU replacement policy
		static unsigned int recent_used = 0;
		cache->indicator = recent_used;
		recent_used += 1;
		
	} else if (replacement_policy == LFU){
		//LFU replacement policy
		cache->indicator += 1;
		
	} else {
		//Random replacement policy
		
	}

	return 0;

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : set_cart_cache_size
// Description  : Set the size of the cache (must be called before init)
//
// Inputs       : max_frames - the maximum number of items your cache can hold
// Outputs      : 0 if successful, -1 if failure

int set_cart_cache_size(uint32_t max_frames) {
	
	max = max_frames;
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_cart_cache
// Description  : Initialize the cache and note maximum frames
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int init_cart_cache(void) {
	
	//allocate memory for cache
	cache = malloc(max * sizeof(CacheFrame));
	
	// initilize cache map
	for (int i = 0; i < CART_MAX_CARTRIDGES; ++i){
		for (int j = 0; j < CART_CARTRIDGE_SIZE; ++j){
			cache_map_table[i][j] = -1;
		}
	}
	
	// initilize number of frame
	count = 0;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : close_cart_cache
// Description  : Clear all of the contents of the cache, cleanup
//
// Inputs       : none
// Outputs      : o if successful, -1 if failure

int close_cart_cache(void) {
	
	free(cache);
	cache = NULL;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : put_cart_cache
// Description  : Put an object into the frame cache
//
// Inputs       : cart - the cartridge number of the frame to cache
//                frm - the frame number of the frame to cache
//                buf - the buffer to insert into the cache
// Outputs      : 0 if successful, -1 if failure

int put_cart_cache(CartridgeIndex cart, CartFrameIndex frm, void *buf)  {
	
	if (max == 0) return 0;

	//Check if the frame in the cache
	if (cache_map_table[cart][frm] != -1){

		// have a deep copy of the data
		memcpy(cache[cache_map_table[cart][frm]].data, buf, CART_FRAME_SIZE);
		update_indicator(cache + cache_map_table[cart][frm]);
		
	} else if (count < max){
		// cache is not full
		
		// have a deep copy of the data
		memcpy(cache[count].data, buf, CART_FRAME_SIZE);

		// update the info of the frame
		update_indicator(cache + count);
		cache[count].cart = cart;
		cache[count].frame = frm;

		// set the map table
		cache_map_table[cart][frm] = count;

		// update left_count
		count += 1;
 	
	} else {

		// cache is full
		int curCart, curFrame;
		int curCacheIdx;		// Current working frame that need to be replaced

		// determind which frame to be replaced
		frame_to_replace(&curCart, &curFrame);
		curCacheIdx = cache_map_table[curCart][curFrame];

		// have a deep copy of the data
		memcpy(cache[curCacheIdx].data, buf, CART_FRAME_SIZE);

		// update the info of the frame
		if (replacement_policy == LFU) cache[curCacheIdx].indicator = 0;	//Reset the indicator
		update_indicator(cache + curCacheIdx);
		cache[curCacheIdx].cart = cart;
		cache[curCacheIdx].frame = frm;
		
		// update the map table
		cache_map_table[cart][frm] = curCacheIdx;
		cache_map_table[curCart][curFrame] = -1;	//Invalidate 

	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_cart_cache
// Description  : Get an frame from the cache (and return it)
//
// Inputs       : cart - the cartridge number of the cartridge to find
//                frm - the  number of the frame to find
// Outputs      : pointer to cached frame or NULL if not found

void * get_cart_cache(CartridgeIndex cart, CartFrameIndex frm) {

	// Check if it is in the cache
	if (cache_map_table[cart][frm] == -1){
		//not in the cache
		return NULL;
	}
	
	// Update the indicator
	update_indicator(cache + cache_map_table[cart][frm]);

	return (void *)cache[cache_map_table[cart][frm]].data;	
	
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : delete_cart_cache
// Description  : Remove a frame from the cache (and return it)
//
// Inputs       : cart - the cart number of the frame to remove from cache
//                blk - the frame number of the frame to remove from cache
// Outputs      : pointe buffer inserted into the object

void * delete_cart_cache(CartridgeIndex cart, CartFrameIndex blk) {
	void *buf;	//buf to store the data
	
	// Allocate memory
	buf = malloc(CART_FRAME_SIZE * sizeof(char));

	// Have a deep copy of data
	memcpy(buf, cache[cache_map_table[cart][blk]].data, CART_FRAME_SIZE);

	// Set the indicator to zero
	cache[cache_map_table[cart][blk]].indicator = 0;

	// Invalidate map
	cache_map_table[cart][blk] = -1;

	return buf;	
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : set_replacement_policy 
// Description  : Set the replacement policy 
//
// Inputs       : policy - the replacement policy to set 
// Outputs      : 0 if success 

int set_replacement_policy(ReplacementPolicy policy){

	replacement_policy = policy;

	return 0;
}

//
// Unit test

////////////////////////////////////////////////////////////////////////////////
//
// Function     : cartCacheUnitTest
// Description  : Run a UNIT test checking the cache implementation
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int cartCacheUnitTest(void) {
	
	void *randomData;
	int cart;
	int frame;

	init_cart_cache();

	randomData = malloc(1024 * sizeof(char));

	for (int i = 0; i < 10000; ++i){

		// generata
		cart = getRandomValue(0, 63);
		frame = getRandomValue(0, 1023);
		getRandomData((char *)randomData, 1024);
		
		// Check if it is in the cache
		if (cache_map_table[cart][frame] == -1){
			// Not in the frame
			if (get_cart_cache(cart, frame) != NULL) return -1;	// Test Fail if data read from cahce

		} else {
			// In the frame
			if (get_cart_cache(cart, frame) == NULL) return -1;	// Test Fail if data read from cahce
		}

		// generate random number
		cart = getRandomValue(0, 63);
		frame = getRandomValue(0, 1023);

		// random frame put
		put_cart_cache(cart, frame, randomData);
		
		if (i%100 == 0) printf("Unit Test Complete: %d%%\r", (i/100));
		fflush(stdout);
	}

	free(randomData);

	close_cart_cache();

	// Return successfully
	logMessage(LOG_OUTPUT_LEVEL, "Cache unit test completed successfully.");
	return(0);
}
