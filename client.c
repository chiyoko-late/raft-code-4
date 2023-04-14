// ./ client 127.0.0.1
#include "appendentries.h"
#include "debug.h"
// #define NULL __DARWIN_NULL

int main(int argc, char *argv[])
{
    int l_port = 1111;
    int sock;
    struct sockaddr_in addr;
    char *l_ip;
    l_ip = argv[1];

    /* ソケットを作成 */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket error ");
        exit(0);
    }
    memset(&addr, 0, sizeof(struct sockaddr_in));

    /* サーバーのIPアドレスとポートの情報を設定 */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(l_port);
    addr.sin_addr.s_addr = inet_addr(l_ip);
    const size_t addr_size = sizeof(addr);

    int opt = 1;
    // ポートが解放されない場合, SO_REUSEADDRを使う
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) == -1)
    {
        perror("setsockopt error ");
        close(sock);
        exit(0);
    }

    // /* leaderとconnect */
    printf("Start connect...\n");
    int k = 0;
    connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    my_recv(sock, &k, sizeof(int) * 1);
    printf("%d\n", k);
    k = 9;
    my_send(sock, &k, sizeof(int) * 1);
    printf("Finish connect with leader!\n");

    char *str = malloc(sizeof(char) * STRING_MAX);
    int result;

    // 時間記録用ファイル
    FILE *timerec;
    timerec = fopen("client_time.txt", "w+");
    if (timerec == NULL)
    {
        printf("cannot open file\n");
        exit(1);
    }

    int i = 1;
    printf("entry -> ");
    scanf("%s", str);
    /* 接続済のソケットでデータのやり取り */
    while (i < 10)
    {

        printf("%d回目\n", i);
        /* leaderに送る */
        clock_gettime(CLOCK_MONOTONIC, &ts1);
        for (int i = 1; i < ONCE_SEND_ENTRIES; i++)
        {
            my_send(sock, str, sizeof(char) * STRING_MAX);
        }
        my_recv(sock, &result, sizeof(int));
        if (result == 1)
        {
            printf("commit\n");
        }
        else
        {
            printf("There is something error!!\n");
        }
        clock_gettime(CLOCK_MONOTONIC, &ts2);
        t = ts2.tv_sec - ts1.tv_sec + (ts2.tv_nsec - ts1.tv_nsec) / 1e9;
        fprintf(timerec, "%.4f\n", t);
        // fwrite(&t, sizeof(double), 1, timerec);
        printf("%.4f\n", t);
        i++;
    }

    exit(0);
}