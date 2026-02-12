#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

/*
 * このプログラムは「名前付きパイプ（FIFO: named pipe）」を使って、
 * 別プロセスとデータをやり取りする IPC（プロセス間通信）の例である。
 *
 * 無名パイプ(pipe)との違い（重要）:
 *   - 無名パイプ: pipe(fd) で作り、基本的に親子（fork したプロセス）間で使う
 *   - 名前付きパイプ(FIFO): mkfifo("path", mode) で「ファイルとして」作り、
 *                            親子関係がない別プロセス同士でも、同じパスを open して通信できる
 *
 * このコードの全体アルゴリズム（想定）:
 *   1) ./named_pipe という FIFO を作成（すでにあっても作ろうとする）
 *   2) 標準入力から word を読む
 *   3) FIFO を O_WRONLY で open して word(固定長) を送る
 *   4) FIFO を O_RDONLY で open して int n を受け取る
 *   5) "word <-> n" を表示する
 *   6) これを繰り返す
 *
 * 想定される相手側プロセス（サーバ的役割）のイメージ:
 *   - FIFO を O_RDONLY で open して word を受け取る
 *   - strlen(word) のような計算で n を作る
 *   - FIFO を O_WRONLY で open して n を返す
 *
 * 重要ポイント:
 *   - FIFO は 1本で「ストリーム」なので、送受信の順序（プロトコル）が合っていないと詰まる
 *   - open(O_WRONLY) は相手が読み側 open するまでブロックする
 *   - open(O_RDONLY) は相手が書き側 open するまでブロックする（状況による）
 *   - このコードは「書く→閉じる→読む→閉じる」を毎回やるので、相手側と握りが必要
 */

int main(){
   int fd, ret1, n;
   /*
    * fd:
    *   FIFO を open したときのファイルディスクリプタ。
    *
    * ret1:
    *   sscanf の戻り値（入力が取れたかどうか）。
    *
    * n:
    *   FIFO から受け取る整数結果（相手が計算して返してくる値）。
    */

   char *myfifo = "./named_pipe";
   /*
    * FIFO のパス。
    * 相手プロセスもこのパスを使って open することで通信できる。
    */

   char line[256], word[256];
   /*
    * line:
    *   stdin から1行読み込むバッファ。
    *
    * word:
    *   改行を除いた入力文字列を格納する。
    */

   mkfifo(myfifo, 0666);
   /*
    * mkfifo(path, mode):
    *   名前付きパイプ（FIFO）を作成する。
    *
    * mode=0666:
    *   読み書き権限を全ユーザに与える（umask の影響は受ける）。
    *
    * 注意:
    *   既に同名の FIFO が存在すると失敗する（errno=EEXIST）。
    *   本コードは戻り値チェックをしていないため、
    *   教材として簡略化されている。
    */

   while(1){
      /*
       * REPL 的ループ:
       *   標準入力 → FIFOへ送信 → FIFOから結果受信 → 表示
       */
      fgets(line, sizeof(line), stdin);
      /*
       * 1行読み込み。
       * EOF なら NULL になるが、このコードは EOF を考慮していない（簡略化）。
       */

      ret1 = sscanf(line, "%[^\n]", word);
      /*
       * "%[^\n]" は改行以外を全て読む。
       * つまり word には改行を除いた文字列が入る。
       *
       * ret1 > 0 なら入力が存在したとみなす。
       */

      if(ret1 > 0){
         /*
          * ここから「要求→応答」の通信を行う。
          *
          * プロトコル（順序）:
          *   (1) こちらが word を送る
          *   (2) 相手が n を返す
          */

         fd = open(myfifo, O_WRONLY);
         /*
          * open(path, O_WRONLY):
          *   FIFO を書き込み専用で開く。
          *
          * FIFOの重要な性質:
          *   書き込み側 open(O_WRONLY) は
          *   「相手が読み込み側 open(O_RDONLY) してくれるまでブロック」し得る。
          *
          * つまり、相手プロセスがいないとここで止まる可能性がある。
          */

         write(fd, word, sizeof(word));
         /*
          * word を FIFO に書き込む。
          *
          * 注意:
          *   sizeof(word)=256 なので固定長で送信している。
          *   実務なら strlen(word)+1 など必要分だけ送るのが一般的。
          *
          * 相手側も 256 バイト読み取る前提で実装されている必要がある。
          */

         close(fd);
         /*
          * 送信完了後に close する。
          *
          * FIFO では close により「書き込み側がいなくなった」状態が相手に伝わり、
          * 相手側の read が EOF を受け取る条件の一部になる。
          */

         fd = open(myfifo, O_RDONLY);
         /*
          * open(path, O_RDONLY):
          *   FIFO を読み込み専用で開く。
          *
          * FIFOの性質:
          *   読み込み側 open(O_RDONLY) も、
          *   相手が書き込み側を open するまでブロックし得る。
          *
          * ここでは「相手が n を書き込むために open(O_WRONLY) する」ことを期待している。
          */

         read(fd, &n, sizeof(n));
         /*
          * FIFO から int n を受け取る。
          *
          * n は相手が計算して返した結果である想定。
          * 例: word の文字数
          *
          * 型サイズに依存するため、送受信する両者が同じ環境前提ならOK。
          * 異なる環境を考えるなら int32_t 等の固定幅を使うのが安全。
          */

         fprintf(stderr, "%s <-> %d \n", word, n);
         /*
          * 対応表示。
          * 例:
          *   apple <-> 5
          */

         close(fd);
         /*
          * 受信後 close。
          */
      }
   }

   return 0;
}
