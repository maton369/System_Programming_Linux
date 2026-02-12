#include <stdio.h>
#include <stdlib.h>
#include <signal.h>  // シグナル関連定義
#include <time.h>
#include <unistd.h>
/*
 * stdio.h:
 *   fprintf() を使用。
 *
 * stdlib.h:
 *   exit() を使用。
 *
 * signal.h:
 *   signal(), SIGINT などのシグナル定義を使用。
 *
 * time.h:
 *   time() を使用。
 *
 * unistd.h:
 *   sleep() を使用。
 *
 * このプログラムは、
 *   Ctrl+C (SIGINT) を受信したときに
 *   実行時間を表示して終了する。
 */

time_t start;
/*
 * グローバル変数。
 *
 * プログラム開始時刻を保存する。
 *
 * シグナルハンドラ内から参照するため、
 * グローバルに定義している。
 */

void stop(int x);
/*
 * シグナルハンドラ関数。
 *
 * signal() によって登録され、
 * SIGINT 受信時に自動的に呼び出される。
 */

int main(){

   time(&start);
   /*
    * 現在時刻を取得。
    *
    * UNIX time:
    *   1970年1月1日からの経過秒数。
    *
    * start にプログラム開始時刻を保存。
    */

   signal(SIGINT, stop);
   /*
    * signal(シグナル番号, ハンドラ関数)
    *
    * SIGINT:
    *   Ctrl+C が押されたときに送られるシグナル。
    *
    * stop:
    *   SIGINT 受信時に呼び出される関数。
    *
    * これにより、
    *   Ctrl+C → stop() 実行
    * となる。
    */

   while(1){
      /*
       * 無限ループ。
       *
       * シグナルが来るまで継続。
       */

      sleep(1);
      /*
       * 1秒待機。
       *
       * sleep はシグナルを受信すると
       * 途中で中断される場合がある。
       */

      fprintf(stderr, ".");
      /*
       * 1秒ごとに "." を表示。
       *
       * 経過時間の視覚的表示。
       */
   }

   return 0;
   /*
    * 通常はここに到達しない。
    */
}


/*
 * stop():
 *   SIGINT 受信時に呼び出される。
 *
 * 引数 x:
 *   受信したシグナル番号。
 */
void stop(int x){

   time_t end;

   time(&end);
   /*
    * シグナル受信時刻を取得。
    */

   fprintf(stderr,"\n signal number=%d, time= %ld\n",
           x, end - start);
   /*
    * x:
    *   受信したシグナル番号（通常 2）。
    *
    * end - start:
    *   実行開始からの経過秒数。
    *
    * つまり:
    *   プログラム実行時間を表示している。
    */

   exit(0);
   /*
    * プログラム終了。
    *
    * シグナルハンドラ内から exit() を呼ぶことで
    * 安全に終了している。
    */
}
