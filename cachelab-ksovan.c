// cachelab.c - CSC322 - ksovan 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "cachelab.h"

// Cache hit time and miss penalty are defined in cachelab.h

// Structure for each cache line
typedef struct {
    int valid;
    unsigned long tag;
    int lruCount; // counter
} cache_line;

// Structure for cache set
typedef struct {
    cache_line *lines;
} cache_set;

// Structure for cache
typedef struct {
    cache_set *sets;
    int S; //  sets
    int E; //  lines per set
} cache_simulator;

int useFIFO = 0; 


// print result of cache simulation showing hit number, miss number, miss rate, and total running time
void printResult(int hits, int misses, int missRate, int runTime) {
    printf("[result] hits: %d misses: %d miss rate: %d%% total running time: %d cycle\n",
           hits, misses, missRate, runTime);
}

// initialize cache
cache_simulator *initCache(int s, int e) {
    cache_simulator *cache = malloc(sizeof(cache_simulator));
    cache->S = 1 << s; //  sets = 2^s
    cache->E = 1 << e; //  lines per set = 2^e
    cache->sets = malloc(cache->S * sizeof(cache_set));

    for (int i = 0; i < cache->S; i++) {
        cache->sets[i].lines = malloc(cache->E * sizeof(cache_line));
        for (int j = 0; j < cache->E; j++) {
            cache->sets[i].lines[j].valid = 0;
            cache->sets[i].lines[j].tag = 0;
            cache->sets[i].lines[j].lruCount = 0;
        }
    }
    return cache;
}

//  free cache
void freeCache(cache_simulator *cache) {
    for (int i = 0; i < cache->S; i++) {
        free(cache->sets[i].lines);
    }
    free(cache->sets);
    free(cache);
}

// find hit/miss using LRU 
int accessCache(cache_simulator *cache, unsigned long long address, int s, int b, int *hits, int *misses, int *time) {
    unsigned long long setIndex = (address >> b) & ((1 << s) - 1);
    unsigned long long tag = address >> (s + b);

    cache_set *set = &cache->sets[setIndex];

    // check hit
    for (int i = 0; i < cache->E; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            (*hits)++;
            printf("%llX H\n", address);
            // For LRU 
            if (!useFIFO) {
                set->lines[i].lruCount = *time;
            }
            return 1;
        }
    }

    // miss
    (*misses)++;
    printf("%llX M\n", address);

    // find an empty line first
    int replaceIndex = -1;
    for (int i = 0; i < cache->E; i++) {
        if (!set->lines[i].valid) {
            replaceIndex = i;
            break;
        }
    }

    // if no empty line, find where
    if (replaceIndex == -1) {
        int minLRU = set->lines[0].lruCount;
        replaceIndex = 0;
        for (int i = 1; i < cache->E; i++) {
            if (set->lines[i].lruCount < minLRU) {
                minLRU = set->lines[i].lruCount;
                replaceIndex = i;
            }
        }
    }

    // replace 
    set->lines[replaceIndex].valid = 1;
    set->lines[replaceIndex].tag = tag;
    set->lines[replaceIndex].lruCount = *time;

    return 0;
}

int main(int argc, char **argv) {
    int m = 0, s = 0, e = 0, b = 0;
    char *inputFile = NULL;
    char *replacement = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "m:s:e:b:i:r:")) != -1) {
        switch (opt) {
            case 'm':
                m = atoi(optarg);
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'e':
                e = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'i':
                inputFile = optarg;
                break;
            case 'r':
                replacement = optarg;
                break;
            default:
                printf("Missing or wrong arguments.\n");
                return 1;
        }
    }

    if (!inputFile || !replacement) {
        printf("Missing required arguments.\n");
        return 1;
    }

    if (strcmp(replacement, "lru") == 0) {
    useFIFO = 0;
	} else if (strcmp(replacement, "fifo") == 0) {
  	  useFIFO = 1;
	} else {
   	 printf("Only LRU and FIFO replacement are implemented.\n");
   	 return 1;	
	}

    FILE *fp = fopen(inputFile, "r");
    if (!fp) {
        printf("Error: cannot open input file %s\n", inputFile);
        return 1;
    }

    cache_simulator *cache = initCache(s, e);

    unsigned long long address;
    int hits = 0, misses = 0;
    int time = 0;

    // read hex addresses
    while (fscanf(fp, "%llx", &address) == 1) {
        time++;
        accessCache(cache, address, s, b, &hits, &misses, &time);
    }

    fclose(fp);

    // calculate
    int totalAccess = hits + misses;
    double missRate = (double)misses / totalAccess;
    double avgAccessTime = HIT_TIME + missRate * MISS_PENALTY;
    int totalRunTime = (int)(avgAccessTime * totalAccess);

    printResult(hits, misses, (int)(missRate * 100), totalRunTime);

    freeCache(cache);
    return 0;
}

