#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>

/*
 * このプログラムは「共有メモリ（POSIX shm）」と「System V セマフォ」を使い、
 * 別プロセス（入力側＝送信側）が共有メモリに書いた文字列を受け取り、
 * その文字数を計算して “結果（数字文字列）” を共有メモリへ書き戻す側である。
 *
 * コメントにある通り:
 *   - shared memory の文字列を受け取って
 *   - 文字数を代入する（= 共有メモリ上の文字列を数値文字列へ置き換える）
 *
 * つまり「要求→応答」型の処理であり、
 *   要求: "Apple"
 *   応答: "5"
 * のように、共有メモリ上の内容を書き換えて返している。
 *
 * --------------------------------------------------------------------
 * 【全体アルゴリズム（高レベル）】
 *
 * 共有資源:
 *   - POSIX共有メモリ: "/shared_memory"（4096バイト）
 *   - System V セマフォ: 1個（semid の sem[0]）
 *
 * ループの基本:
 *   1) セマフォで「要求が来るまで待つ」
 *   2) 共有メモリ p から要求文字列を読む
 *   3) "exit" なら終了
 *   4) strlen(p) で文字数 n を計算
 *   5) snprintf で n を文字列に変換し、p に書き戻す（応答）
 *   6) セマフォで「応答を書いた」ことを通知
 *
 * 重要:
 *   共有メモリは “データ置き場” でしかないため、
 *   読むタイミング・書くタイミングはセマフォで厳密に同期する必要がある。
 *
 * --------------------------------------------------------------------
 * 【セマフォの使い方が独特】
 * このプログラムは sem_op に -2 や +1 を使っている。
 * これは「セマフォ値を状態（フェーズ）として扱う」設計であり、
 * 相手側（入力側）が +2 / -1 / +1 などを使って状態を進めることを前提にしている。
 *
 * 典型的な二値セマフォの P(-1)/V(+1) よりも “プロトコル色” が強いので、
 * 相手実装と1ビットでもズレるとデッドロックしやすい。
 *
 * --------------------------------------------------------------------
 * 【注意点（バグ/危険ポイント）】
 * - munmap の length が sizeof(char) (=1) になっているが、mmap したのは 4096。
 *   本来は munmap(p, 4096) が正しい。
 * - ftok("mmap2_r_sem",'a') の path ファイルが存在しないと失敗する。
 * - strncpy(p, count, 4096) で終端保証が曖昧（snprintf が作る文字列自体は終端されるが、
 *   共有メモリ領域の残骸が残る設計なのでプロトコル設計としては要注意）。
 */

int main(int argc, char *argv[]){
   char *p, *ptr, count[4096];
   /*
    * p:
    *   共有メモリ領域を mmap した先頭アドレス。
    *   要求文字列の読み取りと、応答文字列の書き込みに使う。
    *
    * ptr:
    *   未使用（名残変数）。
    *
    * count:
    *   計算した文字数 n を文字列として格納するバッファ。
    *   例: n=5 → count="5"
    */

   int fd, ret1, semid, n, i;
   /*
    * fd:
    *   shm_open が返す FD（共有メモリオブジェクトのハンドル）。
    *
    * semid:
    *   semget が返すセマフォ集合ID（セマフォは1個）。
    *
    * ret1:
    *   strcmp の結果などに使用。
    *
    * n:
    *   strlen による文字数。
    *
    * i:
    *   未使用（名残変数）。
    */

   key_t semkey;
   /*
    * semkey:
    *   ftok により生成する System V IPC キー。
    *   相手プロセスと同じ key を使うことで同一セマフォ集合を共有する。
    */

   struct sembuf buf;
   /*
    * semop で行う操作指定。
    *   sem_num: 何番目のセマフォを操作するか（0番）
    *   sem_op : セマフォ値に対する加減算/待ち条件
    *   sem_flg: フラグ（0=ブロック待ち）
    */

   semkey = ftok("mmap2_r_sem", 'a');
   /*
    * ftok(path, proj_id):
    *   path と proj_id から “同じなら同じキーになる” 値を生成する。
    *
    * 注意:
    *   path で指定したファイルが存在する必要がある。
    *   （無いと ftok が失敗し semkey が -1 になる可能性がある）
    */

   semid = semget(semkey, 1, IPC_CREAT | IPC_EXCL | 0666);
   /*
    * semget(key, nsems, semflg):
    *   nsems=1 なのでセマフォ1個の集合。
    *
    * IPC_CREAT|IPC_EXCL:
    *   「存在しなければ作成」「存在すればエラー」。
    *   先に起動したプロセスだけが作成者になる。
    */

   if(semid < 0){
      /*
       * すでに存在していた場合（EEXIST など）は取り直す。
       */
      semid = semget(semkey, 1, IPC_CREAT | 0666);
   }
   else{
      /*
       * 新規作成者なら初期値をセット。
       * ここでは 1 にしている。
       *
       * ただし、相手側も同様に “作成したら SETVAL(1)” しているため、
       * どちらが先に作成者になるかで初期状態が決まる設計。
       */
      semctl(semid, 0, SETVAL, 1);
   }

   buf.sem_num = 0; // セマフォ0番を使う
   buf.sem_flg = 0; // ブロック待ち

   fd = shm_open("/shared_memory", O_RDWR, 0666); // O_RDONLY->O_RDWR
   /*
    * shm_open(name, oflag, mode):
    *   "/shared_memory" を読み書きで開く。
    *
    * ここが O_RDONLY ではなく O_RDWR なのは、
    *   - 要求を読むだけでなく
    *   - 応答（count文字列）を書き戻す
    * 必要があるため。
    *
    * 前提:
    *   共有メモリは相手側が O_CREAT|O_RDWR で作成済み、かつ ftruncate 済み。
    *   （本プログラムはサイズ設定をしていない）
    */

   if(fd == -1){
      fprintf(stderr, "shm_open failed\n");
      exit(1);
   }

   p = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   /*
    * mmap により共有メモリを 4096 バイトマップする。
    *
    * PROT_READ|PROT_WRITE:
    *   読む（要求）＋書く（応答）ため両方許可。
    *
    * MAP_SHARED:
    *   書いた内容が相手プロセスから見える。
    */

   if(p == MAP_FAILED){
      perror("mmap");
      return 1;
   }

   while(1){
      buf.sem_op = -2;
      semop(semid, &buf, 1);
      /*
       * sem_op = -2:
       *   セマフォ値が 2以上になるまで待ち、2減らす。
       *
       * つまり「相手がセマフォを +2 することでこの側が動き出せる」
       * という “プロトコル” を表している。
       *
       * この動作が成立するには、相手側が適切なタイミングで +2 を行っている必要がある。
       * （前に出てきた入力側プログラムが buf.sem_op=2 を使っていたので整合している）
       */

      fprintf(stderr, "-%s-\n", p);
      /*
       * 現在の共有メモリ内容（要求文字列）を表示。
       * 例: "-Apple-"
       *
       * 重要:
       *   共有メモリ上のデータがヌル終端された文字列であることが前提。
       */

      ret1 = strcmp(p, "exit");
      /*
       * 要求が "exit" なら終了する。
       * strcmp は一致なら 0。
       */

      if(ret1 == 0) break;

      n = strlen(p);
      /*
       * 共有メモリ上の要求文字列の長さを計算。
       * 例: "Apple" → 5
       */

      snprintf(count, sizeof(count), "%d", n);
      /*
       * n を文字列化して count に格納。
       * 例: n=5 → count="5"
       *
       * snprintf は必ず終端 '\0' を付ける（バッファサイズ内なら）。
       */

      fprintf(stderr, "%s\n", count);
      /*
       * 計算結果を表示（デバッグ用途）。
       * 例: "5"
       */

      strncpy(p, count, 4096);
      /*
       * 応答として、共有メモリ領域 p の内容を count に置き換える。
       *
       * これにより相手側（入力側）は p を読むことで結果を受け取れる。
       *
       * 注意:
       *   - count は短い文字列なので、共有メモリの残り領域には以前のデータが残る可能性がある。
       *   - ただし文字列として読むときは '\0' で止まるので見かけ上は問題になりにくい。
       *   - より堅牢にするなら memset(p,0,4096) → strcpy(p,count) のようにゼロクリアする手もある。
       */

      /*fprintf(stderr,"+%s+\n",p);
      */
      buf.sem_op = 1;
      semop(semid, &buf, 1);
      /*
       * sem_op = +1:
       *   セマフォ値を 1増やす。
       *
       * これは相手側へ「応答を書いたので次へ進め」という通知（解放）に相当する。
       * （相手側が -1 で待っているなら、ここで動けるようになる）
       */
   }

   if(munmap(p, sizeof(char)) == -1){
      /*
       * munmap(addr, length):
       *   mmap した領域をアンマップする。
       *
       * 注意（重要）:
       *   length が sizeof(char)=1 になっているが、mmap したのは 4096。
       *   本来は munmap(p, 4096) が正しい。
       *
       * このままだと EINVAL で失敗する可能性がある。
       */
      perror("munmap");
   }

   //if(semctl(semid,0,IPC_RMID)!=0){ perror("semctl"); }
   /*
    * セマフォ削除はコメントアウトされている。
    * どちらが削除責任を持つかは設計次第だが、
    * 片方だけが削除すると相手が動作中に壊れる可能性があるため、
    * 「最後に終わる側が消す」などルールが必要。
    *
    * 実行例では ipcs -s で残ったセマフォを確認し、
    * ipcrm -s で手動削除している。
    */

   close(fd);

   shm_unlink("/shared_memory");
   /*
    * shm_unlink(name):
    *   共有メモリオブジェクト名を削除する（unlink）。
    *   ファイルの unlink と同様、参照が残っていれば即消えるとは限らない。
    *
    * ここではこのプロセスが “掃除役” として unlink している設計に見える。
    */

   return 0;
}

/*
 * 実行例（コメントより）:
 *
 *   -Apple-
 *   5
 *   -Ball-
 *   4
 *   -exit-
 *
 * その後:
 *   ipcs -s でセマフォが残っているのを確認し、
 *   ipcrm -s <semid> で削除している。
 *
 * これは semctl(IPC_RMID) がコメントアウトされているためである。
 */
