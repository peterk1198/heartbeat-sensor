#ifndef DISPLAY_H
#define DISPLAY_H

void display_init();
void draw_graph(int x, int y, int w, int h);
void write_graph(int bpm);
void display_heart();
void pulsate_heart(int bpm);
void clear_heart();
void update_bpm(int bpm);
void clear_bpm();
int get_graph_height();
int get_graph_width();









#endif
