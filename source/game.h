/*************************************************************************
 * Contains the bulk of game functions
 *
 **************************************************************************
 */
#ifndef __GAME_H__
#define __GAME_H__

#include <stdlib.h>
#include <MKL46Z4.h>

/*
 * Sets up the accelerometer
 */
void accel_Initialize(void);
/*
 * Sets up the touch bar
 */
void ts_Initialize(void);
/*
 * Sets up SW1
 */
void SW1_Initialize(void);
/*
 * Sets up SW3
 */
void SW3_Initialize(void);
/*
 * Sets up the game -- as in, initializes LEDs, buttons, current time, score, and the 5 tasks
 */
void setup(void);
/*
 * Flashes both Red and Green LEDs simultaneously three times to indicate game starting countdown
 * Approx 1 second/flash
 */
void start_countdown(void);
/*
 * Chooses a random sequence of 5 tasks for user to do
 * As of MS1, will be a random binary sequence of 5 digits (b/c we only have left and right buttons)
 */
int choose_tasks(void);
/*
 * Plays the game -- that is, after the countdown, goes thru the tasks
 * Interprets them (0 -- must_hit_SW1 and 1 -- must_hit_SW3)
 * Executes the proper functions (must_hit_SW1 or must_hit_SW3)
 * Sees if user did the correct action
 * If yes -- continues the while loop
 * If no -- calls "end_game" and then "lose_game()"
 * If get to the end of the while loop, calls "game_over" and then "win_game()"
 *
 * Returns the score of the game
 */
int play_game(void);
/*
 * Signals the game is over -- 3 green/red, 1 flash/half second
 */
void game_over(void);
/*
 * Signals user has lost the game -- 1 indefinite red
 */
void lose_game(void);
/*
 * Signals user has won the game -- 1 indefinite green
 */
void win_game(void);
/*
 * Flashes the red LED once
 */
void must_hit_SW1(void);
/*
 * Flashes the green LED once
 */
void must_hit_SW3(void);
/*
 * Flashes red and green LED once at the same time
 */
void must_shake_board(void);
/*
 * Flashes red and green LED once at the same time then red
 * Triggers YES if you press HARD from l->r
 */
void must_touch_bar_lr(void);
#endif
