#ifndef CHIPEE_PERIPHERALS_H_
#define CHIPEE_PERIPHERALS_H_

void init_display();
void draw(unsigned char* display);
void sdl_handler(unsigned char* keypad);//copying from source, this will need ot be hotwired to work with bluetooth if i ever get that working. can prob sub for buttons.
int shoud_quit();
void stop_display();
#endif

