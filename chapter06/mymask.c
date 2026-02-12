#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
/*
 * sys/types.h:
 *   mode_t 型などの基本型定義を提供。
 *
 * sys/stat.h:
 *   stat(), chmod(), struct stat を使用するために必要。
 *
 * unistd.h:
 *   POSIX 系関数群の宣言。
 *
 * このプログラムは、
 *   1. stat() でファイルのメタデータ取得
 *   2. st_mode を加工
 *   3. chmod() でパーミッション変更
 * を行う。
 */

int main(int argc, char *argv[]) {

   mode_t mode;
   /*
    * mode_t:
    *   ファイルのモード（パーミッション + 種別）を表す型。
    *
    * st_mode には以下が含まれる:
    *   - ファイル種別ビット
    *   - 所有者/グループ/その他のパーミッションビット
    */

   struct stat buf;
   /*
    * struct stat:
    *   inode のメタデータを格納する構造体。
    *
    * st_mode:
    *   ファイル種別 + パーミッション情報を保持。
    */

   int ret;

   ret = stat(argv[1], &buf);
   /*
    * stat(path, &buf)
    *
    * 指定ファイルの inode 情報を取得する。
    *
    * 成功: 0
    * 失敗: -1
    */

   if(ret < 0){
      perror("stat");
      return 1;
   }

   /*
    * st_mode の構造（概念図）
    *
    *  15         12 11  9 8  6 5  3 2  0
    *  | ファイル種別 | 所有者 | G | O |
    *
    *  例:
    *    rwxr-xr-x
    *
    *  ビット構造:
    *    0400 = 所有者読み
    *    0200 = 所有者書き
    *    0100 = 所有者実行
    *    0040 = グループ読み
    *    0020 = グループ書き
    *    0010 = グループ実行
    *    0004 = その他読み
    *    0002 = その他書き
    *    0001 = その他実行
    */

   mode = buf.st_mode & ~0066;
   /*
    * 0066 (8進数):
    *
    *   0066 = 000 110 110
    *
    *   グループ書き (0020)
    *   グループ実行 (0010)
    *   その他書き (0002)
    *   その他実行 (0001)
    *
    * ~0066:
    *   これらのビットを 0 にするマスク。
    *
    * & ~0066:
    *   指定ビットを削除する操作。
    *
    * つまり:
    *   グループとその他の書き込み・実行権限を削除する。
    *
    * 例:
    *   元: rwxrwxrwx
    *   結果: rwxr--r--
    */

   ret = chmod(argv[1], mode);
   /*
    * chmod(path, mode):
    *
    * 指定ファイルのパーミッションを変更する。
    *
    * 内部では inode の st_mode フィールドを書き換える。
    *
    * 成功: 0
    * 失敗: -1
    */

   if(ret < 0){
      perror("chmod");
      return 1;
   }

   return 0;
   /*
    * 正常終了。
    */
}
