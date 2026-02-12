#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

/*
 * stdio.h
 *   fprintf(), scanf() を使用。
 *
 * stdlib.h
 *   srand(), rand() を使用。
 *
 * signal.h
 *   signal(), SIGALRM を使用。
 *
 * unistd.h
 *   alarm() を使用。
 *
 * time.h
 *   time() を使用。
 *
 * このプログラムは、
 *   5秒以内に足し算問題へ回答するゲームである。
 *
 * 5秒経過すると SIGALRM が発生し、
 * func() が呼び出される。
 */

void func(int x);
/*
 * SIGALRM を受信したときに実行される
 * シグナルハンドラ。
 */

int main(){

   int num1, num2;     // 問題用の乱数
   int ans;            // 正解
   int in_ans;         // ユーザ入力

   signal(SIGALRM, func);
   /*
    * signal(SIGALRM, func)
    *
    * SIGALRM が発生したときに
    * func() を呼び出すよう登録。
    *
    * SIGALRM:
    *   alarm() によって発生するタイマーシグナル。
    */

   srand(time(NULL));
   /*
    * 乱数初期化。
    *
    * time(NULL):
    *   現在時刻（秒）を取得。
    *
    * 同じ乱数列にならないようにする。
    */

   num1 = rand() % 100;
   num2 = rand() % 100;
   /*
    * 0〜99 の乱数生成。
    */

   ans = num1 + num2;
   /*
    * 正解を計算。
    */

   fprintf(stderr, "%d + %d ? ", num1, num2);
   /*
    * 問題表示。
    */

   alarm(5);
   /*
    * alarm(秒数)
    *
    * 指定秒数後に SIGALRM を送信する。
    *
    * ここでは 5 秒後に SIGALRM 発生。
    *
    * カーネル内部動作:
    *
    *   1. プロセスにタイマー設定
    *   2. 5秒経過
    *   3. カーネルが SIGALRM を配送
    */

   scanf("%d", &in_ans);
   /*
    * ユーザ入力待機。
    *
    * もし 5 秒以内に入力されなければ、
    * SIGALRM が発生し func() が呼ばれる。
    *
    * scanf はシグナルにより中断される可能性がある。
    */

   alarm(0);
   /*
    * alarm(0)
    *
    * タイマー解除。
    *
    * 入力が間に合った場合、
    * 残っているタイマーをキャンセルする。
    */

   if(ans == in_ans){
      fprintf(stderr, "You got it.\n");
   }
   else{
      fprintf(stderr, "That's wrong.\n");
   }

   return 0;
}

/*
 * SIGALRM ハンドラ
 *
 * x:
 *   受信したシグナル番号（通常 14）
 */
void func(int x){

   fprintf(stderr, "Time is up!\n");
   /*
    * タイムアップ通知。
    *
    * 注意:
    *   シグナルハンドラ内では
    *   非同期安全関数のみ使用すべき。
    *
    * 本例は教育用の簡易実装。
    */
}
