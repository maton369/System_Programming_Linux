#include <stdio.h>
/*
 * stdio.h:
 *   標準入出力ライブラリ。
 *   scanf(), fprintf(), stderr などを使用するために必要。
 *
 * scanf は「入力先のアドレス」に直接書き込む関数であり、
 * ポインタや構造体との関係理解が重要になる。
 */

/*
 * struct denwacho:
 *   電話帳1件分の情報を表す構造体。
 *
 * 各メンバはメモリ上で連続して配置される
 *（ただし int のアラインメントにより隙間が入る場合あり）。
 */
struct denwacho{
   char name[40];
   /*
    * name:
    *   名前を格納する文字配列。
    *   配列名は式中で先頭要素のアドレスとして扱われる。
    */

   char tel[12];
   /*
    * tel:
    *   電話番号を格納する文字配列。
    *   こちらも char の連続領域。
    */

   int age;
   /*
    * age:
    *   年齢を表す整数値。
    *   int 型なので値そのものを保持する。
    */
};

int main(){
    struct denwacho friend[3], *p;
    /*
     * friend[3]:
     *   struct denwacho 型の配列（要素数3）。
     *
     *   メモリ上では次のように配置される（概念図）:
     *
     *   friend[0] | friend[1] | friend[2]
     *
     *   各要素は struct denwacho 1個分のサイズを持ち、
     *   連続したメモリ領域に並ぶ。
     *
     * p:
     *   struct denwacho 型へのポインタ。
     *   構造体そのものではなく、
     *   「構造体が置かれている場所」を指す。
     */

	int i;
    /*
     * i:
     *   ループ制御用の変数。
     */

    p = friend;
    /*
     * 配列名 friend は式中で
     *   「先頭要素 friend[0] のアドレス」
     * として評価される。
     *
     * つまりこの代入は次と等価:
     *
     *   p = &friend[0];
     *
     * p は現在、配列の先頭要素を指している。
     */

    for(i = 0; i < 3; i++){
      scanf("%s", p->name);
      /*
       * p->name:
       *   p が指している構造体の name メンバ。
       *
       * -> 演算子は、
       *   (*p).name
       * の省略記法。
       *
       * name は char 配列なので、
       * 配列名は先頭アドレスとして scanf に渡される。
       */

      scanf("%s", p->tel);
      /*
       * tel も char 配列。
       * 入力文字列がそのまま構造体内部に書き込まれる。
       *
       * ※ サイズ制限がないため、実用では危険。
       */

      scanf("%d", &p->age);
      /*
       * age は int 型の値。
       *
       * scanf は「書き込み先アドレス」を必要とするため、
       * &p->age としてアドレスを渡している。
       */

      p++;
      /*
       * p++:
       *   ポインタ演算。
       *
       * struct denwacho 型ポインタなので、
       * p は次の構造体要素を指すように進む。
       *
       * 実際には:
       *   p = p + 1;
       *   → sizeof(struct denwacho) バイト分進む
       */
    }

    p = friend;
    /*
     * p を再び配列の先頭に戻す。
     *
     * 先ほどのループで p は friend[3] の位置まで
     * 進んでいるため、そのままでは参照できない。
     */

    for(i = 0; i < 3; i++){
      fprintf(stderr, "p->name=%s\n", p->name);
      /*
       * 構造体ポインタを通じて name メンバを参照。
       * 出力時は値を渡すため & は不要。
       */

      fprintf(stderr, "p->tel=%s\n", p->tel);
      /*
       * tel メンバも同様に文字列として出力。
       */

      fprintf(stderr, "p->age=%d\n", p->age);
      /*
       * age は int 型なので %d で出力。
       */

      p++;
      /*
       * 次の構造体要素へ移動。
       */
    }

    return 0;
    /*
     * プログラムの正常終了。
     * 戻り値 0 は OS に対して「正常終了」を意味する。
     */
}
