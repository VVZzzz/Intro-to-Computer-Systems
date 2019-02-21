#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cachelab.h"

/*memory's address 64bit*/
typedef unsigned long long int mem_addr_t;

typedef struct {
  int s; /*2^s is set's size*/
  int b; /*2^b cacheline block's size*/
  int E; /*num of lines per set*/
  int S; /*num of set*/
  int B; /*B = 2^b*/

  int hits;
  int misses;
  int evicts;
} Cache_para;

typedef struct {
  int last_used;  /*LRU's flag*/
  int valid;      /*valid flag*/
  mem_addr_t tag; /*tag section*/
  char *block;    /*block per line*/
} Cache_set_line;

typedef struct {
  Cache_set_line *lines;
} Cache_set;

typedef struct {
  Cache_set *sets;
} Cache;

/*better to build a class 'Cache'*/
/*
 * create a new Cahe
 */
Cache build_cache(long long num_sets, int num_lines, long long block_size) {
  Cache new_cache;
  int set_index;
  int line_index;
  new_cache.sets = (Cache_set *)malloc(sizeof(Cache_set) * num_sets);
  for (set_index = 0; set_index < num_sets; set_index++) {
    new_cache.sets[set_index].lines =
        (Cache_set_line *)malloc(sizeof(Cache_set_line) * num_lines);
    for (line_index = 0; line_index < num_lines; line_index++) {
      new_cache.sets[set_index].lines[line_index].last_used = 0;
      new_cache.sets[set_index].lines[line_index].valid = 0;
      new_cache.sets[set_index].lines[line_index].tag = 0;
    }
  }
  return new_cache;
}

/*
 * delete the new Cache
 */
void clear_cache(Cache sim_cache, long long num_sets) {
  int set_index;
  for (set_index = 0; set_index < num_sets; set_index++) {
    Cache_set tmp_set = sim_cache.sets[set_index];
    if (tmp_set.lines != NULL) free(tmp_set.lines);
    tmp_set.lines = NULL;
  }
  if (sim_cache.sets != NULL) free(sim_cache.sets);
  sim_cache.sets = NULL;
}

/*
 * find a empty line for per cache_set
 * if no empty line,return -1
 */
int find_emptyline(Cache_set cache_set, Cache_para para) {
  int num_lines = para.E;
  int line_index;
  Cache_set_line set_line;
  for (line_index = 0; line_index < num_lines; line_index++) {
    set_line = cache_set.lines[line_index];
    if (set_line.valid == 0) return line_index;
  }
  // if no empty line,return -1;
  return -1;
}

/*
 * Use LRU(least recently used) rule , find the LRU line to evict
 * That is contract each line's using times, return the least one;
 * 'used_lines' [0]:min_used [1]:max_used;
 * return min_used_index;
 */
int find_evic_line(Cache_set cache_set, Cache_para para, int *used_lines) {
  int num_lines = para.E;
  int min_used;
  int max_used = min_used = cache_set.lines[0].last_used;
  int min_used_index = 0;
  Cache_set_line set_line;
  int line_index;
  for (line_index = 1; line_index < num_lines; line_index++) {
    set_line = cache_set.lines[line_index];
    if (set_line.last_used < min_used) {
      min_used = set_line.last_used;
      min_used_index = line_index;
    }
    if (max_used < set_line.last_used) {
      max_used = set_line.last_used;
    }
  }
  used_lines[0] = min_used;
  used_lines[1] = max_used;
  return min_used_index;
}

/* simulate reading cache procedure*/
Cache_para run_read_sim(Cache sim_cache, Cache_para para, mem_addr_t address) {
  int line_index;
  int num_lines = para.E;
  int cache_full = 1;
  int prev_hits = para.hits;
  int tag_size = 64 - (para.b + para.s);
  mem_addr_t input_tag = (address >> (para.s + para.b));
  mem_addr_t temp = (address << tag_size);
  // mem_addr_t set_index = (temp >> para.b);
  mem_addr_t set_index = (temp >> (64 - para.s));

  Cache_set cache_set = sim_cache.sets[set_index];
  for (line_index = 0; line_index < num_lines; line_index++) {
    Cache_set_line temp_line = cache_set.lines[line_index];
    if (temp_line.valid) {
      if (temp_line.tag == input_tag) {
        /*hit*/
        temp_line.last_used++;
        para.hits++;
        cache_set.lines[line_index] = temp_line;
      }
    } else if (!(temp_line.valid) && cache_full) {
      /*the line is valid,eaqul to empty line*/
      /*so set the cache_full*/
      cache_full = 0;
    }
  }
  /*if hit just return*/
  if (prev_hits == para.hits)
    para.misses++;
  else
    return para;

  /*misses happend , find evict line*/
  int *used_line = (int *)malloc(sizeof(int) * 2);
  int min_used_index = find_evic_line(cache_set, para, used_line);

  /*
   * If cache if full , need to evict the LRU line;
   * And overwrite the line. The new line's last_used should
   * be the former max + 1.(means recently used)
   * If cache if not full, find the first empty line, write in;
   */
  if (cache_full) {
    para.evicts++;
    cache_set.lines[min_used_index].tag = input_tag;
    cache_set.lines[min_used_index].last_used = used_line[1] + 1;
  } else {
    int empty_line_index = find_emptyline(cache_set, para);
    cache_set.lines[empty_line_index].tag = input_tag;
    cache_set.lines[empty_line_index].valid = 1;
    cache_set.lines[empty_line_index].last_used = used_line[1] + 1;
  }
  free(used_line);
  return para;
}

void print_help(char *argv[]) {
  printf("Usage %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
  printf("Options:\n");
  printf("  -h         Print this help message.\n");
  printf("  -v         Optional verbose flag.\n");
  printf("  -s <num>   Number of set index bits.\n");
  printf("  -E <num>   Number of lines per set.\n");
  printf("  -b <num>   Number of block offset bits.\n");
  printf("  -t <file>  Trace file.\n");
  printf("\nExamples:\n");
  printf("  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
  printf("  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
}

int main(int argc, char **argv) {
  char c;
  char *trace_file = NULL;
  // int verbose_flag = 0;

  Cache_para param;
  Cache sim_cache;
  long long num_sets;
  long long block_size;
  memset(&param, 0, sizeof(param));

  FILE *read_trace;
  char trace_cmd;
  mem_addr_t address;
  int size;

  /*command line opt*/
  while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
    switch (c) {
      case 's':
        param.s = atoi(optarg);
        break;
      case 'E':
        param.E = atoi(optarg);
        break;
      case 'b':
        param.b = atoi(optarg);
        break;
      case 't':
        trace_file = optarg;
        break;
      case 'v':
        // verbose_flag = 1;
        break;
      case 'h':
        print_help(argv);
        exit(0);
      default:
        print_help(argv);
        exit(1);
    }
  }
  if (param.s == 0 || param.E == 0 || param.b == 0 || trace_file == NULL) {
    printf("%s: Missing required command line\n", argv[0]);
    print_help(argv);
    exit(1);
  }

  /*processing trace file*/
  num_sets = (1 << param.s);
  block_size = (1 << param.b);
  sim_cache = build_cache(num_sets, param.E, block_size);

  read_trace = fopen(trace_file, "r");
  if (read_trace != NULL) {
    while (fscanf(read_trace, " %c %llx,%d", &trace_cmd, &address, &size) ==
           3) {
      switch (trace_cmd) {
        case 'I':
          break;
        case 'L':
          param = run_read_sim(sim_cache, param, address);
          break;
        case 'S':
          param = run_read_sim(sim_cache, param, address);
          break;
        case 'M':
          param = run_read_sim(sim_cache, param, address);
          param = run_read_sim(sim_cache, param, address);
          break;
        default:
          break;
      }
    }
  }
  printSummary(param.hits, param.misses, param.evicts);
  clear_cache(sim_cache, num_sets);
  fclose(read_trace);
  return 0;
}
