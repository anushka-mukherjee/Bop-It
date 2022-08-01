#include "utils.h"
#include "3140_concur.h"
#include "realtime.h"
#include "game.h"
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include "lcd.h"

int main(void) {
	setup();
	start_countdown();
	int score = play_game(); //score is how many levels you've passed
	//following functions are from Aaron
	init_lcd();
	displayDigit(1, score);
	while (1){
		//loop forever
	}
	return 0;

}
