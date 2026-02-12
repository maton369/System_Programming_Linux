#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
/*
 * stdio.h:
 *   printf() を使用する。
 *
 * stdlib.h:
 *   exit() を使用する。
 *
 * sys/stat.h:
 *   stat() と struct stat を使用するために必要。
 *
 * sys/types.h:
 *   stat 構造体内の型定義（ino_t など）のために必要。
 *
 * unistd.h:
 *   POSIX 系関数群の宣言。
 *
 * time.h:
 *   ctime() を使用するために必要。
 *
 * stat() は、指定されたパスの inode 情報を取得する
 * システムコールである。
 */

int main(int argc, char *argv[]){

  struct stat buf;
  /*
   * struct stat:
   *   ファイルのメタデータ（inode情報）を格納する構造体。
   *
   * 主なフィールド:
   *   st_size   : ファイルサイズ（バイト）
   *   st_atime  : 最終アクセス時刻
   *   st_mtime  : 最終更新時刻
   *   st_ctime  : inode変更時刻
   *   st_ino    : inode番号
   *   st_mode   : ファイル種別とアクセス権
   */

  time_t time;
  /*
   * time 型変数。
   * 本コードでは未使用。
   * （ctime() の引数として time_t が必要）
   */

  int ret;
  /*
   * stat() の戻り値を格納。
   * 成功: 0
   * 失敗: -1
   */

  ret = stat(argv[1], &buf);
  /*
   * stat(path, &buf)
   *
   * 第1引数:
   *   情報を取得したいファイルのパス
   *
   * 第2引数:
   *   結果を書き込む構造体
   *
   * stat() はカーネルに問い合わせを行い、
   * 指定ファイルの inode 情報を取得する。
   */

  if(ret < 0){
    /*
     * stat が失敗した場合。
     * 例:
     *   - ファイルが存在しない
     *   - アクセス権がない
     */
    perror("stat");
    exit(1);
  }

  printf("Size: %d byte\n", buf.st_size);
  /*
   * st_size:
   *   ファイルサイズ（バイト単位）。
   *
   * これは inode に格納されている値。
   *
   * 注意:
   *   実際には off_t 型であり、
   *   %ld などで表示すべき場合もある。
   */

  printf("Access: %s", ctime(&buf.st_atime));
  /*
   * st_atime:
   *   最終アクセス時刻（UNIX time形式）。
   *
   * ctime():
   *   time_t 型を人間可読形式の文字列に変換する。
   *
   * 例:
   *   Wed Jun  5 12:34:56 2024
   */

  printf("inode: %ld\n", buf.st_ino);
  /*
   * st_ino:
   *   inode番号。
   *
   * inodeとは:
   *   ファイルの実体情報（サイズ、ブロック位置、権限など）
   *   を保持する構造体。
   *
   * ディレクトリエントリは
   *   「ファイル名 → inode番号」
   *   の対応を持つ。
   */

  return 0;
  /*
   * 正常終了。
   */
}
