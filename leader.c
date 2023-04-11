#include "appendentries.h"
#include "debug.h"

FILE *timerec;

int main(int argc, char *argv[])
{
    printf("made logfile\n");
    make_logfile(argv[2]);

    int port[5];
    port[0] = atoi(argv[1]);
    port[1] = 1234;
    port[2] = 2345;
    port[3] = 3456;
    port[4] = 4567;

    int sock[5];
    struct sockaddr_in addr[5];

    char *ip[4];
    ip[0] = argv[3];
    ip[1] = argv[4];
    ip[2] = argv[5];
    ip[3] = argv[6];

    for (int i = 0; i < 5; i++)
    {
        sock[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (sock[i] < 0)
        {
            perror("socket error ");
            exit(0);
        }
    }
    if (argv[1] == 0)
    {
        printf("Usage : \n $> %s [port number]\n", argv[0]);
        exit(0);
    }

    for (int i = 0; i < 5; i++)
    {
        memset(&addr[i], 0, sizeof(struct sockaddr_in));
    }
    addr[0].sin_family = AF_INET;
    addr[0].sin_port = htons(port[0]);
    addr[0].sin_addr.s_addr = htonl(INADDR_ANY);
    const size_t addr_size = sizeof(addr);

    for (int i = 1; i < 5; i++)
    {
        addr[i].sin_family = AF_INET;
        addr[i].sin_port = htons(port[i]);
        addr[i].sin_addr.s_addr = inet_addr(ip[i - 1]);
        const size_t addr_size = sizeof(addr);
    }

    int opt = 1;
    // ポートが解放されない場合, SO_REUSEADDRを使う
    if (setsockopt(sock[0], SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) == -1)
    {
        perror("setsockopt error ");
        close(sock[0]);
        exit(0);
    }
    if (bind(sock[0], (struct sockaddr *)&addr, addr_size) == -1)
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

    struct AppendEntriesRPC_Argument *AERPC_A = malloc(sizeof(struct AppendEntriesRPC_Argument));
    struct AppendEntriesRPC_Result *AERPC_R = malloc(sizeof(struct AppendEntriesRPC_Result));
    struct AllServer_PersistentState *AS_PS = malloc(sizeof(struct AllServer_PersistentState));
    struct AllServer_VolatileState *AS_VS = malloc(sizeof(struct AllServer_VolatileState));
    struct Leader_VolatileState *L_VS = malloc(sizeof(struct Leader_VolatileState));

    // 初期設定
    AERPC_A->term = 1;
    AERPC_A->leaderID = 1;
    AERPC_A->prevLogIndex = 0;
    AERPC_A->prevLogTerm = 0;
    AERPC_A->leaderCommit = 0;

    AS_PS->currentTerm = 1;
    AS_PS->log[0].term = 0;
    for (int i = 0; i < ALL_ACCEPTED_ENTRIES; i++)
    {
        memset(AS_PS->log[i].entry, 0, sizeof(char) * STRING_MAX);
    }
    AS_PS->voteFor = 0;

    AS_VS->commitIndex = 0;
    AS_VS->LastAppliedIndex = 0;

    for (int i = 0; i < 4; i++)
    {
        L_VS->nextIndex[i] = 1; // (AERPC_A->leaderCommit=0) + 1
        L_VS->matchIndex[i] = 0;
    }

    char *str = malloc(sizeof(char) * STRING_MAX);
    int replicatelog_num;

    // 時間記録用ファイル
    FILE *timerec;
    timerec = fopen("timerecord.txt", "w+");
    if (timerec == NULL)
    {
        printf("cannot open file\n");
        exit(1);
    }

    int last_id = 0;
    int sock_client = 0;
    char *buffer = (char *)malloc(32 * 1024 * 1024);

ACCEPT:
    // 接続が切れた場合, acceptからやり直す
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
    printf("accept success!\n");
    // ここで一回送って接続数確認に使う
    int k = 1;
    my_send(sock_client, &k, sizeof(int) * 1);
    if (old_sock_client > 0)
    {
        close(old_sock_client);
    }

    AS_PS->currentTerm += 1;

    printf("4444\n");

    while (1)
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

    return 0;
}