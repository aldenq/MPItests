#include <mpi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define WORK_UNIT_SIZE 4096
#define END 1000000000

struct workerResponse
{
    uint64_t start;
    char responses[WORK_UNIT_SIZE];
};

char isPrime(uint64_t n)
{
    uint64_t base = 2;
    if (n % 2 == 0)
    {
        return 0;
    }
    base = 3;
    while (n / base >= base)
    {
        if (n % base == 0)
        {
            return 0;
        }
        base += 2;
    }
    return 1;
}

struct workerResponse handleWorkload(uint64_t base)
{
    struct workerResponse response;
    response.start = base;
    for (int i = 0; i < WORK_UNIT_SIZE; i++)
    {
        response.responses[i] = isPrime(i + base);
    }
    return response;
}

void worker()
{
    while (1)
    {

        uint64_t batch;
        //receive workload
        MPI_Scatter(0, 0, MPI_LONG, &batch, 1, MPI_LONG, 0, MPI_COMM_WORLD);
        struct workerResponse response = handleWorkload(batch);
        //give response
        MPI_Gather(&response, sizeof(struct workerResponse), MPI_BYTE, 0, sizeof(struct workerResponse), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
}

void printOutput(struct workerResponse *responses, int numWorkers)
{
    for (int i = 0; i < numWorkers; i++)
    {
        struct workerResponse current = responses[i];
        // for(int j = 0; j < WORK_UNIT_SIZE; j++){
        //     // printf("%lu is prime %d\n", j+current.start, (int)current.responses[j]);
        //     //printf("%d\n",j);
        // }
        printf("block: %lu done\n", current.start);
    }
}

void manager(int numWorkers)
{
    struct workerResponse *recvBuffer = (struct workerResponse *)alloca(sizeof(struct workerResponse) * numWorkers);
    uint64_t *sendbuffer = (uint64_t *)alloca(sizeof(uint64_t) * numWorkers);
    for (int i = 0; i < END; i += WORK_UNIT_SIZE * numWorkers)
    {
        for (int w = 0; w < numWorkers; w++)
        {
            sendbuffer[w] = i + WORK_UNIT_SIZE * w;
        }
        uint64_t batch;
        MPI_Scatter(sendbuffer, 1, MPI_LONG, &batch, 1, MPI_LONG, 0, MPI_COMM_WORLD);
        struct workerResponse response = handleWorkload(batch);
        MPI_Gather(&response, sizeof(struct workerResponse), MPI_BYTE, recvBuffer, sizeof(struct workerResponse), MPI_BYTE, 0, MPI_COMM_WORLD);
        printOutput(recvBuffer, numWorkers);
    }
}

int main(int argc, char **argv)
{
    MPI_Init(NULL, NULL);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    char processor_name[MPI_MAX_PROCESSOR_NAME];
    if (world_rank == 0)
    {
        manager(world_size);
    }
    else
    {
        worker();
    }
    MPI_Finalize();
}