/*
 * Gnocatan: a fun game.
 * (C) 2000 the Free Software Foundation
 *
 * Author: Steve Langasek
 *
 * Declarations for Mersenne Twister functions
 * http://www.math.keio.ac.jp/~matumoto/ver980409.html
 */
#ifndef __mt_rand_h
#define __mt_rand_h

void mt_seed(gulong seed);
gulong mt_reload(void);
gulong mt_random(void);

#endif				/* __mt_rand_h */
