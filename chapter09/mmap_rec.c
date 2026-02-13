#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

/*
 * このプログラムは「POSIX 共有メモリ (shared memory)」の “読み取り側” の例である。
 *
 * 共有メモリの基本:
 *   - shm_open で「共有メモリオブジェクト」を開く（ファイルのように FD が得られる）
 *   - mmap でそれを自プロセスの仮想アドレス空間にマップする
 *   - p を通常のポインタとして参照し、中身（文字列など）を読む
 *
 * 想定される利用シナリオ（書き込み側とのペア）:
 *   (Writer) shm_open("/shared_memory", O_CREAT|O_RDWR) → ftruncate(4096) → mmap(PROT_WRITE) → 書き込み
 *   (Reader) shm_open("/shared_memory", O_RDONLY)       → mmap(PROT_READ)  → 読み取り
 *
 * 重要な注意:
 *   - 共有メモリは「同じ領域を共有」するだけで、同期（いつ書いた/いつ読む）は自動ではない
 *   - 排他制御や更新通知が必要なら、セマフォやフラグ領域など別設計が必要
 *
 * 本コードの全体アルゴリズム:
 *   1) shm_open で既存の共有メモリ "/shared_memory" を読み取り専用で開く
 *   2) mmap(PROT_READ, MAP_SHARED) で 4096 バイトをマップする
 *   3) p が指す先を文字列として fprintf で表示する
 *   4) munmap でマップ解除
 *   5) shm_unlink で共有メモリオブジェクト名を削除（破棄要求）
 *   6) close で FD を閉じて終了
 *
 * 共有メモリのイメージ（概念図）:
 *
 *   Writer プロセス                    Reader プロセス
 *     pW = mmap(PROT_WRITE)              pR = mmap(PROT_READ)
 *          |                                   |
 *          +-------- 同じ共有メモリ -----------+
 *
 * Reader は pR を読むだけで Writer の書き込み結果を参照できる。
 */

int main(int argc, char *argv[]){
   char *p;
   /*
    * p:
    *   mmap により共有メモリ領域をマッピングし、その先頭アドレスを受け取るポインタ。
    *   PROT_READ なので読み取り専用として扱う。
    */

   int fd, ret;
   /*
    * fd:
    *   shm_open が返すファイルディスクリプタ（共有メモリオブジェクトのハンドル）。
    *
    * ret:
    *   munmap 等の戻り値確認用。
    */

   fd = shm_open("/shared_memory", O_RDONLY, 0666);
   /*
    * shm_open(name, oflag, mode):
    *
    * name:
    *   "/shared_memory"
    *   先頭 '/' を付けた名前で管理される POSIX 共有メモリの名前空間。
    *
    * oflag:
    *   O_RDONLY → 読み取り専用で開く。
    *
    * mode:
    *   0666 は本来「作成時に効く権限」だが、
    *   既存オブジェクトを開く場合は大きな意味を持たないことが多い。
    *
    * 重要:
    *   ここは「既に共有メモリが存在する」ことが前提。
    *   まだ Writer が shm_open(O_CREAT...) していない場合、ここで失敗する。
    */

   if(fd == -1){
      fprintf(stderr, "shm_open failed\n");
      exit(1);
   }

   p = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
   /*
    * mmap(addr, length, prot, flags, fd, offset):
    *
    * addr:
    *   NULL → OS に配置場所を任せる
    *
    * length:
    *   4096 バイト（Writer 側が ftruncate で確保したサイズと一致している想定）
    *
    * prot:
    *   PROT_READ → 読み取り可能
    *
    * flags:
    *   MAP_SHARED → 共有（他プロセスの更新が見える）
    *
    * fd:
    *   shm_open で得た共有メモリ FD
    *
    * offset:
    *   0 → 先頭からマップ
    *
    * これにより共有メモリ領域が本プロセスの仮想アドレス空間に貼り付く。
    * p はその先頭を指す。
    */

   if(p == MAP_FAILED){
      perror("mmap");
      exit(1);
   }

   fprintf(stderr, "%s\n", p);
   /*
    * 共有メモリの先頭を C文字列として表示する。
    *
    * 重要な前提:
    *   - 共有メモリ上のデータが '\0' 終端された文字列であること
    *   - そうでない場合、fprintf は '\0' を見つけるまで読み続けて危険（未定義動作）になる
    *
    * よって、Writer 側が必ず終端付きで書く（または長さを別途共有する）必要がある。
    */

   ret = munmap(p, sizeof(char));
   /*
    * munmap(addr, length):
    *   mmap した領域をアンマップする（仮想アドレス空間から剥がす）。
    *
    * 注意（重要）:
    *   ここで length に sizeof(char) (=1) を指定しているが、
    *   mmap した長さは 4096 なので、本来は 4096 を指定するのが正しい。
    *
    * OS によっては「ページ境界」や「マップ単位」の制約があり、
    * 1バイトだけ unmap する指定は失敗するか、意図しない挙動になり得る。
    *
    * 正しくは:
    *   munmap(p, 4096);
    * あるいは Writer 側と同じサイズを sizeof(...) で共有する設計が望ましい。
    */

   if(ret == -1){
      perror("munmap");
      exit(1);
   }

   shm_unlink("/shared_memory");
   /*
    * shm_unlink(name):
    *   共有メモリオブジェクトの「名前」を削除する（unlink）。
    *
    * ファイルの unlink と同じ考え方で、
    *   - 名前空間からは消える
    *   - ただし他プロセスがまだ open/mmap して参照している場合はすぐに実体が消えるとは限らない
    *
    * ここで unlink すると、以後 shm_open("/shared_memory", ...) は基本的に失敗するようになる。
    * よって「読み取り側が最後に掃除する」設計になっていると解釈できる。
    */

   close(fd);
   /*
    * 共有メモリオブジェクトの FD を閉じる。
    * これで本プロセスは共有メモリオブジェクトへの参照を解放する。
    */

   return 0;
}
