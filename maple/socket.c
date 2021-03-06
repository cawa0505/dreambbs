#include "bbs.h"

int
Get_Socket(  /* site for hostname, sock for port & socket */
    const char *site,
    int *sock)
{
    struct addrinfo hints = {0};
    struct addrinfo *hosts;
    char port_str[12];

    /* Getting remote-site data */

    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV;
    sprintf(port_str, "%d", *sock);
    if (getaddrinfo(site, port_str, &hints, &hosts))
        return -1;

    /* Getting a socket */

    for (struct addrinfo *host = hosts; host; host = host->ai_next)
    {
        if ((*sock = socket(host->ai_family, host->ai_socktype, 0)) < 0)
        {
            continue;
        }

#ifdef  SET_ALARM
        signal(SIGALRM, timeout);
        alarm(SET_ALARM);
#endif

        /* perform connecting */

        if (connect(*sock, host->ai_addr, host->ai_addrlen) < 0)
        {
            close(*sock);

#ifdef  SET_ALARM
            init_alarm();
#endif

            continue;
        }

        /* Success */

#ifdef  SET_ALARM
        init_alarm();
#endif

        freeaddrinfo(hosts);
        return 0;
    }

#ifdef  SET_ALARM
    init_alarm();
#endif

    freeaddrinfo(hosts);
    return -1;
}



int
POP3_Check(
    const char *site, const char *account, const char *passwd)
{
    FILE *fsock = NULL;
    int sock=110, old_sock GCC_UNUSED = 0;
    char buf[512];

    if (Get_Socket(site, &sock))
        sock = 1;
    else
    if (!(fsock = fdopen(sock, "r+")))
    {
        close(sock);
        sock = 1;
    }
    else
    {
        old_sock = sock;
        sock = 2;
    }

    while (sock < 6)
    {
        switch (sock)
        {
            case 1:             /* Open Socket Fail */
                prints("\n傳回錯誤值 [1]，請重試幾次看看\n");
                refresh();
                return sock;

            case 2:             /* Welcome Message */
                fgets(buf, 512, fsock);
                break;

            case 3:             /* Verify Account */
                fprintf(fsock, "user %s\r\n", account);
                fflush(fsock);
                fgets(buf, 512, fsock);
                break;

            case 4:             /* Verify Password */
                fprintf(fsock, "pass %s\r\n", passwd);
                fflush(fsock);
                fgets(buf, 512, fsock);
                sock = -1;
                break;

            case 0:             /* Successful Verification */
            case 5:             /* Quit */
                fprintf(fsock, "quit\r\n");
                fclose(fsock);
                return sock;
        }

        if (strncmp(buf, "+OK", 3) || strstr(buf, ".bbs"))
        {
            prints("遠端系統傳回錯誤訊息如下：(如不明白訊息意思，請將訊息告知站長)\n");
            prints("%s\n", buf);
            refresh();
/*          log_usies("POP3-MSG", buf, FN_VERIFY_LOG); */
            sock = 5;
        }
        else
            sock++;
    }
    return 1;
}


int
Ext_POP3_Check(
    const char *site, const char *account, const char *passwd)
{
    FILE *fsock = NULL;
    int sock=110, step=1;

    char buf[512];


    fd_set rd;
    struct timeval to;
    int nfds;
    time_t now;
    struct tm *p;


    if (!Get_Socket(site, &sock))
    {
        if (!(fsock = fdopen(sock, "r+")))
        {
            close(sock);
            return 1;
        }
        step = 1;

        while (step < 9)
        {
            FD_ZERO(&rd);
            FD_SET(sock, &rd);
            to.tv_sec = 0;
            to.tv_usec = 0;
            nfds = sock;

            switch (step)
            {
            case 1:
                while (1)
                {
                    fgets(buf, 512, fsock);
                    if (!strncmp(buf, "+OK", 3))
                    {
                        if (step == 2)
                        {
                            pmsg("不受信任的 POP3 主機，請使用 Email 回信認證！");
                            step = 8;
                            logitfile("tmp/visor.log", site, "stat 1");
                            break;
                        }
                        step = 2;
                    }
                    else
                    {
                        pmsg("不支援 POP3 主機，請使用 Email 回信認證！");
                        step = 9;
                        break;
                    }

                    FD_ZERO(&rd);
                    FD_SET(sock, &rd);
                    nfds = select(nfds+1, &rd, NULL, NULL, &to);
                    if (nfds == 0)
                        break;
                }
                break;
            case 2:
                fprintf(fsock, "pass !@#$%%^&*(\r\n");
                fflush(fsock);
                while (1)
                {
                    fgets(buf, 512, fsock);
                    if (!strncmp(buf, "-ERR", 4))
                    {
                        step = 3;
                    }
                    else if (!strncmp(buf, "+OK", 3))
                    {
                        pmsg("不受信任的 POP3 主機，請使用 Email 回信認證！");
                        step = 8;
                        logitfile("tmp/visor.log", site, "stat 2");
                        break;
                    }

                    FD_ZERO(&rd);
                    FD_SET(sock, &rd);
                    nfds = select(nfds+1, &rd, NULL, NULL, &to);
                    if (nfds == 0)
                        break;
                }
                break;
            case 3:
                fprintf(fsock, "noop\r\n");
                fflush(fsock);
                while (1)
                {
                    fgets(buf, 512, fsock);
                    if (!strncmp(buf, "-ERR", 4))
                    {
                        step = 4;
                    }
                    else if (!strncmp(buf, "+OK", 3))
                    {
                        pmsg("不受信任的 POP3 主機，請使用 Email 回信認證！");
                        step = 8;
                        logitfile("tmp/visor.log", site, "stat 3");
                        break;
                    }

                    FD_ZERO(&rd);
                    FD_SET(sock, &rd);
                    nfds = select(nfds+1, &rd, NULL, NULL, &to);
                    if (nfds == 0)
                        break;
                }
                break;
            case 4:
                fprintf(fsock, "user %s\r\n", account);
                fflush(fsock);
                while (1)
                {
                    fgets(buf, 512, fsock);
                    if (!strncmp(buf, "-ERR", 4))
                    {
                        pmsg("無此郵件帳號，請使用 Email 回信認證！");
                        step = 9;
                        break;
                    }
                    else if (!strncmp(buf, "+OK", 3) && step == 4)
                    {
                        step = 5;
                    }
                    else
                    {
                        pmsg("不受信任的 POP3 主機，請使用 Email 回信認證！");
                        step = 8;
                        logitfile("tmp/visor.log", site, "stat 4");
                        break;
                    }

                    FD_ZERO(&rd);
                    FD_SET(sock, &rd);
                    nfds = select(nfds+1, &rd, NULL, NULL, &to);
                    if (nfds == 0)
                        break;
                }
                break;
            case 5:
                fprintf(fsock, "pass %s\r\n", passwd);
                fflush(fsock);
                while (1)
                {
                    fgets(buf, 512, fsock);
                    if (!strncmp(buf, "-ERR", 4))
                    {
                        pmsg("帳號密碼錯誤，請重新認證！");
                        step = 9;
                        break;
                    }
                    else if (!strncmp(buf, "+OK", 3) && step == 5)
                    {
                        step = 6;
                    }
                    else
                    {
                        pmsg("不受信任的 POP3 主機，請使用 Email 回信認證！");
                        step = 8;
                        logitfile("tmp/visor.log", site, "stat 5");
                        break;
                    }

                    FD_ZERO(&rd);
                    FD_SET(sock, &rd);
                    nfds = select(nfds+1, &rd, NULL, NULL, &to);
                    if (nfds == 0)
                        break;
                }
                break;
            case 6:
                fprintf(fsock, "noop\r\n");
                fflush(fsock);

                while (1)
                {
                    fgets(buf, 512, fsock);
                    if (!strncmp(buf, "+OK", 3) && step == 6)
                    {
                        step = 7;
                    }
                    else
                    {
                        pmsg("不受信任的 POP3 主機，請使用 Email 回信認證！");
                        step = 8;
                        logitfile("tmp/visor.log", site, "stat 6");
                        break;
                    }

                    FD_ZERO(&rd);
                    FD_SET(sock, &rd);
                    nfds = select(nfds+1, &rd, NULL, NULL, &to);
                    if (nfds == 0)
                        break;
                }
                break;
            case 7:
                fprintf(fsock, "stat\r\n");
                fflush(fsock);
                while (1)
                {
                    fgets(buf, 512, fsock);
                    if (!strncmp(buf, "+OK", 3) && step == 7)
                    {
                        step = 0;
                    }
                    else
                    {
                        pmsg("不受信任的 POP3 主機，請使用 Email 回信認證！");
                        step = 8;
                        logitfile("tmp/visor.log", site, "stat 7");
                        break;
                    }

                    FD_ZERO(&rd);
                    FD_SET(sock, &rd);
                    nfds = select(nfds+1, &rd, NULL, NULL, &to);
                    if (nfds == 0)
                        break;
                }
                break;
            case 8:
                time(&now);

                p = localtime(&now);
                sprintf(buf, "%s # %02d/%02d/%02d %02d:%02d 不受信任的主機\n",
                    site, p->tm_year % 100, p->tm_mon + 1, p->tm_mday,
                    p->tm_hour, p->tm_min);
                f_cat(FN_ETC_UNTRUST_ACL, buf);
                // Falls through
            case 9:
            case 0:
                fprintf(fsock, "quit\r\n");
                fclose(fsock);
                return step;
            }
        }
    }
    return 1;
}
