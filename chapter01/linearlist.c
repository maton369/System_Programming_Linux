#include <stdio.h>
/*
 * stdio.h:
 *   標準入出力ライブラリ。
 *   scanf(), fprintf(), stderr を使用する。
 *
 * scanf は「値を返す」のではなく、
 * 指定されたアドレスに直接データを書き込む関数である。
 */

/*
 * struct linear:
 *   単方向リスト（singly linked list）の1ノードを表す構造体。
 *
 * data:
 *   このノードが保持するデータ本体。
 *
 * next:
 *   次のノードを指すポインタ。
 *   同じ struct linear 型へのポインタを持つため、
 *   これを「自己参照構造体」と呼ぶ。
 */
struct linear{
   int data;
   struct linear *next;
};

int main(){
   int i;
   /*
    * i:
    *   ループ制御用変数。
    */

   struct linear arr[5], *ptr, *curr;
   /*
    * arr[5]:
    *   struct linear 型の配列（要素数5）。
    *
    *   本来、連結リストは malloc などで動的に確保されることが多いが、
    *   このコードでは理解を容易にするため、
    *   あらかじめ配列として連続領域を確保している。
    *
    *   メモリ配置（概念）:
    *
    *   arr[0] | arr[1] | arr[2] | arr[3] | arr[4]
    *
    * ptr:
    *   現在入力・処理対象となっているノードを指すポインタ。
    *
    * curr:
    *   「直前のノード」を保持するためのポインタ。
    *   next ポインタを設定するために使用される。
    */

   ptr = arr;
   /*
    * 配列名 arr は式中で
    *   「先頭要素 arr[0] のアドレス」
    * として評価される。
    *
    * つまりこの代入は次と等価:
    *
    *   ptr = &arr[0];
    *
    * ptr は現在、最初のノードを指している。
    */

   for(i = 0; i < 5; i++){
      scanf("%d", &ptr->data);
      /*
       * ptr->data:
       *   ptr が指しているノードの data メンバ。
       *
       * scanf は書き込み先アドレスが必要なため、
       * &ptr->data としてアドレスを渡している。
       */

      curr = ptr;
      /*
       * curr に現在のノードの位置を保存。
       *
       * このノードの next を、
       * 「次のノード」に設定するために使う。
       */

      curr->next = ++ptr;
      /*
       * ++ptr:
       *   ポインタ ptr を次の配列要素へ進める。
       *
       * struct linear 型ポインタなので、
       *   sizeof(struct linear) バイト分進む。
       *
       * curr->next = ++ptr; は次を意味する:
       *   1. ptr を次のノードに進める
       *   2. そのアドレスを curr->next に代入する
       *
       * これにより、
       *   arr[i].next → arr[i+1]
       * というリンクが張られる。
       */
   }

   curr->next = NULL;
   /*
    * 最後のノードの next を NULL に設定。
    *
    * これは「リストの終端」を表す重要な操作。
    * next == NULL のノードが最後の要素となる。
    */

   ptr = arr;
   /*
    * 再び ptr を先頭ノードに戻す。
    *
    * 先ほどの for ループ終了時点では、
    * ptr は arr[5]（配列外）を指しているため、
    * 正しく走査するためにリセットが必要。
    */

   while(ptr != NULL){
      /*
       * ptr が NULL でない限りループを続ける。
       *
       * これは単方向リストを走査する
       * 最も基本的な書き方。
       */

      fprintf(stderr, "%d\n", ptr->data);
      /*
       * 現在のノードが保持している data を出力。
       */

      ptr = ptr->next;
      /*
       * next ポインタをたどって、
       * 次のノードへ移動。
       *
       * 最後のノードでは next == NULL となり、
       * ループが終了する。
       */
   }

   return 0;
   /*
    * プログラムの正常終了。
    */
}
