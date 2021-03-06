/*-------------------------------------------------------*/
/* popup.h       ( YZU_CSE WindTop BBS )                 */
/*-------------------------------------------------------*/
/* author : verit.bbs@bbs.yzu.edu.tw                     */
/* target : popup menu                                   */
/* create : 2003/02/12                                   */
/*-------------------------------------------------------*/


#ifndef POPUP_H
#define POPUP_H

#define POPUP_QUIT              0x00
#define POPUP_FUN               0x01
#define POPUP_XO                0x02
#define POPUP_MENU              0x04
#define POPUP_MENUTITLE         0x08
#if NO_SO
  #define POPUP_SO              POPUP_FUN
#else
  #define POPUP_SO              0x10  /* For dynamic library loading */
#endif

#define POPUP_DO_INSTANT        0x01000000

#define POPUP_ARG               0x40000000  /* `item` is a function and a `void *` argument */

#define POPUP_MASK              0x000000FF

#endif  /* #ifndef POPUP_H */
