#include "header.h"

// sort data from "data.in" and write the sorted data to "data.out" by using
// merge sort with multiple threads

void merge(int arr[], int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;
    int L[n1], R[n2];
    for (i = 0; i < n1; i++) {
        L[i] = arr[l + i];
    }
    for (j = 0; j < n2; j++) {
        R[j] = arr[m + 1 + j];
    }
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort(int arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

typedef struct {
    int *arr;
    int l;
    int r;
} thread_args;

void *mergeSort_multipthreads(void *arg) {
    thread_args *args = (thread_args *)arg;
    int *arr = args->arr;
    int l = args->l;
    int r = args->r;

    if (l < r) {
        int m = l + (r - l) / 2;
        pthread_t tid1, tid2;
        pthread_attr_t attr1, attr2;
        pthread_attr_init(&attr1);
        pthread_attr_init(&attr2);
        pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setscope(&attr1, PTHREAD_SCOPE_SYSTEM);
        pthread_attr_setscope(&attr2, PTHREAD_SCOPE_SYSTEM);
        thread_args args1 = {arr, l, m};
        thread_args args2 = {arr, m + 1, r};
        if (r - l < 100000) {
            mergeSort(arr, l, m);
            mergeSort(arr, m + 1, r);
            merge(arr, l, m, r);
            return NULL;
        }

        int rc1 = pthread_create(&tid1, NULL, mergeSort_multipthreads, &args1);
        int rc2 = pthread_create(&tid2, NULL, mergeSort_multipthreads, &args2);
        if (rc1 || rc2) {
            printf("Error creating thread\n");
            exit(-1);
        }
        int rc3 = pthread_join(tid1, NULL);
        int rc4 = pthread_join(tid2, NULL);
        if (rc3 || rc4) {
            printf("Error joining thread\n");
            exit(-1);
        }
        merge(arr, l, m, r);
    }
}

int main(int argc, char *argv[]) {
    FILE *f = fopen("data.in", "r");
    int n;
    fscanf(f, "%d", &n);
    int *arr = malloc(n * sizeof(int));
    printf("n = %d\n", n);
    if (arr == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }
    for (int i = 0; i < n; i++) {
        fscanf(f, "%d", &arr[i]);
    }
    fclose(f);
    clock_t start, end;
    start = clock();
    thread_args args = {arr, 0, n - 1};
    mergeSort_multipthreads(&args);

    end = clock();

    FILE *f1 = fopen("data.out", "w+");
    fprintf(f1, "%d\n", n);
    for (int i = 0; i < n; i++) {
        fprintf(f1, "%d ", arr[i]);
    }
    fclose(f1);
    free(arr);
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time taken by single pthread: %f second(s)\n", time_taken);
    return 0;
}