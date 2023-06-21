#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include "my_sock.h"

#define SERVER_ADDR "0.0.0.0"
#define STRING (10LL)
#define ALL_ACCEPTED_ENTRIES (1000L)
#define ENTRY_NUM (100LL)

// using namespace std;

// int STRING = 100;
// int ALL_ACCEPTED_ENTRIES = 10000;
// int ENTRY_NUM = 10000;

uint64_t c1,
    c2;
struct timespec ts1, ts2;
double t;

static uint64_t rdtscp()
{
    uint64_t rax;
    uint64_t rdx;
    uint32_t aux;
    asm volatile("rdtscp"
                 : "=a"(rax), "=d"(rdx), "=c"(aux)::);
    return (rdx << 32) | rax;
}

struct append_entry
{
    char entry[STRING];
};

struct AppendEntriesRPC_Argument
{
    int term;
    int leaderID;
    int prevLogIndex;
    int prevLogTerm;
    int leaderCommit;
    std::vector<append_entry> entries = std::vector<append_entry>(ENTRY_NUM);
};

struct AppendEntriesRPC_Result
{
    int term;
    bool success;
};

struct LOG
{
    int term;
    int index;
    std::vector<append_entry> entries = std::vector<append_entry>(ENTRY_NUM);
};

struct AllServer_PersistentState
{
    int currentTerm;
    int voteFor;
    LOG log;
};

struct AllServer_VolatileState
{
    int commitIndex;
    int LastAppliedIndex;
};

struct Leader_VolatileState
{
    std::vector<int> nextIndex = std::vector<int>(4);
    std::vector<int> matchIndex = std::vector<int>(4);
};

char *filename;
char *logfilename;

int fdo;
// // logfile初期化
void make_logfile(char *name)
{
    filename = name;
    fdo = open(filename, (O_CREAT | O_RDWR), 0644);
    if (fdo == -1)
    {
        printf("file open error\n");
        exit(1);
    }
    printf("fdo: %d\n", fdo);
    return;
}

void read_prev(int prevLogIndex, int *read_index, int *read_term)
{
    int read_log[2];
    lseek(fdo, sizeof(struct LOG) * (prevLogIndex - 1), SEEK_SET);
    read(fdo, read_log, sizeof(int) * 2);
    *read_term = read_log[0];
    *read_index = read_log[1];
    return;
}

void write_log(
    int prevLogIndex, struct LOG *log)
{
    lseek(fdo, sizeof(struct LOG) * prevLogIndex, SEEK_SET);
    write(fdo, &log, sizeof(struct LOG));
    fsync(fdo);
    // 後ろを削除
    return;
}

// void read_log(
//     // char filename[],
//     int i)
// {
//     struct AllServer_PersistentState *AS_PS = new struct AllServer_PersistentState;

//     lseek(fdo, sizeof(struct AllServer_PersistentState) * (i - 1), SEEK_SET);

//     read(fdo, AS_PS, sizeof(struct AllServer_PersistentState));

//     for (int num = 1; num < ONCE_SEND_ENTRIES; num++)
//     {
//         printf("[logfile] AS_PS->currentTerm = %d\n", AS_PS->currentTerm);
//         printf("[logfile] AS_PS->voteFor = %d\n", AS_PS->voteFor);
//         printf("[logfile] AS_PS->log[%d].term = %d\n", (i - 1) * (ONCE_SEND_ENTRIES - 1) + num, AS_PS->log[(i - 1) * (ONCE_SEND_ENTRIES - 1) + num].term);
//         // printf("[logfile] AS_PS->log[%d].entry = %s\n\n", (i - 1) * (ONCE_SEND_ENTRIES - 1) + num, AS_PS->log[(i - 1) * (ONCE_SEND_ENTRIES - 1) + num].entry);
//         std::string output_string(AS_PS->log[(i - 1) * (ONCE_SEND_ENTRIES - 1) + num].entry.begin(), AS_PS->log[(i - 1) * (ONCE_SEND_ENTRIES - 1) + num].entry.end());

//         printf("[logfile] AS_PS->log[%d].entry = %s\n\n", (i - 1) * (ONCE_SEND_ENTRIES - 1) + num, output_string.c_str());
//         // cout << "[logfile] AS_PS->log[ " << (i - 1) * (ONCE_SEND_ENTRIES - 1) + num << "].entry = " << AS_PS->log[(i - 1) * (ONCE_SEND_ENTRIES - 1) + num].entry << endl
//         //      << endl;
//     }
//     return;
// }

// void output_AERPC_A(struct AppendEntriesRPC_Argument *p)
// {
//     printf("---appendEntriesRPC---\n");
//     printf("term: %d\n", p->term);
//     for (int i = 1; i < ONCE_SEND_ENTRIES; i++)
//     {
//         std::string output_string(p->entries[i - 1].entry.begin(), p->entries[i - 1].entry.end());
//         printf("entry: %s\n", output_string.c_str());
//         // cout << "entry:" << p->entries[i - 1].entry << endl;
//     }
//     printf("prevLogIndex: %d-%d\n", p->prevLogIndex[0], p->prevLogIndex[ONCE_SEND_ENTRIES - 1]);
//     printf("prevLogTerm: %d-%d\n", p->prevLogTerm[0], p->prevLogIndex[ONCE_SEND_ENTRIES - 1]);
//     printf("LeaderCommitIndex: %d\n", p->leaderCommit);
//     printf("----------------------\n");
//     return;
// }

// void output_AERPC_R(struct AppendEntriesRPC_Result *p)
// {
//     printf("**AERPC_R**\n");
//     printf("term: %d\n", p->term);
//     printf("bool: %d\n", p->success);
//     printf("***********\n");
// }