/* leader log 127.0.0.1 127.0.0.1 127.0.0.1 127.0.0.1 */

#include "appendentries.h"
#include "debug.h"

int AppendEntriesRPC(
    int connectserver_num,
    int sock[],
    struct AppendEntriesRPC_Argument *AERPC_A,
    struct AppendEntriesRPC_Result *AERPC_R,
    struct Leader_VolatileState *L_VS,
    struct AllServer_VolatileState *AS_VS,
    struct AllServer_PersistentState *AS_PS)
{
    int replicatelog_num = 0;

    /* AERPC_Aの設定 */

    AERPC_A->term = AS_PS->currentTerm;
    AERPC_A->prevLogIndex = AS_PS->log.index;
    AERPC_A->prevLogTerm = AS_PS->log.term;
    AERPC_A->leaderCommit = AS_VS->commitIndex;
    AERPC_A->entries = AS_PS->log.entries;

    // for (int i = 1; i < ONCE_SEND_ENTRIES; i++)
    // {
    //     // strcpy(AERPC_A->entries[i - 1].entry, AS_PS->log[L_VS->nextIndex[0] + (i - 1)].entry);
    // }

    // output_AERPC_A(AERPC_A);

    for (int i = 1; i <= connectserver_num; i++)
    {
        my_send(sock[i], AERPC_A, sizeof(struct AppendEntriesRPC_Argument));
    }
    printf("finish sending\n\n");

    for (int i = 1; i <= connectserver_num; i++)
    {
        my_recv(sock[i], AERPC_R, sizeof(struct AppendEntriesRPC_Result));
    }

    for (int i = 1; i <= connectserver_num; i++)
    {
        // output_AERPC_R(AERPC_R);
        printf("send to server %d\n", i);

        // • If successful: update nextIndex and matchIndex for follower.
        if (AERPC_R->success == 1)
        {
            L_VS->nextIndex[i] += 1;
            L_VS->matchIndex[i] += 1;

            replicatelog_num += 1;

            printf("Success:%d\n", i);
        }
        // • If AppendEntries fails because of log inconsistency: decrement nextIndex and retry.
        else
        {
            printf("failure0\n");
            // L_VS->nextIndex[i] -= (ONCE_SEND_ENTRIES - 1);
            // AERPC_A->prevLogIndex -= (ONCE_SEND_ENTRIES - 1);
            // AppendEntriesRPC(connectserver_num, sock, AERPC_A, AERPC_R, L_VS, AS_VS, AS_PS);
            printf("failure1\n");
            exit(1);
        }
    }

    return replicatelog_num;
}

int main(int argc, char *argv[])
{
    printf("made logfile\n");
    make_logfile(argv[1]);

    int port[5];
    port[0] = 1111;
    port[1] = 1234;
    port[2] = 2345;
    port[3] = 3456;
    port[4] = 4567;

    char *ip[4];
    ip[0] = argv[2];
    ip[1] = argv[3];
    ip[2] = argv[4];
    ip[3] = argv[5];

    // ソケット作成
    int sock[5];
    struct sockaddr_in addr[5];
    for (int i = 0; i < 5; i++)
    {
        sock[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (sock[i] < 0)
        {
            perror("socket error ");
            exit(0);
        }
        memset(&addr[i], 0, sizeof(struct sockaddr_in));
    }
    /* サーバーのIPアドレスとポートの情報を設定 */
    // client
    addr[0].sin_family = AF_INET;
    addr[0].sin_port = htons(port[0]);
    addr[0].sin_addr.s_addr = htonl(INADDR_ANY);
    const size_t addr_size = sizeof(addr[0]);

    // follower
    for (int i = 1; i < 5; i++)
    {
        addr[i].sin_family = AF_INET;
        addr[i].sin_port = htons(port[i]);
        addr[i].sin_addr.s_addr = inet_addr(ip[i - 1]);
        const size_t addr_size = sizeof(addr);
    }

    int opt = 1;
    // ポートが解放されない場合, SO_REUSEADDRを使う
    for (int i = 0; i < 5; i++)
    {
        if (setsockopt(sock[i], SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) == -1)
        {
            perror("setsockopt error ");
            close(sock[i]);
            exit(0);
        }
    }

    if (bind(sock[0], (struct sockaddr *)&addr[0], addr_size) == -1)
    {
        perror("bind error ");
        close(sock[0]);
        exit(0);
    }
    printf("bind port=%d\n", port[0]);

    // クライアントのコネクション待ち状態は最大10
    if (listen(sock[0], 10) == -1)
    {
        perror("listen error ");
        close(sock[0]);
        exit(0);
    }
    printf("listen success!\n");

    // /* followerとconnect */
    int connectserver_num = 0;
    printf("Start connect...\n");
    for (int i = 1; i < 5; i++)
    {
        int k = 0;
        connect(sock[i], (struct sockaddr *)&addr[i], sizeof(struct sockaddr_in));
        my_recv(sock[i], &k, sizeof(int) * 1);
        if (k == 1)
        {
            connectserver_num += k;
        }
    }
    printf("Finish connect with %dservers!\n", connectserver_num);

    int last_id = 0;
    int sock_client = 0;
    char *buffer = (char *)malloc(32 * 1024 * 1024);

ACCEPT:
    //     // 接続が切れた場合, acceptからやり直す
    printf("last_id=%d\n", last_id);
    int old_sock_client = sock_client;
    struct sockaddr_in accept_addr = {
        0,
    };
    socklen_t accpet_addr_size = sizeof(accept_addr);
    sock_client = accept(sock[0], (struct sockaddr *)&accept_addr, &accpet_addr_size);
    printf("sock_client=%d\n", sock_client);
    if (sock_client == 0 || sock_client < 0)
    {
        perror("accept error ");
        exit(0);
    }
    printf("succeed in conecting with client!\n");
    // ここで一回送って接続数確認に使う
    int k = 7;
    my_send(sock_client, &k, sizeof(int) * 1);
    if (old_sock_client > 0)
    {
        close(old_sock_client);
    }
    my_recv(sock_client, &k, sizeof(int) * 1);
    printf("%d\n", k);

    struct AppendEntriesRPC_Argument *AERPC_A = new struct AppendEntriesRPC_Argument;
    struct AppendEntriesRPC_Result *AERPC_R = new struct AppendEntriesRPC_Result;
    struct AllServer_PersistentState *AS_PS = new struct AllServer_PersistentState;
    struct AllServer_VolatileState *AS_VS = new struct AllServer_VolatileState;
    struct Leader_VolatileState *L_VS = new struct Leader_VolatileState;

    // 初期設定
    AERPC_A->term = 1;
    AERPC_A->leaderID = 1;
    AERPC_A->prevLogIndex = 0;
    AERPC_A->prevLogTerm = 0;
    AERPC_A->leaderCommit = 0;
    AS_PS->currentTerm = 1;
    AS_PS->voteFor = 0;
    AS_PS->log.index = 0;
    AS_PS->log.term = 0;

    AS_VS->commitIndex = 0;
    AS_VS->LastAppliedIndex = 0;

    for (int i = 0; i < 4; i++)
    {
        L_VS->nextIndex[i] = 1;
        L_VS->matchIndex[i] = 0;
    }

    // char *str = malloc(sizeof(char) * STRING_MAX);
    // std::vector<char> str(STRING_MAX);
    int replicatelog_num;

    /* log記述用のファイル名 */
    make_logfile(argv[1]);

    // 時間記録用ファイル
    FILE *timerec;
    timerec = fopen("timerecord.txt", "w+");

    if (timerec == NULL)
    {
        printf("cannot open file\n");
        exit(1);
    }

    /* 接続済のソケットでデータのやり取り */
    for (int i = 1; i < (ALL_ACCEPTED_ENTRIES / ENTRY_NUM); i++)
    {
        printf("replicate start!\n");
        // clock_gettime(CLOCK_MONOTONIC, &ts1);
        for (int k = 0; k < ENTRY_NUM; k++)
        {
            // clientから受け取り
            my_recv(sock_client, &AS_PS->log.entries[k], sizeof(char) * STRING);
        }
        AS_PS->log.term = AS_PS->currentTerm;
        AS_PS->log.index = AS_PS->log.index + 1;

        // write_log(i, AS_PS);
        // read_log(i);

        /* AS_VSの更新 */
        AS_VS->LastAppliedIndex += 1;

        replicatelog_num = AppendEntriesRPC(connectserver_num, sock, AERPC_A, AERPC_R, L_VS, AS_VS, AS_PS);

        int result = 0;
        if (replicatelog_num > (connectserver_num + 1) / 2)
        {
            printf("majority of servers replicated\n");
            AS_VS->commitIndex += 1;
            result = 1;
        }
        my_send(sock_client, &result, sizeof(int) * 1);

        // clock_gettime(CLOCK_MONOTONIC, &ts2);
        t = ts2.tv_sec - ts1.tv_sec + (ts2.tv_nsec - ts1.tv_nsec) / 1e9;

        // fprintf(timerec, "%.4f\n", t);
        // printf("%.4f\n", t);
    }

    while (true)
    {
        ae_req_t ae_req;

        if (my_recv(sock_client, &ae_req, sizeof(ae_req_t)))
            goto ACCEPT;
        if (my_recv(sock_client, buffer, ae_req.size))
            goto ACCEPT;

        ae_res_t ae_res;
        ae_res.id = last_id = ae_req.id;
        ae_res.status = 0;

        if (my_send(sock_client, &ae_res, sizeof(ae_res_t)))
            goto ACCEPT;
    }
    printf("logは上限です");
    return 0;
}