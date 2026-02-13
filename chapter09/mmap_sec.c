#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

/*
 * このプログラムは「POSIX 共有メモリ (shared memory)」を使って、
 * 別プロセスとデータを共有するための “書き込み側” の例である。
 *
 * 共有メモリの考え方（重要）:
 *   - pipe/FIFO のような「カーネルバッファへ順番に流す」通信ではなく、
 *     「複数プロセスが同じメモリ領域を参照する」方式。
 *   - 一度 mmap でメモリにマップすると、その領域への読み書きは通常のメモリアクセスと同様になる。
 *   - ただし「同時アクセスの同期（排他制御）」は別途必要になることが多い（セマフォなど）。
 *
 * このコードの全体アルゴリズム:
 *   1) shm_open で共有メモリオブジェクトを作成/オープンする
 *   2) ftruncate で共有メモリのサイズを確保（ここでは 4096 バイト）
 *   3) mmap で共有メモリを自プロセスの仮想アドレス空間へマッピングする
 *   4) 標準入力から1行読み、その内容を共有メモリ領域にコピーする
 *   5) munmap でマッピング解除し、fd を close して終了する
 *
 * 想定される相手側（読み取り側）:
 *   - 同じ名前 "/shared_memory" を shm_open で開く
 *   - mmap(PROT_READ, MAP_SHARED, ...) で読み取り可能にマップ
 *   - p から文字列を読み取る
 *
 * 共有メモリのイメージ（概念図）:
 *
 *   プロセスA（書き込み側）         プロセスB（読み取り側）
 *        p = mmap(...)                   q = mmap(...)
 *            |                                |
 *            v                                v
 *      [ 同じ物理メモリページを共有 ]  ←→  [ 同じ物理メモリページを共有 ]
 *
 * つまり、A が p に書いた内容を、B は q から読める。
 */

int main(){
   char *p, line[4096], command[4096];
   /*
    * p:
    *   mmap により共有メモリをマッピングし、その先頭アドレスを指すポインタ。
    *
    * line:
    *   stdin から読み取る1行バッファ（最大4096バイト）。
    *
    * command:
    *   line から改行を除いた文字列を入れるバッファ。
    *   sscanf("%[^\n]") で改行までを取り出して格納する。
    */

   int fd, ret1;
   /*
    * fd:
    *   shm_open が返すファイルディスクリプタ。
    *   共有メモリオブジェクトを “ファイルのように” 扱うためのハンドル。
    *
    * ret1:
    *   各関数の戻り値チェック用。
    */

   fd = shm_open("/shared_memory", O_CREAT | O_RDWR, 0666);
   /*
    * shm_open(name, oflag, mode):
    *   POSIX共有メモリオブジェクトを作成/オープンする。
    *
    * name:
    *   "/shared_memory"
    *   先頭が '/' の名前空間で管理される（ファイルパスそのものではない）。
    *
    * oflag:
    *   O_CREAT | O_RDWR
    *   - O_CREAT: 存在しなければ作成
    *   - O_RDWR : 読み書き可能で開く
    *
    * mode:
    *   0666（umask の影響を受ける）
    *
    * 失敗すると -1 が返り、errno が設定される。
    */

   if(fd == -1){
      fprintf(stderr, "shm_open failed\n");
      exit(1);
   }

   ret1 = ftruncate(fd, sizeof(line));
   /*
    * ftruncate(fd, size):
    *   共有メモリオブジェクトの「サイズ」を設定する。
    *
    * 重要:
    *   shm_open 直後の共有メモリはサイズ0のことがあり、
    *   そのまま mmap すると失敗したり、アクセスで SIGBUS が起きる可能性がある。
    *
    * ここでは line のサイズ（4096）に合わせて確保している。
    */

   if(ret1 < 0){
      perror("ftruncate");
      exit(1);
   }

   p = mmap(NULL, sizeof(line), PROT_WRITE, MAP_SHARED, fd, 0);
   /*
    * mmap(addr, length, prot, flags, fd, offset):
    *
    * addr:
    *   NULL → OSに配置場所を任せる
    *
    * length:
    *   sizeof(line) = 4096
    *
    * prot:
    *   PROT_WRITE → 書き込み可能
    *
    * flags:
    *   MAP_SHARED → 変更が共有される（他プロセスから見える）
    *
    * fd:
    *   shm_open で得た共有メモリオブジェクトのFD
    *
    * offset:
    *   0 → 先頭からマップ
    *
    * これにより「共有メモリ」を自プロセスの仮想メモリ空間に貼り付け、
    * p を通じてメモリとしてアクセスできるようになる。
    */

   if(p == MAP_FAILED){
      perror("mmap");
      exit(1);
   }

   fgets(line, sizeof(line), stdin);
   /*
    * 標準入力から1行読み込む。
    * 例:
    *   "hello world\n"
    *
    * EOF の場合は NULL だが、ここでは簡略化されている。
    */

   ret1 = sscanf(line, "%[^\n]", command);
   /*
    * "%[^\n]" は改行以外を全て読む。
    * command に改行なしの文字列が入る。
    *
    * ret1 > 0 なら command に何か入ったと判断する。
    */

   if(ret1 > 0){
      strncpy(p, command, sizeof(command));
      /*
       * strncpy(宛先, 元, n):
       *   command の内容を共有メモリ領域 p にコピーする。
       *
       * 重要:
       *   p は共有メモリの先頭アドレスなので、
       *   ここに書いた文字列は他プロセスが同じ共有メモリを mmap していれば読める。
       *
       * 注意:
       *   strncpy は n 文字コピーするが、必ず '\0' 終端されるとは限らない。
       *   ただしここでは n=sizeof(command)=4096 なので、通常は終端が含まれやすいが、
       *   入力が4095文字以上だと終端が保証されない。
       *
       * また、共有メモリ上に以前の長い文字列が残っている場合、
       * 新しい短い文字列で上書きしても末尾の残骸が残り得る。
       * （プロトコル設計としては先頭に長さを書いたりゼロクリアする設計が多い）
       */
   }

   if(munmap(p, sizeof(line)) == -1){
      /*
       * munmap(addr, length):
       *   mmap した領域をアンマップする（仮想アドレス空間から剥がす）。
       *
       * 共有メモリオブジェクト自体が消えるわけではない。
       * 単にこのプロセスから見えなくなるだけ。
       */
      perror("munmap");
      return 1;
   }

   close(fd);
   /*
    * FD を閉じる。
    *
    * これも共有メモリオブジェクト自体の削除ではない。
    * オブジェクトを削除するには shm_unlink("/shared_memory") が必要。
    * （本コードでは削除は行わず、作りっぱなしにしている）
    */

   return 0;
}
