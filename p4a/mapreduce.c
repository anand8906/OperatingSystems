#include <stdio.h>
#include <stdlib.h>
#include <zconf.h>
#include <pthread.h>
#include <string.h>
#include "mapreduce.h"
#include <time.h>


#define BUCKETS 1229
/**
 *  Key List:
 *
 *  10 partitions:
 *
 *  p1  p2  p3  ...     p10
 *
 *  head    head
 *  k1       k4
 *  k2       k5
 *  k3       k6
 *
 *  Key Node:
 *
 *  k1 -> (key name, (value_node))
 *
 *  Value_node
 *
 *  (value, next -> value_node)
 *
 *
 *
 */

// todo: remove this - pasted from SO
void print_time() {
  time_t rawtime;
  struct tm *timeinfo;

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  printf("Current local time and date: %s", asctime(timeinfo));
}

void sort_keys_and_values(int);

void free_bitches();

static int num_files = 0;
pthread_mutex_t consumer_lock;

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
  unsigned long hash = 5381;
  int c;
  while ((c = *key++) != '\0')
    hash = hash * 33 + c;
  return hash % num_partitions;
}


//node structure for values LL
struct value_node {
  char *value;
  struct value_node *next;
};


//node structure for data of each key LL
typedef struct {
  char *key;
  struct value_node *value_list;
  struct value_node *value_array;
  int value_array_size;
  int next_index;
  pthread_mutex_t value_list_lock;
} key_node;

//Node structure for key LL
struct key_list {
  key_node *key_name; // key node k1
  struct key_list *next; // key_list - (k2)
};

//Data structure for each partition
typedef struct {
  int partition_number;
  struct key_list **buckets;

  struct key_list *array;
  int array_count;
  pthread_mutex_t bucket_locks[BUCKETS];
  int reducer_key_index;
} partition;

Partitioner curr_partitioner;
Mapper mapper_func;
Reducer curr_reduce;
int curr_num_reducers = 0;
char **files = NULL;
int **reducer_args = 0;

partition *partitions = NULL;

char *get_next(char *key, int partition_number) {
//  for (int i = 0; i < partitions[partition_number].array_count; i++) {
//    if (strcmp(partitions[partition_number].array[i].key_name->key, key) == 0) {
//
//
//    }
//  }

  partition part = partitions[partition_number];
  key_node *node = part.array[part.reducer_key_index].key_name;

  if (node->next_index < node->value_array_size) {
    char *val = node->value_array[node->next_index].value;
    node->next_index = node->next_index + 1;
    return val;
  } else if (node->next_index == node->value_array_size) {
    return NULL;
  }

  return NULL;

//  printf("Error: Did not find key in get next\n");
//  return NULL;
}

void free_pointers() {
  for(int i = 0; i < curr_num_reducers; i++)
  {
    partition p = partitions[i];
    for(int j = 0; j < BUCKETS; j++)
    {
      struct key_list *bucket = p.buckets[j];
      while(bucket != NULL) {
        key_node* keyNode = bucket->key_name;
        struct value_node* valueList = keyNode->value_list;
        while(valueList != NULL) {
          free(valueList->value);
          struct value_node* tmp_value_list = valueList;
          valueList = valueList->next;
          free(tmp_value_list);
        }
        free(keyNode->key);
        free(keyNode->value_array);
        free(keyNode);
        struct key_list *tmp_bucket = bucket;
        bucket = bucket->next;
        free(tmp_bucket);
      }
    }
    free(p.buckets);
    free(p.array);
//    free(p);
  }
  free(partitions);
  for(int i = 0; i < curr_num_reducers; i++)
  {
    free(reducer_args[i]);
  }
  free(reducer_args);
}

void *reduce_caller(void *reducer_num) {
  int num = *((int *) reducer_num);
//  printf("Sorting in reducer %d!\n", num);
//  print_time();
  sort_keys_and_values(num);
//  printf("Sorting done in reducer %d!\n", num);
//  print_time();

  for (int i = 0; i < partitions[num].array_count; i++) {
//    printf("here?\n");
    partitions[num].reducer_key_index = i;
    curr_reduce(partitions[num].array[i].key_name->key, get_next, num);
  }

  return NULL;
}

int sort_key_comparator(const void *a, const void *b) {
  struct key_list *list_a = (struct key_list *) a;
  struct key_list *list_b = (struct key_list *) b;
  return strcmp(list_a->key_name->key, list_b->key_name->key);
}

int sort_value_comparator(const void *a, const void *b) {
  struct value_node *node_a = (struct value_node *) a;
  struct value_node *node_b = (struct value_node *) b;

  return strcmp(node_a->value, node_b->value);
}

void *mapper_thread(void *arg) {
  while (1) {
    pthread_mutex_lock(&consumer_lock);
    if (num_files > 0) {
      int file_index = num_files--;
      pthread_mutex_unlock(&consumer_lock);
      mapper_func(files[file_index]);
    } else {
      pthread_mutex_unlock(&consumer_lock);
      break;
    }
  }

  return NULL;
}

void sort_keys_and_values(int i) {
//  for (int i = 0; i < curr_num_reducers; i++) {
//  printf("sorting partition %d\n", i);


  int key_count_in_partition = 0;
  for (int j = 0; j < BUCKETS; j++) {
    struct key_list *iter = partitions[i].buckets[j];
    while (iter != NULL) {
      key_count_in_partition++;
      iter = iter->next;
    }
  }

  struct key_list *array = (struct key_list *) malloc(sizeof(struct key_list) * key_count_in_partition);
  int x = 0;
  for (int j = 0; j < BUCKETS; j++) {
    struct key_list *iter = partitions[i].buckets[j];
    while (iter != NULL) {
      array[x] = *iter;
      x++;
      iter = iter->next;
    }
  }

  qsort(array, (size_t) key_count_in_partition, sizeof(struct key_list), sort_key_comparator);
  partitions[i].array = array;
  partitions[i].array_count = key_count_in_partition;

  // todo: free all the elements in the linkedlist

  // todo: Sort all value lists;


  for (int k = 0; k < key_count_in_partition; k++) {
    struct value_node *value_iter = partitions[i].array[k].key_name->value_list;
    int value_count = 0;
    while (value_iter != NULL) {
      value_count++;
      value_iter = value_iter->next;
    }

    struct value_node *value_array = (struct value_node *) malloc(sizeof(struct value_node) * value_count);
    value_iter = partitions[i].array[k].key_name->value_list;

    // todo: free all the elements in the linkedlist

    for (int m = 0; m < value_count; m++) {
      value_array[m] = *value_iter;
      value_iter = value_iter->next;
    }
    qsort(value_array, (size_t) value_count, sizeof(struct value_node), sort_value_comparator);
    partitions[i].array[k].key_name->value_array = value_array;
    partitions[i].array[k].key_name->value_array_size = value_count;
    partitions[i].array[k].key_name->next_index = 0;
  }


//  }
}

void MR_Run(int argc, char *argv[],
            Mapper map, int num_mappers,
            Reducer reduce, int num_reducers,
            Partitioner partition_func) {

  if (pthread_mutex_init(&consumer_lock, NULL) != 0) {
    printf("Mutex init failed\n");
  };

  curr_partitioner = partition_func == NULL ? MR_DefaultHashPartition : partition_func;
  curr_reduce = reduce;
  mapper_func = map;
  curr_num_reducers = num_reducers;
  reducer_args = malloc(sizeof(int *) * num_reducers);
  partitions = (partition *) malloc(sizeof(partition) * num_reducers);

  for (int i = 0; i < num_reducers; i++) {
    partitions[i].buckets = NULL;

    for (int j = 0; j < BUCKETS; j++) {
      pthread_mutex_init(&partitions[i].bucket_locks[j], NULL);
    }

    int partition_num = i;

    partitions[partition_num].buckets = malloc(sizeof(struct key_list *) * BUCKETS);

    for (int j = 0; j < BUCKETS; j++) {
      partitions[partition_num].buckets[j] = NULL;
    }
  }

  num_files = argc - 1;
  files = argv;

  pthread_t mapper_threads[num_mappers];
//  printf("Starting map!\n");
//  print_time();

  for (int i = 0; i < num_mappers; i++) {
    pthread_create(&mapper_threads[i], NULL, mapper_thread, NULL);
  }

  for (int i = 0; i < num_mappers; i++) {
    pthread_join(mapper_threads[i], NULL);
  }

//  printf("Mapping done!\n");
//  print_time();

//  sort_keys_and_values();

  //call reducers
  pthread_t reducer_threads[num_reducers];

//  printf("Starting reducers!\n");
//  print_time();

  for (int i = 0; i < num_reducers; i++) {
    int *arg = (int *) malloc(sizeof(int));
    *arg = i;
    reducer_args[i] = arg;
    // todo: free these somehow;
    pthread_create(&reducer_threads[i], NULL, reduce_caller, (void *) arg);
  }
  for (int i = 0; i < num_reducers; i++) {
    pthread_join(reducer_threads[i], NULL);
//    printf("1 Reducer done!\n");
  }

  free_pointers();


//  print_time();

  // What's in my DS?

//  for(int i = 0; i < num_reducers;i++) {
//    partition part = partitions[i];
//    for(int j = 0; j < part.array_count; j++) {
//      printf("%s\n", part.array[j].key_name->key);
//
//      for(int k = 0; k < part.array[j].key_name->value_array_size; k++) {
//        printf("%s\n", part.array[j].key_name->value_array[k].value);
//      }
//    }
//  }

}

void free_bitches() {
  for (int i = 0; i < curr_num_reducers; i++) {
    for (int j = 0; j < partitions[i].array_count; j++) {

      struct key_list *iter = &partitions[i].array[j];
      pthread_mutex_destroy(&iter->key_name->value_list_lock);


      for (int k = 0; k < iter->key_name->value_array_size; k++) {
        free(iter->key_name->value_array[k].value);
      }

      free(iter->key_name->value_array);
      free(iter->key_name->key);
      free(iter->key_name);
//    free(&partitions[i].array[j]);
    }

//    for (int j = 0; j < BUCKETS; j++) {
//      pthread_mutex_destroy(&partitions[i].bucket_locks[j]);
//    }

    free(partitions[i].buckets);
    free(partitions[i].array);
  }
  free(partitions);
}

void MR_Emit(char *key, char *value) {

  unsigned long partition_num = curr_partitioner(key, curr_num_reducers);
  unsigned long bucket_in_partition = MR_DefaultHashPartition(key, BUCKETS);

  pthread_mutex_lock(&partitions[partition_num].bucket_locks[bucket_in_partition]);

  struct key_list *iter = partitions[partition_num].buckets[bucket_in_partition];

  while (iter != NULL) {
    if (strcmp(iter->key_name->key, key) == 0) {

      // todo: free this
      struct value_node *node = (struct value_node *) malloc(sizeof(struct value_node));

      node->value = strdup(value);

      pthread_mutex_lock(&iter->key_name->value_list_lock);

      node->next = iter->key_name->value_list;
      iter->key_name->value_list = node;

      pthread_mutex_unlock(&iter->key_name->value_list_lock);
      pthread_mutex_unlock(&partitions[partition_num].bucket_locks[bucket_in_partition]);

      return;
    }
    iter = iter->next;
  }
  // No such key found before;

  key_node *node = (key_node *) malloc(sizeof(key_node));
  struct key_list *key_list_node = (struct key_list *) malloc(sizeof(struct key_list));
  struct value_node *value_list = (struct value_node *) malloc(sizeof(struct value_node));;

  value_list->next = NULL;

  // todo: free this;
  value_list->value = strdup(value);

  // todo: free this
  node->key = strdup(key);
  node->value_list = value_list;

  key_list_node->key_name = node;

  key_list_node->next = partitions[partition_num].buckets[bucket_in_partition];

  if (pthread_mutex_init(&key_list_node->key_name->value_list_lock, NULL) != 0) {
    printf("Couldn't initialize value list lock");
    exit(1);
  };

  partitions[partition_num].buckets[bucket_in_partition] = key_list_node;
  pthread_mutex_unlock(&partitions[partition_num].bucket_locks[bucket_in_partition]);
}
