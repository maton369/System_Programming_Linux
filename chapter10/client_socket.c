#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 256

/*
 * このプログラムは TCP ソケットを使った “クライアント側” の最小例である。
 *
 * 役割（高レベル）:
 *   - 指定されたサーバIPアドレス・ポートへ TCP 接続する（connect）
 *   - 標準入力から文字列を読み取る
 *   - その文字列をサーバへ送る（send）
 *   - サーバから「文字列の長さ（int）」を受信する（recv）
 *   - "exit" を入力したら終了する
 *
 * つまり、サーバ側プログラムが行っていた
 *   受信した文字列の strlen を計算して返す
 * という処理の “相手側” であり、TCP を使った request/response のペアになっている。
 *
 * --------------------------------------------------------------------
 * TCP ソケットの基本手順（クライアント側）:
 *
 *   socket()   : 通信口を作る
 *      ↓
 *   connect()  : サーバ（IP/port）へ接続要求し、接続を確立する（能動オープン）
 *      ↓
 *   send/recv  : 送受信
 *      ↓
 *   close()    : 終了
 *
 * --------------------------------------------------------------------
 * 注意点（このコードに特有 / 教材として重要）:
 *
 * 1) send で送るバイト数
 *    - このコードは send(myfd, word, strlen(word), 0) で “文字列の実長” を送っている。
 *    - これはサーバ側が recv で受け取れる。
 *
 * 2) recv で int を受け取る設計
 *    - ret = recv(myfd, &n, BUF_SIZE, 0) となっているが、
 *      本来 n は int なので sizeof(n) だけ受け取るのが自然。
 *    - BUF_SIZE で受け取ると「int 以上のサイズで読み込みに行く」ので、
 *      サーバ側の send の実装次第では余計なデータを読もうとしてしまう可能性がある。
 *
 * 3) TCP は “メッセージ境界がない”
 *    - send/recv 1回ずつで文字列と応答が必ず対応する保証は本来ない。
 *      ただし教材として短いデータであれば成立する、という前提で書かれている。
 */

int main(int argc, char *argv[]){
   char *server_ip;
   unsigned short port;
   int myfd = -1, ret, ret_rcv, n;
   struct sockaddr_in my_addr;
   char word[BUF_SIZE];

   /*
    * 引数:
    *   argv[1] = サーバIPアドレス（例 "127.0.0.1"）
    *   argv[2] = ポート番号（例 "5000"）
    */
   if(argc != 3){
      fprintf(stderr, "Usage:$ ./client_socket [ip_address] [port]\n");
      exit(1);
   }

   server_ip = argv[1];
   port = (unsigned short)atoi(argv[2]);
   /*
    * atoi でポート番号を数値化している。
    * 実務なら 1〜65535 の範囲チェックが欲しい。
    */

   myfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   /*
    * socket(AF_INET, SOCK_STREAM, IPPROTO_TCP):
    *   IPv4 + TCP のソケットを作る（クライアントもサーバも同じ作り方）。
    *
    * 戻り値:
    *   成功: FD（>=0）
    *   失敗: -1
    */

   memset(&my_addr, 0, sizeof(my_addr));
   /*
    * sockaddr_in をゼロクリアしてから必要なフィールドを埋める。
    */

   my_addr.sin_port = htons(port);
   /*
    * ポート番号はネットワークバイトオーダ（ビッグエンディアン）に変換する。
    */

   my_addr.sin_family = AF_INET;
   /*
    * IPv4 を表すアドレスファミリ。
    */

   my_addr.sin_addr.s_addr = inet_addr(server_ip);
   /*
    * inet_addr("127.0.0.1") のように IPv4 文字列を 32bit 値へ変換する。
    *
    * 注意:
    *   inet_addr は失敗時に INADDR_NONE を返すなど扱いが微妙なので、
    *   実務では inet_pton を使うことが多い。
    */

   fprintf(stderr, "Connecting to %s:\n", server_ip);

   // サーバのソケットに接続する
   connect(myfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
   /*
    * connect(fd, addr, addrlen):
    *   指定したサーバへ TCP 接続を確立する（能動オープン）。
    *
    * 成功すると以降 myfd に対して send/recv が可能になる。
    *
    * 本来は connect の戻り値をチェックし、失敗時は perror するのが安全。
    */

   // サーバとの送受信ループ
   while(1){
      memset(word, 0, BUF_SIZE);
      /*
       * 入力バッファのクリア。
       */

      fprintf(stderr, "> ");
      fgets(word, sizeof(word), stdin);
      /*
       * 標準入力から1行読む。
       * 末尾に '\n' が付く（入力が長すぎる場合など例外あり）。
       */

      word[strlen(word) - 1] = '\0';
      /*
       * fgets で入った改行 '\n' を '\0' に置き換えて除去する。
       *
       * 注意:
       *   空行や EOF の場合に strlen(word)-1 が危険になり得る。
       *   教材としては「必ず何か入力される」前提。
       */

      send(myfd, word, strlen(word), 0);
      /*
       * send(fd, buf, len, flags):
       *   サーバへ文字列を送る。
       *
       * len = strlen(word):
       *   '\0' は送らない（文字列の本体だけ送る）。
       *   サーバ側は受信したバイト列を文字列として扱うなら終端処理が必要だが、
       *   サーバ側では buf をゼロクリアしているので成立しやすい。
       */

      ret = strcmp(word, "exit");
      /*
       * "exit" を送ったら終了する（サーバにも "exit" が届く）。
       */
      if(ret == 0) break;

      ret = recv(myfd, &n, BUF_SIZE, 0);
      /*
       * サーバから応答（文字数）を受信する。
       *
       * 重要:
       *   ここは「int n を受け取る」設計なので、
       *   本来は BUF_SIZE ではなく sizeof(n) だけ受け取るのが自然。
       *
       *   正しくは概念的に:
       *     recv(myfd, &n, sizeof(n), 0);
       *
       * また、TCP はストリームのため、1回の recv で必ず sizeof(n) が揃う保証は本来ない。
       * 教材では短い応答で揃う前提になっている。
       */

      if(ret == 0 || ret == -1){
         /*
          * ret==0: サーバが接続を閉じた
          * ret==-1: エラー
          */
         break;
      }

      fprintf(stderr, "from server: %d\n", n);
      /*
       * 受信した n を表示。
       */
   }

   // ソケットのクローズ
   if(myfd != -1){
      close(myfd);
      /*
       * 接続をクローズする。
       * TCPでは FIN を送り、相手に切断を通知する流れになる。
       */
   }

   return 0;
}
