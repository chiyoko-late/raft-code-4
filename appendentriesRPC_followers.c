#include "appendentries.h"
#include "debug.h"

FILE *timerec;

int consistency_check(
    struct AppendEntriesRPC_Argument *rpc,
    struct AllServer_PersistentState *as_ps,
    struct AllServer_VolatileState *as_vs)
{

    // 1. Reply false if term<currentTerm
    if (rpc->term < as_ps->currentTerm)
    {
        printf("reject1,%d,%d\n", rpc->term, as_ps->currentTerm);
        return false;
    }

    // 2. Reply false if log doesn’t contain an entry at prevLogIndex whose term matches prevLogTerm
    if (rpc->prevLogTerm != as_ps->log[rpc->prevLogIndex].term)
    {
        printf("reject2,%d,%d\n", rpc->prevLogTerm, as_ps->log[rpc->prevLogIndex].term);
        return false;
    }

    // 3. If an existing entry conflicts with a new one(same index but different terms), delete the existing entry and all that follow it
    if (strcmp(as_ps->log[rpc->prevLogIndex + 1].entry, as_ps->log[0].entry) == -1 && as_ps->log[rpc->prevLogIndex + 1].term != rpc->term)
    {
        printf("reject3.0\n");
        int j = strcmp(as_ps->log[rpc->prevLogIndex + 1].entry, as_ps->log[0].entry);

        while (j == 0)
        {
            for (int i = rpc->prevLogIndex + 1; i < ONCE_SEND_ENTRIES; i++)
            {
                memset(as_ps->log[rpc->prevLogIndex + 1].entry, 0, sizeof(char) * STRING_MAX);
            }
        }
        printf("reject3\n");
        return false;
    }

    // printf("rpc->prevLogIndex = %d\n", rpc->prevLogIndex);

    // 4. Append any new entries not already in the log
    for (int num = 1; num < ONCE_SEND_ENTRIES; num++)
    {
        // as_ps->log[rpc->prevLogIndex + 1].term = rpc->term;
        as_ps->log[rpc->prevLogIndex + num].term = rpc->term;
        strcpy(as_ps->log[rpc->prevLogIndex + num].entry, rpc->entries[num - 1].entry);
    }

    as_vs->LastAppliedIndex = rpc->prevLogIndex + ONCE_SEND_ENTRIES;
    /* log記述 */
    write_log(rpc->prevLogIndex / (ONCE_SEND_ENTRIES - 1) + 1, as_ps);
    // read_log(rpc->prevLogIndex / (ONCE_SEND_ENTRIES - 1) + 1);

    // 5. If leaderCmakeommit> commitIndex, set commitIndex = min(leaderCommit, index of last new entry)
    if (rpc->leaderCommit > as_vs->commitIndex)
    {
        as_vs->commitIndex = rpc->leaderCommit;
    }
    // printf("write log\n");

    return true;
};

int transfer(
    int sock,
    struct AppendEntriesRPC_Argument *AERPC_A,
    struct AppendEntriesRPC_Result *AERPC_R,
    struct AllServer_PersistentState *AS_PS,
    struct AllServer_VolatileState *AS_VS)
{

    /* リーダーから文字列を受信 */
    printf("try-recv\n");
    my_recv(sock, AERPC_A, sizeof(struct AppendEntriesRPC_Argument));
    printf("Receiving AppendEntriesRPC is success.\n");
    //     // recv(sock, AERPC_A->entries[num - 1], sizeof(char) * MAX, MSG_WAITALL);
    //     my_recv(sock, AERPC_A->entries[num - 1].entry, sizeof(char) * STRING_MAX);
    // }
    // output_AERPC_A(AERPC_A);

    // consistency check
    clock_gettime(CLOCK_MONOTONIC, &ts1);
    AERPC_R->success = consistency_check(AERPC_A, AS_PS, AS_VS);
    clock_gettime(CLOCK_MONOTONIC, &ts2);
    AERPC_R->term = AS_PS->currentTerm;
    if (AERPC_R->success == false)
    {
        printf("AERPC_R->success == false\n");
        exit(1);
    }

    // lerderに返答
    my_send(sock, AERPC_R, sizeof(struct AppendEntriesRPC_Result));

    /* 受信した文字列を表示 */
    printf("replied to leader. Send AERPC_R\n");

    t = ts2.tv_sec - ts1.tv_sec + (ts2.tv_nsec - ts1.tv_nsec) / 1e9;
    // fprintf(timerec, "%.4f\n", t);
    // printf("CCCheck TIME: %.4f\n", t);

    return 0;
}

int main(int argc, char *argv[])
{

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket error ");
        exit(0);
    }
    if (argv[1] == 0)
    {
        printf("Usage : \n $> %s [port number]\n", argv[0]);
        exit(0);
    }
    int port = atoi(argv[1]);
    struct sockaddr_in addr = {
        0,
    };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    const size_t addr_size = sizeof(addr);

    int opt = 1;
    // ポートが解放されない場合, SO_REUSEADDRを使う
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) == -1)
    {
        perror("setsockopt error ");
        close(sock);
        exit(0);
    }
    if (bind(sock, (struct sockaddr *)&addr, addr_size) == -1)
    {
        perror("bind error ");
        close(sock);
        exit(0);
    }
    printf("bind port=%d\n", port);

    // クライアントのコネクション待ち状態は最大10
    if (listen(sock, 10) == -1)
    {
        perror("listen error ");
        close(sock);
        exit(0);
    }
    printf("listen success!\n");

    struct AppendEntriesRPC_Argument *AERPC_A = malloc(sizeof(struct AppendEntriesRPC_Argument));
    struct AppendEntriesRPC_Result *AERPC_R = malloc(sizeof(struct AppendEntriesRPC_Result));
    struct AllServer_PersistentState *AS_PS = malloc(sizeof(struct AllServer_PersistentState));
    struct AllServer_VolatileState *AS_VS = malloc(sizeof(struct AllServer_VolatileState));
    printf("made logfile\n");

    make_logfile(argv[2]);

    // 時間記録用ファイル
    timerec = fopen("cctime.txt", "w+");
    if (timerec == NULL)
    {
        printf("cannot open file\n");
        exit(1);
    }

    AS_PS->currentTerm = 0;
    AS_PS->log[0].term = 0;
    for (int i = 0; i < ALL_ACCEPTED_ENTRIES; i++)
    {
        memset(AS_PS->log[i].entry, 0, sizeof(char) * STRING_MAX);
    }
    AS_VS->commitIndex = 0;
    AS_VS->LastAppliedIndex = 0;

    int last_id = 0;
    int sock_leader = 0;
    char *buffer = (char *)malloc(32 * 1024 * 1024);

ACCEPT:
    // 接続が切れた場合, acceptからやり直す
    printf("last_id=%d\n", last_id);
    int old_sock_client = sock_leader;
    struct sockaddr_in accept_addr = {
        0,
    };
    socklen_t accpet_addr_size = sizeof(accept_addr);
    sock_leader = accept(sock, (struct sockaddr *)&accept_addr, &accpet_addr_size);
    printf("sock_client=%d\n", sock_leader);
    if (sock_leader == 0 || sock_leader < 0)
    {
        perror("accept error ");
        exit(0);
    }
    printf("accept success!\n");
    // ここで一回送って接続数確認に使う
    int k = 1;
    my_send(sock_leader, &k, sizeof(int) * 1);
    if (old_sock_client > 0)
    {
        close(old_sock_client);
    }

    AS_PS->currentTerm += 1;

    for (int i = 1; i < (ALL_ACCEPTED_ENTRIES / ONCE_SEND_ENTRIES); i++)
    {
        transfer(sock_leader, AERPC_A, AERPC_R, AS_PS, AS_VS);
    }

    while (true)
    {
        ae_req_t ae_req;

        if (my_recv(sock_leader, &ae_req, sizeof(ae_req_t)))
            goto ACCEPT;
        if (my_recv(sock_leader, buffer, ae_req.size))
            goto ACCEPT;

        ae_res_t ae_res;
        ae_res.id = last_id = ae_req.id;
        ae_res.status = 0;

        if (my_send(sock_leader, &ae_res, sizeof(ae_res_t)))
            goto ACCEPT;
    }

    return 0;
}