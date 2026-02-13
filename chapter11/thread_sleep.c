#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

/*
 * このプログラムは pthread を使って、
 * 「重い処理（20秒かかる処理）」を別スレッドで実行しながら、
 * メインスレッド側で “処理中表示（スピナー的な進捗表示）” を行うサンプルである。
 *
 * 高レベルの流れ:
 *
 *   mainスレッド:
 *     1) flag=0 を用意
 *     2) processing スレッドを作成し、flag のアドレスを渡す
 *     3) flag が 1 になるまで、1秒おきに "." を表示し続ける
 *     4) flag==1 になったらループを抜け、pthread_join でスレッド終了を待つ
 *
 *   processingスレッド:
 *     1) sleep(20) で “重い処理” を模擬
 *     2) flag（x が指す int）に 1 を書き込む（完了通知）
 *     3) "done." を出力して終了
 *
 * --------------------------------------------------------------------
 * 重要ポイント（スレッドと共有変数）:
 *
 * - main と processing は同じメモリ空間を共有するため、
 *   main のローカル変数 flag のアドレスを processing に渡すと、
 *   processing 側から flag を書き換えられる（共有状態）。
 *
 * - ただし、厳密には「共有変数 flag を複数スレッドが同時に読む/書く」ので
 *   データ競合（data race）の可能性がある。
 *   学習用の簡単な例としては動くことが多いが、実務では
 *     - mutex
 *     - atomic
 *     - condition variable
 *   などで同期するのが基本である。
 *
 * - pthread_join(th,NULL) は
 *   「作成したスレッドが終わるまで待つ（合流する）」操作であり、
 *   join しないとスレッド資源の回収（スレッドの終了処理）ができずリークになる。
 */

void *processing(void *x);

int main(){
   pthread_t th;
   int i, flag = 0;

   /*
    * pthread_create(&th, attr, start_routine, arg):
    *   - th: 作成したスレッドIDが入る
    *   - attr: NULL ならデフォルト属性
    *   - start_routine: スレッドが実行する関数（processing）
    *   - arg: processing に渡す引数（void*）
    *
    * ここでは flag のアドレス (&flag) を渡して、
    * processing 側から「完了したら flag=1」にできるようにしている。
    */
   pthread_create(&th, NULL, processing, &flag);

   /*
    * ここからメインスレッドは “処理中表示” を行う。
    * processing スレッドが 20秒後に flag を 1 にするまで回り続ける。
    */
   fprintf(stderr, "processing");

   i = 1;
   while(1){
      /*
       * processing スレッドが *(int*)x = 1 を実行すると、
       * ここで flag == 1 となりループを抜ける。
       */
      if(flag == 1) break;

      /*
       * 表示の工夫:
       *   i が 4 の倍数のときだけ "\r"（キャリッジリターン）を使って
       *   同じ行を上書きする形にしている。
       *
       * "\r" は「行頭に戻る」制御文字であり、
       *   "\rprocessing    \rprocessing"
       * とすると、いったん "processing    " を表示して文字を消すように見せつつ、
       * 再度 "processing" を上書きする効果がある（簡易アニメーション）。
       */
      if(i % 4 == 0) fprintf(stderr, "\rprocessing    \rprocessing");
      else fprintf(stderr, ".");

      /*
       * 1秒待つことで表示が速すぎず、人間に見える速度になる。
       * 同時に CPU を無駄に回し続ける “ビジーループ” を多少緩和している。
       */
      sleep(1);

      i++;
   }

   /*
    * pthread_join(th, retval):
    *   指定スレッドが終了するまで待つ（合流）。
    *   ここで join することで、スレッドの終了を確実に待ち、
    *   スレッドが持つ資源を回収できる。
    *
    * ※このコードは flag==1 になってから join するので、
    *   通常はすでにスレッドが終わっており、すぐ戻る。
    */
   pthread_join(th, NULL);

   return 0;
}

void *processing(void *x){
   /*
    * この関数は別スレッドで動く。
    * 引数 x は void* なので、元の型にキャストして使う。
    * ここでは &flag（int*）が渡されているため、(int*) にキャストする。
    */

   sleep(20); // 重い処理に見立てる（20秒かかる計算などを模擬）

   /*
    * *(int *)x = 1;
    *   x が指す int（つまり main の flag）に 1 を書き込む。
    *   これが main スレッドへの “完了通知” になっている。
    *
    * 注意:
    *   厳密にはこの共有書き込みは同期が無いのでデータ競合になり得る。
    *   学習段階では「共有メモリで通信できる」例として捉える。
    */
   *(int *)x = 1;

   /*
    * 改行して done を表示。
    * メインスレッドが "." を出している途中でも割り込むため、
    * 出力が混ざる（表示が崩れる）ことがあるのはスレッド出力の自然な副作用。
    */
   fprintf(stderr, "\ndone.\n");

   return NULL;
}
