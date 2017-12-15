/*
  ******************************************************************************
  * @file   ./debug.h
  * @author xiaohui wang
  * @E-mail wangxh77@163.com
  * @version V1.000
  * @date    11-10-2017
  * @brief   SDL
  ********************************************************************************
 */


  
#ifndef __SDL_DEBUG_HH_
#define __SDL_DEBUG_HH_



#ifndef NULL
#define NULL 0
#endif


#define DEBUG_ON

#ifdef DEBUG_ON
#define SDL_DEBUG(msq...) 					do{     \
			                                    printf("sdl_msg >>< %s : %s : %d :", __FUNCTION__, __FILE__, __LINE__); \
			                                    printf(msq);\
			                                    printf("\n"); \
			                                 }while(0)
                                         
#else
#define	SDL_DEBUG(str, arts...)				do {} while(0)
#endif


#endif
