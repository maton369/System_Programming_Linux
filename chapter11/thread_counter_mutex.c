#include <stdio.h>
#include <pthread.h>

#define COUNT 1000000
#define TH_N 5

/*
 * このプログラムは、複数スレッドで共有カウンタを増やす際に
 * pthread_mutex_t（ミューテックス）で排他制御を行い、
 * “正しい合計値” を得る例である。
 *
 * 前の（mutex無し）版では、
 *   c++ が原子的でないためロストアップデートが発生し、
 *   期待値 TH_N*COUNT に届かないことがあった。
 *
 * 本版では、
 *   pthread_mutex_lock / pthread_mutex_unlock
 * で「c++ の区間を同時に1スレッドしか実行できない」ようにし、
 * 期待値通り 5,000,000 を得ることができる。
 *
 * --------------------------------------------------------------------
 * 【全体の狙い】
 *
 * - 共有変数 c への更新を、ミューテックスで “臨界区間” として保護する
 * - これによりデータ競合（data race）を排除し、正しい結果を保証する
 *
 * --------------------------------------------------------------------
 * 【ミューテックスとは】
 *
 * - mutual exclusion（相互排他）の略
 * - 「ある区間（臨界区間）を同時に実行できるのは1スレッドだけ」にする仕組み
 *
 * 典型形:
 *   pthread_mutex_lock(&mut);
 *   // 共有資源の操作（臨界区間）
 *   pthread_mutex_unlock(&mut);
 *
 * --------------------------------------------------------------------
 * 【性能面の重要ポイント】
 *
 * コメントの実測結果:
 *   mutex あり → 5000000（正しい）
 *   ただし real が 50 秒程度と非常に遅い
 *
 * 理由:
 * - c++ のたびに lock/unlock を行うため、合計で
 *     TH_N * COUNT = 5,000,000 回
 *   lock/unlock を実行することになる。
 * - lock/unlock はOSスケジューリングやキャッシュ同期のコストを含み、
 *   1回の c++ に対して過剰なオーバーヘッドになりやすい。
 *
 * 学習的には:
 * - 「正しさを得ると遅くなる」ことを体験する良い例
 * - 次の改善として “ローカルに数えて最後に足す” などの設計が考えられる
 */

pthread_mutex_t mut; // 全スレッドで共有するミューテックス（臨界区間の門番）

void *counter(void *x);

int main(){
   int i = 0;
   int c = 0;                 // 共有カウンタ（全スレッドで同じ &c を参照）
   pthread_t th[TH_N];

   /*
    * pthread_mutex_init(&mut, attr):
    *   ミューテックスの初期化。
    *   attr を NULL にするとデフォルト属性。
    *
    * これを呼ばずに lock/unlock すると未定義動作になり得る。
    */
   pthread_mutex_init(&mut, NULL);

   /*
    * TH_N 個のスレッドを生成する。
    * 引数として &c を渡すので、全スレッドが同じ共有変数を更新する。
    */
   for(i = 0; i < TH_N; i++){
      pthread_create(&th[i], NULL, counter, &c);
   }

   /*
    * 全スレッドが終了するまで待つ（合流）。
    * join 後には c の更新がすべて終わっていることが保証される。
    */
   for(i = 0; i < TH_N; i++){
      pthread_join(th[i], NULL);
   }

   /*
    * 結果表示:
    *   mutex によりロストアップデートが防げるため、
    *   期待値 TH_N*COUNT = 5,000,000 が得られる。
    */
   fprintf(stderr, "%d\n", c);

   /*
    * 実務的には pthread_mutex_destroy(&mut); で破棄するのが作法。
    * （プロセス終了で OS が回収するので教材では省略されがち）
    */
   return 0;
}

void *counter(void *x){
   int i;

   /*
    * 各スレッドが COUNT 回だけ共有カウンタを増やす。
    * ただし毎回 lock/unlock することで安全性を確保している。
    */
   for(i = 0; i < COUNT; i++){
      /*
       * lock:
       *   すでに他スレッドが mut を保持している場合は待たされる。
       *   ここから unlock までが臨界区間。
       */
      pthread_mutex_lock(&mut);

      /*
       * 共有変数の更新（臨界区間）:
       *   ここを同時に実行できるのは1スレッドだけなので、
       *   c++ の load/add/store が衝突せず、増分が失われない。
       */
      (*(int *)x)++;

      /*
       * unlock:
       *   臨界区間を抜け、待っている別スレッドが lock できるようになる。
       */
      pthread_mutex_unlock(&mut);
   }

   return NULL;
}

/*
実行例（コメント）:

sasayama-no-MacBook-Air:Syoseki sasayama$ time ./thread_counter_mutex
5000000

real    0m50.078s
user    0m6.032s
sys     0m47.636s

解釈:
- 結果は正しい（5,000,000）
- しかし lock/unlock を 5,000,000 回やっているため非常に遅い
  （特に sys が大きいのはスレッド同期のオーバーヘッドが効いている可能性が高い）
*/
