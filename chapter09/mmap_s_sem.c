#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>

/*
 * このプログラムは
 *   - POSIX 共有メモリ (shm_open + mmap)
 *   - System V セマフォ (semget + semop + semctl)
 * を組み合わせて、別プロセスと「要求→応答」を同期しながらやり取りする例である。
 *
 * 目的（コメントにある通り）:
 *   入力:  Apple
 *   応答:  5
 * のように、共有メモリに「文字列」を書き、相手プロセスが「結果文字列（数値）」を書き戻し、
 * それを表示する。
 *
 * 共有メモリ単体だと「いつ読んでいいか / いつ書き終わったか」が分からないため、
 * セマフォで同期（手順の順番保証）を行っているのが本質。
 *
 * ただし、このセマフォの使い方は少し独特で、
 *   1個のセマフォ値を状態変数のように使い、
 *   sem_op = -1 / +2 / +1 などで値を動かしてフェーズを切り替える設計になっている。
 *
 * --------------------------------------------------------------------
 * 【全体アルゴリズム（高レベル）】
 *
 * 共有資源:
 *   - 共有メモリ "/shared_memory"（4096バイト）
 *   - セマフォ 1個（semid の sem[0]）
 *
 * 本プログラム（ここでは“入力側”= クライアント的）のループは概ね:
 *
 *   1) セマフォ待ち（入力してよいフェーズを待つ）
 *   2) ユーザ入力 command を読む
 *   3) 共有メモリ p に command を書く（要求送信）
 *   4) セマフォを進める（相手に「処理してよい」と通知）
 *   5) セマフォ待ち（相手の処理完了を待つ）
 *   6) 共有メモリ p から結果を表示する（応答受信）
 *   7) セマフォを戻す（次の要求へ）
 *
 * つまり「共有メモリ＝データ本体」「セマフォ＝順番制御」という役割分担。
 *
 * --------------------------------------------------------------------
 * 【相手プロセス（推測）】
 * 出力例から、相手は p に書かれた "Apple" を読み取り、
 * p に "5" のような結果文字列を書いて返していると推測される。
 * （※このコード単体では相手は存在しないので、別途 mmap2_r_sem のようなプログラムが必要）
 *
 * --------------------------------------------------------------------
 * 【注意点】
 * - セマフォの状態遷移が “慣習的な二値セマフォ” ではなく、値を段階的に動かしているため、
 *   相手プログラムと「同じプロトコル」を厳密に共有していないとデッドロックする。
 * - strncpy は必ずヌル終端するとは限らないため、出力を文字列扱いするなら終端保証が欲しい。
 * - semget(IPC_CREAT|IPC_EXCL) → 失敗なら既存を取得、という典型パターンになっている。
 *
 * --------------------------------------------------------------------
 * 【コンパイル】
 *   gcc mmap2_s_sem.c -o mmap2_s_sem -lrt
 *   ※ shm_open/ftruncate 等で -lrt が必要な環境がある（古いglibcなど）
 */

int main(){
   char *p, line[4096], command[4096];
   /*
    * p:
    *   共有メモリを mmap した先頭アドレス（共有領域へのポインタ）。
    *
    * line:
    *   stdin から読み取る行バッファ。
    *
    * command:
    *   改行を除いた入力文字列（実際に共有メモリへ書く要求）。
    */

   int fd, ret1, ret2, ret3, tmp, i, semid;
   /*
    * fd:
    *   shm_open が返す共有メモリオブジェクトのFD（ファイルのように扱える）。
    *
    * semid:
    *   semget が返す System V セマフォ集合のID。
    *
    * ret1,ret2,ret3:
    *   各種戻り値チェック用。
    *
    * tmp,i:
    *   このコードでは未使用（途中の試行の名残の可能性が高い）。
    */

   key_t semkey;
   /*
    * semkey:
    *   ftok で生成する System V IPC 用キー。
    *   セマフォを「別プロセスと同じキーで共有」するために使う。
    */

   struct sembuf buf;
   /*
    * sembuf:
    *   semop で行う操作（どのセマフォ番号に、どんな加減算をするか）を指定する構造体。
    *
    *   buf.sem_num : セマフォ集合の中の何番目を操作するか（ここでは 0）
    *   buf.sem_op  : セマフォ値に対して行う操作
    *                -1: 値が 1以上になるまで待ってから 1減らす（P操作に相当）
    *                +1: 1増やす（V操作に相当）
    *                +2: 2増やす（このコード独特の“状態遷移”）
    *   buf.sem_flg : フラグ（0 はデフォルト、IPC_NOWAIT 等も指定可能）
    */

   semkey = ftok("mmap2_r_sem",'a');
   /*
    * ftok(path, id):
    *   System V IPC 用のキーを生成する。
    *
    * 注意:
    *   path で指定したファイル（ここでは "mmap2_r_sem"）が存在しないと失敗する。
    *   つまり、相手側も同じ ftok をするなら、このファイルは両者から見える必要がある。
    *
    * 'a' はプロジェクトIDのようなもので、同じ path + 同じ id なら同じ key が生成される。
    */

   semid = semget(semkey, 1, IPC_CREAT | IPC_EXCL | 0666);
   /*
    * semget(key, nsems, semflg):
    *
    * nsems=1:
    *   セマフォを1個だけ持つセマフォ集合を作る/取得する。
    *
    * IPC_CREAT|IPC_EXCL:
    *   「存在しなければ作る」かつ「すでに存在するならエラーにする」。
    *   よって、最初に起動したプロセスだけが “作成者” になれる。
    */

   fprintf(stderr, "semid=%d\n", semid);

   if(semid < 0){
      /*
       * すでに同じ key のセマフォ集合が存在していた場合、
       * IPC_EXCL により semget は失敗する（典型的には errno=EEXIST）。
       * その場合は既存のセマフォ集合を取り直す。
       */
      semid = semget(semkey, 1, IPC_CREAT | 0666);
      fprintf(stderr, ">semid=%d\n", semid);
   }
   else{
      /*
       * このプロセスが “新規作成者” の場合のみ、初期値をセットする。
       *
       * semctl(semid, semnum, SETVAL, val):
       *   指定セマフォの値を val に設定する。
       *
       * ここでは初期値を 1 にしている。
       * つまり「最初は誰か1回だけ通れる」状態から開始。
       */
      semctl(semid, 0, SETVAL, 1);
   }

   buf.sem_num = 0;  // セマフォ集合の 0 番目を操作
   buf.sem_flg = 0;  // デフォルト（ブロック待ちあり）

   fd = shm_open("/shared_memory", O_CREAT | O_RDWR, 0666);
   /*
    * shm_open(name, oflag, mode):
    *   POSIX共有メモリオブジェクトを作成/オープンする。
    *
    * name:
    *   "/shared_memory"（POSIX共有メモリの名前空間）
    *
    * oflag:
    *   O_CREAT | O_RDWR → 無ければ作り、読み書きで開く
    */

   if(fd == -1){
      fprintf(stderr, "shm_open failed\n");
      exit(1);
   }

   ret1 = ftruncate(fd, sizeof(line));
   /*
    * ftruncate(fd, size):
    *   共有メモリオブジェクトのサイズを確保する。
    *   これをしないと mmap 後のアクセスで SIGBUS になることがある。
    *
    * ※このコードは ret1 のエラーチェックが無い（教材として簡略化）。
    */

   p = mmap(0, sizeof(line), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
   /*
    * mmap(addr, length, prot, flags, fd, offset):
    *
    * PROT_WRITE|PROT_READ:
    *   読み書き可能としてマップする（要求を書き、応答を読むため）。
    *
    * MAP_SHARED:
    *   変更が共有され、相手プロセスから見える。
    */

   if(p == MAP_FAILED){
      perror("mmap");
      return 1;
   }

   while(1){
      /*
       * ここから要求→応答のループ。
       * セマフォ操作（semop）で “いつ入力し、いつ表示するか” を同期する。
       *
       * このプロトコルは相手側も一致している必要がある。
       */

      buf.sem_op = -1;
      semop(semid, &buf, 1);
      /*
       * sem_op = -1:
       *   セマフォ値が 1以上になるまで待ち、1減らす。
       *
       * 直感的には「ロックを取る」「自分の番が来るまで待つ」に相当。
       */

      fprintf(stderr, "> ");
      fgets(line, sizeof(line), stdin);

      ret2 = sscanf(line, "%[^\n]", command);
      /*
       * 改行を除いた入力を command に格納。
       * ret2 < 0 の場合 break しているが、
       * sscanf の返り値仕様的には空入力などで 0 が返るケースもある。
       * （教材として大きくは問題にしない）
       */

      if(ret2 < 0) break;

      ret3 = strcmp(command, "exit");
      /*
       * "exit" ならループを抜けるための判定。
       * strcmp は一致なら 0 を返す。
       */

      strncpy(p, command, sizeof(command));
      /*
       * 共有メモリ領域 p に要求文字列 command をコピーする。
       * これにより相手プロセスが同じ共有メモリを mmap していれば、
       * p の内容を読んで処理できる。
       *
       * 注意:
       *   strncpy は終端 '\0' を保証しない場合がある。
       *   相手が文字列として扱うなら、終端保証（明示的に p[4095]=0 等）が安全。
       */

      buf.sem_op = 2;
      semop(semid, &buf, 1);
      /*
       * sem_op = +2:
       *   セマフォ値を 2 増やす。
       *
       * ここが “状態機械”的な使い方になっているポイント。
       * 「相手に処理開始を通知する」意図で値を大きく動かしている。
       *
       * 通常の二値セマフォなら +1 で十分なことが多いが、
       * このコードは相手側との合意プロトコルにより +2 を使っていると考えられる。
       */

      if(ret3 == 0){
         /*
          * "exit" の場合、要求を書いた時点で終了。
          * （相手側も "exit" を見て終了する設計かもしれない）
          */
         break;
      }

      buf.sem_op = -1;
      semop(semid, &buf, 1);
      /*
       * もう一度 sem_op = -1 で待つ。
       * これは「相手が応答を書き終えるまで待つ」意図と推測できる。
       * （相手側がどのタイミングで sem を増やすかが重要）
       */

      fprintf(stderr, "=%s <-> %s=\n", command, p);
      /*
       * command（自分が送った要求）と p（共有メモリ上の応答）を表示。
       *
       * 出力例:
       *   =Apple <-> 5=
       *
       * つまり相手は p を "5" のような文字列に書き換えて返している構造。
       */

      buf.sem_op = 1;
      semop(semid, &buf, 1);
      /*
       * sem_op = +1:
       *   セマフォ値を 1 増やす。
       *
       * 次のループへ進むための解放（相手への通知）と推測できる。
       * この 1 / 2 / -1 の組み合わせで状態遷移を作っている。
       */
   }

   if(munmap(p, sizeof(line)) == -1){
      /*
       * マッピング解除。
       * このプロセスの仮想メモリ空間から共有領域を剥がす。
       */
      perror("munmap");
   }

   if(semctl(semid, 0, IPC_RMID) != 0){
      /*
       * semctl(semid, 0, IPC_RMID):
       *   セマフォ集合を削除する（破棄）。
       *
       * 注意:
       *   相手プロセスがまだこのセマフォを使っている最中だと、
       *   想定外のエラーや同期崩壊が起きる可能性がある。
       *
       * どちらが削除責任を持つか（最後に終了する側が消す等）は設計の問題。
       */
      perror("semctl");
   }

   close(fd);
   /*
    * shm_open の FD を閉じる。
    * 共有メモリオブジェクトの unlink はここでは行っていないため、
    * 名前自体は残る（別途 shm_unlink が必要）。
    */

   return 0;
}

/*
 * 実行例（コメントより）:
 *
 *   > Apple
 *   =Apple <-> 5=
 *   > Ball
 *   =Ball <-> 4=
 *   > exit
 *
 * この結果から、相手側プロセスが
 *   - 共有メモリから文字列を読み取り
 *   - 文字数を計算して
 *   - 共有メモリへ結果（数値文字列）を書き戻す
 * と推測できる。
 */
