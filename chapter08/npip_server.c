#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

/*
 * このプログラムは「名前付きパイプ（FIFO）」を使った簡易サーバ（server）側の例である。
 *
 * 役割（全体アルゴリズム）:
 *   - クライアント（別プロセス）が FIFO に送ってくる文字列 word を受信する
 *   - 受信した word の長さ n = strlen(word) を計算する
 *   - その n を FIFO に書き戻してクライアントへ返す
 *   - これを無限に繰り返す
 *
 * このサーバは、クライアントと「要求→応答」のプロトコルで通信している。
 *
 * プロトコル（順序が重要）:
 *   1) server: FIFO を O_RDONLY で open して word を受け取る
 *   2) server: FIFO を O_WRONLY で open して n を返す
 *
 * クライアント側も同じ順序にしないと詰まる（ブロック）可能性がある。
 *
 * FIFO（named pipe）のポイント:
 *   - ファイルパス "./named_pipe" を介して、親子関係が無いプロセス同士でも通信できる
 *   - FIFO は「一方向ストリーム」であり、open/read/write の順序は非常に重要
 *   - open(O_RDONLY) や open(O_WRONLY) は相手側が open するまでブロックし得る
 *
 * 図解（概念）:
 *
 *   client                       server
 *     | write(word)  ---> FIFO --->  read(word)
 *     | read(n)      <--- FIFO <---  write(n)
 *
 * ただし本実装は「1本の FIFO を要求と応答の両方に使う」設計である。
 * 教材としては成立するが、実務では通常「要求用FIFO」「応答用FIFO」を分ける設計が多い。
 */

int main(){ // server
   int fd, n;
   /*
    * fd:
    *   FIFO を open したときのファイルディスクリプタ。
    *
    * n:
    *   word の長さ（strlen(word)）を格納し、クライアントへ返す整数。
    */

   char *myfifo = "./named_pipe";
   /*
    * FIFO のパス。
    * クライアントも同じパスを open することで通信できる。
    */

   char line[256], word[256];
   /*
    * word:
    *   FIFO から受け取る文字列（固定長256バイト前提）。
    *
    * line:
    *   ここでは使われていない（残骸変数）。
    *   教材の途中経過で残った可能性がある。
    */

   mkfifo(myfifo, 0666);
   /*
    * mkfifo(path, mode):
    *   名前付きパイプ(FIFO)を作成する。
    *
    * mode=0666:
    *   読み書き権限を全ユーザに与える（umask の影響を受ける）。
    *
    * 注意:
    *   既に同名 FIFO が存在すると EEXIST で失敗する。
    *   本コードは戻り値チェックを省略している（教材として簡略化）。
    */

   while(1){
      /*
       * サーバループ:
       *   (1) クライアントの要求（word）を受信
       *   (2) 応答（n）を計算して送信
       * を繰り返す。
       */

      fd = open(myfifo, O_RDONLY);
      /*
       * FIFO を読み込みモードで open。
       *
       * FIFOの性質:
       *   open(O_RDONLY) は相手（クライアント）が write 側で open するまで
       *   ブロックすることがある。
       *
       * つまり、クライアントが来るまでここで待つ「サーバらしい挙動」になる。
       */

      read(fd, word, sizeof(word));
      /*
       * クライアントが送った word を受信する。
       *
       * 注意:
       *   sizeof(word)=256 なので固定長で読む設計。
       *   クライアント側も固定長256バイトで write する前提になっている。
       *
       * 実務なら read の戻り値を確認し、
       * 短読・エラー・EOF を扱うのが普通。
       */

      fprintf(stderr, "crient: %s \n", word);
      /*
       * 受信した文字列を表示。
       *
       * "crient" はおそらく "client" のタイポ。
       */

      close(fd);
      /*
       * 読み込み完了後に close。
       *
       * FIFO では、読み側・書き側の open/close の順序がプロトコルになるため、
       * 「読む→閉じる」を明確に区切っている。
       */

      fd = open(myfifo, O_WRONLY);
      /*
       * FIFO を書き込みモードで open。
       *
       * FIFOの性質:
       *   open(O_WRONLY) は相手（クライアント）が read 側で open するまで
       *   ブロックし得る。
       *
       * ここではクライアントが応答を受け取るために
       * open(O_RDONLY) してくれることを期待している。
       */

      n = strlen(word);
      /*
       * 受け取った word の長さを計算。
       *
       * 重要:
       *   strlen は '\0'（ヌル終端）まで数える。
       *   したがってクライアントが送った word が
       *   適切に '\0' を含む文字列であることが前提。
       */

      write(fd, &n, sizeof(n));
      /*
       * 計算結果 n（int）を FIFO に書き戻す。
       *
       * 注意:
       *   int のサイズは環境依存（多くは4バイト）。
       *   送受信双方が同じ環境で動く前提なら問題になりにくいが、
       *   一般には int32_t など固定幅整数を使うと安全。
       */

      close(fd);
      /*
       * 応答送信完了後に close。
       * これで 1 回の要求→応答が完結する。
       */
   }

   return 0;
}
