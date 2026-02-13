#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define DATA_N 100000
#define LOOP_N DATA_N/10

/*
 * このプログラムは pthread を用いて、
 *   - 1つのスレッド(input)で「重い計算（ループ処理）」を実行しながら
 *   - 別スレッド(query)で「途中経過の問い合わせ」を受け付ける
 * という “並行実行” の例になっている。
 *
 * 具体的には、input スレッドが count を増やし続け、
 * query スレッドが Enter キー入力のたびに count を表示する。
 *
 * --------------------------------------------------------------------
 * 【全体の狙い（高レベル）】
 *
 *   input スレッド:
 *     - 大きな配列 data を何回も “初期化っぽく” 触りながら
 *     - 進捗カウンタ count を更新する
 *
 *   query スレッド:
 *     - ユーザが Enter を押すたびに
 *     - 現在の count を表示する（進捗照会）
 *
 * つまり「処理が走っている間も、別スレッドでユーザインタラクションを受けられる」
 * という並行処理の基本パターンである。
 *
 * --------------------------------------------------------------------
 * 【重要な注意点（教材としての論点）】
 *
 * 1) データ型のミス/不整合:
 *    main で data は double data[DATA_N]; としているが、
 *    input では ((int *)x)[j] = 0.0; と int* にキャストして代入している。
 *    これは型不整合であり、メモリの解釈が壊れる（未定義動作）可能性がある。
 *
 *    本来は
 *      ((double *)x)[j] = 0.0;
 *    のように double* として扱うべき。
 *
 * 2) count は共有変数だが同期していない:
 *    input が count++ を行い、query が count を読む。
 *    これは複数スレッドからの読み書きであり、厳密には data race の可能性がある。
 *    学習用としては動くことが多いが、実務では atomic や mutex で保護する。
 *
 * 3) join しているのが th1 だけ:
 *    main は th2（query）を join せずに終了する設計になっている。
 *    ただし th1 が終わると main は return するため、プロセスが終了し、
 *    query スレッドもまとめて終了する（強制終了）形になる。
 *    「最後まで query を生かす」なら th2 も join するか、終了条件を作る必要がある。
 */

long count;              // 進捗（作業量）を表す共有カウンタ
void *input(void *x);    // 重い処理（計算/初期化）担当
void *query(void *x);    // 途中経過を表示する UI 担当

int main(int argc, char *argv[]){
  int i, j;              // 未使用（名残変数）
  double data[DATA_N];   // input スレッドが触る大きな配列（スタック上に確保）
  pthread_t th1, th2;    // スレッドID

  /*
   * count はグローバル（共有）で、input が増やし、query が読む。
   */
  count = 0;

  /*
   * input スレッド起動:
   *   第4引数に data を渡すことで、input 側が data を操作できる。
   */
  pthread_create(&th1, NULL, input, data);

  /*
   * query スレッド起動:
   *   引数は使わないので NULL を渡す。
   *   getchar() でブロッキングしながら、Enter のたびに count を表示する。
   */
  pthread_create(&th2, NULL, query, NULL);

  /*
   * pthread_join(th1):
   *   input スレッド（重い処理）が終わるまで main を待機させる。
   *   th1 が終わると main は return するため、プロセスが終了し th2 も終了する。
   */
  pthread_join(th1, NULL);

  return 0;
}

void *input(void *x){
  int i, j;

  /*
   * “重い処理” の開始表示。
   */
  fprintf(stderr, "Calculating...");

  /*
   * 二重ループ:
   *
   * 外側: LOOP_N 回繰り返す
   * 内側: DATA_N 要素を走査する
   *
   * 合計反復回数（理論）:
   *   LOOP_N * DATA_N
   * ここで LOOP_N = DATA_N/10 なので、
   *   (DATA_N/10) * DATA_N = DATA_N^2 / 10
   * DATA_N=100000なら、かなり大きい回数になる（教材として“重い”）。
   */
  for(i = 0; i < LOOP_N; i++){
     for(j = 0; j < DATA_N; j++){
        /*
         * 本来の意図:
         *   data[j] を 0.0 に初期化する（または計算結果を書き込む）
         *
         * ただし現状コードは int* にキャストしているので不整合。
         * 正しくは:
         *   ((double *)x)[j] = 0.0;
         *
         * 現状:
         *   ((int *)x)[j] = 0.0;
         * は double 配列を int 配列として書き換えるため、メモリ破壊になり得る。
         */
        ((int *)x)[j] = 0.0;

        /*
         * 進捗カウンタを1増やす。
         * query スレッドがこれを読むことで進捗表示ができる。
         *
         * 注意:
         *   同期なしなので厳密には data race の可能性がある。
         */
        count++;
     }
  }

  fprintf(stderr, "\ndone.\n");
  return NULL;
}

void *query(void *x){
  /*
   * このスレッドは無限ループで待ち続ける。
   * getchar() は入力が来るまでブロックするので、
   * CPU を無駄に回すビジーループにはならない。
   *
   * Enter が押されるたびに現在の count を表示する。
   */
  while(1){
     getchar(); // 何かキー入力（主に Enter）を待つ

     /*
      * 現在の count を表示。
      * 改行が無いので、表示が続けて上書きされるように見える場合がある。
      * 見やすくするなら "\n" を付けるのも手。
      */
     fprintf(stderr, "count=%ld", count);
  }

  return NULL;
}
