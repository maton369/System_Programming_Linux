#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define BUF_SIZE 256
#define C_MAX 5

/*
 * このプログラムは TCP サーバを「select() による I/O 多重化」で実装した例である。
 *
 * 目的（高レベル）:
 *   - 1つの待受ソケット(sfd)で新規接続を受け付ける
 *   - 最大 C_MAX 個のクライアント接続を同時に扱う（スレッドは使わない）
 *   - 各クライアントから文字列を recv で受け取り、文字数 n=strlen(buf) を返す
 *   - "exit" を受け取ったクライアントは切断し、スロットを空ける
 *   - Ctrl+C(SIGINT) を受けたら全ソケットを close して終了する
 *
 * --------------------------------------------------------------------
 * 【select による多重化の考え方】
 *
 * select は複数の FD（ファイルディスクリプタ）について、
 * 「読み込み可能になった FD があるか」をまとめて待つ仕組みである。
 *
 * ここで監視する FD は2種類:
 *   1) sfd : 待受ソケット（ここが readable になる = 新規接続が来て accept 可能）
 *   2) socketfds[i] : accept 後の通信ソケット（ここが readable になる = クライアントからデータ到着）
 *
 * select ループの典型形:
 *
 *   FD_ZERO(&rfds);
 *   FD_SET(sfd, &rfds);
 *   FD_SET(各クライアントFD, &rfds);
 *   ret = select(fd_max+1, &rfds, NULL, NULL, timeout);
 *   if FD_ISSET(sfd) なら accept
 *   if FD_ISSET(clientfd) なら recv/send
 *
 * --------------------------------------------------------------------
 * 【重要な注意点（このコードにある不整合/改善点）】
 *
 * 1) send の送信サイズが不適切:
 *    send(socketfds[i], &n, ret_rcv, 0) となっているが、
 *    n は int なので本来は sizeof(n) を送るべきである。
 *    ret_rcv は「受信した文字列バイト数」であり、int のサイズとは無関係。
 *
 *    正しくは概念的に:
 *      send(socketfds[i], &n, sizeof(n), 0);
 *
 * 2) recv の戻り値 ret_rcv は「受信したバイト数」であり、
 *    buf は必ず '\0' 終端されるとは限らない。
 *    ただしこのコードは memset(buf,0,BUF_SIZE) しているため短い入力では文字列化しやすい。
 *
 * 3) accept の addr_len 型:
 *    accept の第3引数は socklen_t* が推奨。
 *    このコードは (socklen_t *)&addr_len とキャストしている。
 *
 * 4) SIGINT ハンドラ内で close/exit は教材としてはOKだが、
 *    非同期シグナル安全性など厳密には論点がある（学習段階では深追い不要）。
 */

void stop(int x);

/*
 * グローバル変数:
 *   SIGINT ハンドラ stop() からも参照できるようにグローバルにしている。
 */
int sfd = -1;                 // 待受ソケットFD
int socketfds[C_MAX];         // クライアント通信ソケットFDの配列（-1 は空きスロット）

int main(int argc, char *argv[]){
   unsigned short port;
   int ret, ret_rcv, on = 1, max = C_MAX, fd_max, i, n;
   fd_set rfds;
   struct sockaddr_in s_addr, c_addr;
   struct timeval tm;
   int addr_len = sizeof(struct sockaddr_in);
   char buf[BUF_SIZE];

   /*
    * 引数チェック:
    *   argv[1] に待受ポート番号を指定。
    */
   if(argc != 2){
      fprintf(stderr, "Usage: $ server <port>\n");
      exit(1);
   }
   port = (unsigned short)atoi(argv[1]);

   /*
    * sockaddr_in をゼロクリア（未初期化のゴミ値を避ける）。
    */
   memset(&s_addr, 0, sizeof(s_addr)); // 受信バッファの初期化（実際はアドレス構造体初期化）

   /*
    * クライアントFD配列を -1 で初期化。
    * -1 を「空きスロット」として扱う。
    */
   for(i = 0; i < C_MAX; i++){
      socketfds[i] = -1;
   }

   // シグナルハンドラの設定（Ctrl+C で stop が呼ばれる）
   signal(SIGINT, stop);

   // ソケットの生成（待受用）
   sfd = socket(PF_INET, SOCK_STREAM, 0);
   /*
    * socket(PF_INET, SOCK_STREAM, 0):
    *   IPv4 + TCP の待受ソケットを作る。
    *   AF_INET と PF_INET は実用上ほぼ同義として扱われることが多い。
    */

   // TIME_WAIT中の再起動でも bind しやすくする（ポート再利用）
   ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
   if(ret < 0){
      perror("setsockopt");
      exit(1);
   }

   // sockaddr_in 構造体に待受IP/portを設定
   s_addr.sin_port = htons(port);
   s_addr.sin_family = PF_INET;
   s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   /*
    * INADDR_ANY = 0.0.0.0 と同義で「全NICで待受」。
    */

   fprintf(stderr, "Address=%s, Port=%u\n", inet_ntoa(s_addr.sin_addr), port);

   // bind: ソケットに待受IP/portを割り当てる
   ret = bind(sfd, (struct sockaddr *)&s_addr, sizeof(s_addr));
   if(ret < 0){
      perror("bind");
      exit(1);
   }

   // listen: 接続待ち状態へ移行（受動オープン）
   ret = listen(sfd, 5);
   if(ret < 0){
      perror("listen");
      exit(1);
   }
   fprintf(stderr, "Waiting for connection...\n");

   /*
    * select ループ:
    *   - 新規接続が来たか（sfd が readable になったか）
    *   - 既存クライアントからデータが来たか（socketfds[i] が readable になったか）
    * を同時に監視する。
    */
   while(1){
      FD_ZERO(&rfds);
      /*
       * rfds は「読み込み可否を監視する FD 集合」。
       * select のたびに作り直す必要がある（select が中身を書き換えるため）。
       */

      FD_SET(sfd, &rfds);
      /*
       * 待受ソケット sfd を監視対象へ追加。
       * sfd が readable になる = accept 可能な新規接続が到着している可能性。
       */

      fd_max = sfd;
      /*
       * select は “0〜fd_max まで” を走査するため、最大FD番号が必要。
       * 監視対象の FD の最大値を求める。
       */

      for(i = 0; i < C_MAX; i++){
         if(socketfds[i] != -1){
            FD_SET(socketfds[i], &rfds);
            if(socketfds[i] > fd_max) fd_max = socketfds[i];
         }
      }

      // select のタイムアウト設定（5秒）
      tm.tv_sec = 5;
      tm.tv_usec = 0;

      // 接続や受信の監視（読み込み可能になるまで待つ）
      ret = select(fd_max + 1, &rfds, NULL, NULL, &tm);
      /*
       * ret:
       *   >0: readable になった FD の数
       *    0: タイムアウト（今回は5秒で戻る）
       *   -1: エラー（シグナル割り込みなど）
       */

      if(ret < 0){
         perror("select");
         break;
      }

      /*
       * 1) 新規接続チェック:
       *   sfd が readable なら accept できる接続要求が来ている。
       */
      ret = FD_ISSET(sfd, &rfds);
      if(ret != 0){
         fprintf(stderr, "Accept new connection\n");

         // 空きスロットを探して accept した FD を保存する
         for(i = 0; i < C_MAX; i++){
            if(socketfds[i] == -1){
               socketfds[i] = accept(sfd, (struct sockaddr *)&c_addr, (socklen_t *)&addr_len);
               /*
                * accept に成功すると、クライアントとの通信専用 FD が返る。
                * これを socketfds[i] に格納して “接続中クライアント一覧” に登録する。
                */

               fprintf(stderr, "client accepted(%d) from %s\n", i, inet_ntoa(c_addr.sin_addr));
               fprintf(stderr, "client fd number=%d\n", socketfds[i]);
               break;
            }
         }
         /*
          * 注意:
          *   C_MAX を超える接続が来た場合、空きがなく accept を呼ばないので、
          *   クライアントは接続できない/待たされる可能性がある。
          *   実務なら「空きが無い時は accept してすぐ close する」等の扱いも検討する。
          */
      }

      /*
       * 2) 既存クライアントの受信チェック:
       *   各 clientfd について FD_ISSET なら recv 可能。
       */
      for(i = 0; i < C_MAX; i++){
         if(socketfds[i] != -1){
            ret = FD_ISSET(socketfds[i], &rfds);
            if(ret != 0){
               memset(buf, 0, BUF_SIZE);

               ret_rcv = recv(socketfds[i], buf, BUF_SIZE, 0);
               /*
                * recv の戻り値:
                *   >0: 受信成功（バイト数）
                *    0: 相手が切断（orderly shutdown）
                *   -1: エラー
                */

               if(ret_rcv > 0){
                  fprintf(stderr, "received: %s\n", buf);

                  n = strlen(buf);
                  /*
                   * 受信した文字列の長さを計算。
                   * buf はゼロクリア済みなので短い入力なら '\0' 終端が効きやすい。
                   * ただし “本来” は ret_rcv を使って終端を入れる方が安全。
                   */

                  send(socketfds[i], &n, ret_rcv, 0);
                  /*
                   * 注意（重要）:
                   *   ここは send サイズが ret_rcv になっている。
                   *   返したいのは int n なので本来は sizeof(n) を送るべき。
                   *
                   * 正しい概念:
                   *   send(socketfds[i], &n, sizeof(n), 0);
                   */

                  if(strcmp(buf, "exit") == 0){
                     /*
                      * "exit" を受け取ったらそのクライアントを切断し、
                      * スロットを空ける。
                      */
                     close(socketfds[i]);
                     socketfds[i] = -1;
                  }
               }
               else{
                  /*
                   * ret_rcv == 0（切断）または ret_rcv < 0（エラー）の場合
                   * クライアントを切断扱いにしてスロットを空ける。
                   */
                  fprintf(stderr, "socket=%d disconnected: \n", socketfds[i]);
                  close(socketfds[i]);
                  socketfds[i] = -1;
               }
            }
         }
      }
   } // select ループの最後

   // ソケットのクローズ（ループを抜けた場合の後始末）
   close(sfd);
   /*
    * main 末尾に return が無いが、ここでは暗黙に終了する。
    * 実務なら return 0; を入れるのが読みやすい。
    */
}

/*
 * SIGINT（Ctrl+C）で呼ばれる終了処理。
 * 教材として「ソケットを全部 close して終了する」ことを示している。
 */
void stop(int x){
   int i;

   if(sfd != -1){
      close(sfd);
   }

   for(i = 0; i < C_MAX; i++){
      if(socketfds[i] != -1){
         close(socketfds[i]);
      }
   }

   exit(1);
}
