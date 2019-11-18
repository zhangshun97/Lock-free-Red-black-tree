#include "tree.h"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

using namespace std;

int total_size = 0, size_per_thread = 0;
int numbers[1000001];
tree_node *root;

void load_data_from_txt();
int run_multi_thread(int thread_count);
void *run(void *p);
void run_serial();

int main()
{
    load_data_from_txt();
    printf("total_size: %d\n", total_size);
    // root = rb_init();
    // run_serial();
    for (int i = 1; i <= 17; i++)
    {
        root = rb_init();
        run_multi_thread(i);
    }
    // root = rb_init();
    // run_multi_thread(1);
    // root = rb_init();
    // run_multi_thread(2);
    // root = rb_init();
    // run_multi_thread(4);
    // root = rb_init();
    // run_multi_thread(8);
    // root = rb_init();
    // run_multi_thread(16);
    root = rb_init();
    run_multi_thread(32);
    return 0;
}

void run_serial()
{
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < total_size; i++)
    {
        rb_insert(root, numbers[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) * 1e9;
    elapsed_time += (end.tv_nsec - start.tv_nsec);
    elapsed_time *= 1e-9;
    cout << "time taken by program with 1 threads: " << fixed << elapsed_time << "sec" << endl;
}

void *run(void *p)
{
    int *start = (int *)p;
    int count = size_per_thread;
    for (int i = 0; i < count; i++)
    {
        int element = start[i];
        rb_insert(root, element);
        dbg_printf("[RUN] finish inserting element %d\n", element);
    }
    return NULL;
}

int run_multi_thread(int thread_count)
{
    pthread_t tid[thread_count];
    size_per_thread = total_size / thread_count;

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    
    thread_count--; // main thread will also perform insertion
    for (int i = 0; i < thread_count; i++)
    {
        pthread_create(&tid[i], NULL, run, numbers + (i + 1) * size_per_thread);
    }
    run(numbers);
    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(tid[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) * 1e9;
    elapsed_time += (end.tv_nsec - start.tv_nsec);
    elapsed_time *= 1e-9;
    cout << "time taken by program with " << thread_count + 1 << " threads: " << fixed << elapsed_time << "sec" << endl;

    // show_tree(root);
    return 0;
}

void load_data_from_txt()
{
    char buffer[20];
    fstream file;
    file.open("data.txt", ios::in);
    total_size = 0;
    while(!file.eof())
    {
        file.getline(buffer, 20, '\n');

        int result = strtol(buffer, NULL, 10);
        if (result > 0)
        {
            numbers[total_size++] = result;
        }
    }
}
