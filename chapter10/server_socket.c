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
 * このプログラムは TCP ソケットを使った “サーバ側” の最小例である。
 *
 * 役割（高レベル）:
 *   - 指定ポートで TCP サーバとして待ち受ける
 *   - クライアントが接続してきたら accept で接続を確立する
 *   - recv でクライアントから文字列を受け取る
 *   - 受け取った文字列の長さ n = strlen(buf) を計算する
 *   - n を send でクライアントに返す（応答）
 *   - "exit" を受け取ったら通信を終了する
 *
 * つまり、これまでの FIFO / 共有メモリの例と同じく
 *   要求: 文字列
 *   応答: 文字数
 * という request/response を TCP で実装した形になっている。
 *
 * --------------------------------------------------------------------
 * TCP ソケットの基本手順（サーバ側）:
 *
 *   socket()   : 通信口（ソケット）を作る
 *      ↓
 *   setsockopt(): 再起動時のポート再利用など設定
 *      ↓
 *   bind()     : ソケットに「待受IP/ポート」を割り当てる
 *      ↓
 *   listen()   : 接続待ち状態にする（受動オープン）
 *      ↓
 *   accept()   : 接続要求を受け付け、通信専用ソケット(cfd)を得る
 *      ↓
 *   recv/send  : cfd を使って送受信
 *      ↓
 *   close()    : ソケットを閉じる
 *
 * 重要:
 *   - sfd は “待ち受け用ソケット”（listen する側）
 *   - cfd は “通信専用ソケット”（accept 後の接続ごとにできる）
 *
 * --------------------------------------------------------------------
 * 注意点（このコードに特有）:
 *   - send(cfd, &n, ret, 0) は「送るバイト数」が ret（=受信バイト数）になっているが、
 *     n は int なので本来は sizeof(n) を送るべきである（バグの可能性）。
 *   - recv の戻り値 ret は「受信したバイト数」。buf はヌル終端されない可能性があるため、
 *     文字列として扱うなら ret を使って終端を入れる設計が安全。
 *   - bind/listen/accept のエラーチェックが無い（教材として簡略化されている）。
 */

int main(int argc, char *argv[]){
   unsigned short port;
   int sfd = -1, cfd = -1; // サーバ用ソケット（待受用）とクライアント用ソケット（通信専用）
   int ret, n, on = 1;
   struct sockaddr_in s_addr, c_addr;
   int addr_len = sizeof(struct sockaddr_in);
   char buf[BUF_SIZE];

   /*
    * 引数チェック:
    *   argv[1] にポート番号を指定する。
    */
   if(argc != 2){
      fprintf(stderr, "Usage: $ ./server_socket [port]\n");
      exit(1);
   }
   port = (unsigned short)atoi(argv[1]);
   /*
    * atoi で数値に変換してポートとして使う。
    * 実務なら範囲チェック（1〜65535）や atoi の失敗検出が欲しい。
    */

   // ソケットの生成
   sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   /*
    * socket(domain, type, protocol):
    *
    * domain:
    *   AF_INET → IPv4
    *
    * type:
    *   SOCK_STREAM → TCP（信頼性のあるバイトストリーム）
    *
    * protocol:
    *   IPPROTO_TCP → TCP を明示（通常 0 でも良い）
    *
    * 成功: ソケットFD（整数）を返す
    * 失敗: -1
    */

   // TIME_WAIT中にサーバを再スタートしたとき、ポートを再利用できるようにする
   ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
   /*
    * setsockopt(fd, level, optname, optval, optlen):
    *
    * SO_REUSEADDR:
    *   直前に同じポートを使っていた接続が TIME_WAIT 状態でも、
    *   サーバの再起動時に bind できる可能性を上げるオプション。
    *
    * 学習中にサーバを何度も落として起動する場合に便利。
    */

   if(ret < 0){
      perror("setsockopt");
      exit(1);
   }

   // sockaddr_in構造体にIP,portをセット
   memset(&s_addr, 0, sizeof(s_addr)); // 構造体をゼロクリア
   /*
    * sockaddr_in は未初期化だとゴミ値が入るため、必ず memset で初期化してから埋める。
    */

   s_addr.sin_port = htons(port);
   /*
    * sin_port は “ネットワークバイトオーダ（ビッグエンディアン）” で渡す必要があるため、
    * htons(host-to-network-short) で変換する。
    */

   s_addr.sin_family = AF_INET;
   /*
    * アドレスファミリ（IPv4）。
    */

   s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   /*
    * INADDR_ANY は「このマシンの全てのNICで受け付ける」という意味。
    * 0.0.0.0 でバインドするイメージ。
    *
    * htonl(host-to-network-long) でネットワークバイトオーダにする。
    */

   fprintf(stderr, "Address=%s, Port=%u\n", inet_ntoa(s_addr.sin_addr), port);
   /*
    * inet_ntoa は IPv4 アドレスを文字列に変換する。
    * ただし INADDR_ANY は 0.0.0.0 相当になる。
    */

   // ソケットの bind
   bind(sfd, (struct sockaddr *)&s_addr, sizeof(s_addr));
   /*
    * bind(fd, addr, addrlen):
    *   作ったソケット sfd に「待ち受けるIP/ポート」を紐づける。
    *
    * 本来は ret=bind(...) としてエラーチェック（-1なら perror）を入れるのが安全。
    */

   // 接続を待つ
   listen(sfd, 5);
   /*
    * listen(fd, backlog):
    *   接続待ち状態にする（受動オープン）。
    *
    * backlog=5:
    *   接続待ちキューの長さの目安。
    *
    * 以降 accept が呼ばれるまで、クライアントの接続要求をキューイングする。
    */

   fprintf(stderr, "Waiting for connection...\n");

   // 接続要求を受け付ける
   cfd = accept(sfd, (struct sockaddr *)&c_addr, &addr_len);
   /*
    * accept(listen_fd, peer_addr, peer_addrlen):
    *
    * 戻り値:
    *   - 成功: 新しい FD（通信専用ソケット）
    *   - 失敗: -1
    *
    * 重要:
    *   sfd は引き続き “待受” のために残る。
    *   cfd はこの接続（クライアント1人）との通信に使う。
    *
    * このサンプルは accept を 1回しか呼ばないので、
    * クライアントは1接続のみ対応（単発サーバ）。
    */

   fprintf(stderr, "Connected from %s\n", inet_ntoa(c_addr.sin_addr));

   // クライアントとの送受信ループ
   while(1){
      memset(buf, 0, BUF_SIZE);
      /*
       * バッファをゼロクリアしておくと、受信後に文字列として扱いやすい。
       * ただし recv はヌル終端を保証しないので、本来は ret を見て終端を入れるべき。
       */

      ret = recv(cfd, buf, BUF_SIZE, 0);
      /*
       * recv(fd, buf, len, flags):
       *   TCP は “バイトストリーム” なので、1回の recv でメッセージ単位が保たれるとは限らない。
       *   ただし教材では「短い文字列が1回で届く」想定で使っている。
       *
       * 戻り値 ret:
       *   - >0: 受信したバイト数
       *   -  0: 相手が orderly shutdown（接続を閉じた）
       *   - -1: エラー
       */

      if(ret == 0 || ret == -1){
         /*
          * 相手が切断した、もしくはエラーなのでループ終了。
          */
         break;
      }

      fprintf(stderr, "received: %s\n", buf);
      /*
       * 受信データを表示。
       * buf はゼロクリアしているので、短い入力なら文字列として表示できやすい。
       */

      n = strlen(buf);
      /*
       * 受信文字列の長さを計算。
       * ここでも '\0' 終端が前提になる。
       */

      send(cfd, &n, ret, 0);
      /*
       * send(fd, buf, len, flags):
       *   ここは注意が必要。
       *
       * 本来送りたいのは int n のバイト列なので、len は sizeof(n) が自然。
       * しかしこのコードでは len=ret（=受信バイト数）になっている。
       *
       * 例:
       *   "Apple" を受信 → ret=5
       *   send(&n, 5) → int(4バイト) を超えて 5バイト送ろうとしてしまい、余計な1バイトを送る可能性がある。
       *
       * 教材として “動いてしまう” 場合もあるが、正しくは:
       *   send(cfd, &n, sizeof(n), 0);
       * または、n を文字列化して send(buf, strlen(buf)) に統一する設計が安全。
       */

      ret = strcmp(buf, "exit");
      /*
       * "exit" を受け取ったら終了。
       * strcmp は一致なら 0。
       */
      if(ret == 0) break;
   }

   // ソケットのクローズ
   if(sfd != -1){
      /*
       * 待受用ソケットを閉じる。
       */
      close(sfd);
   }
   if(cfd != -1){
      /*
       * 通信用ソケット（クライアントとの接続）を閉じる。
       */
      close(cfd);
   }

   return 0;
}
