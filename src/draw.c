#include <pebble.h>
#include "draw.h"

// Function to draw the menu layer primitives:
void drawMenu (Layer *layer, GContext *ctx, GColor color1, GColor color2, GColor color3) {
		GRect bounds = layer_get_bounds(layer);  
	
		for(int x = 1; x < 4; x++) {	
			
			switch(x)
				{
					case 1:
						graphics_context_set_stroke_color(ctx, color1);
						break;
					case 2:
						graphics_context_set_stroke_color(ctx, color2);
						break;
					case 3:
						graphics_context_set_stroke_color(ctx, color3);
						break;
				}
			
			graphics_context_set_stroke_width(ctx, 5);
			
			//Drawing the 3 button blocks onto the menu layer:
			graphics_draw_round_rect(ctx, GRect(bounds.size.w -2, ((x-1)*(bounds.size.h/3)+11), 4, (bounds.size.h/3)-25), 25);
			
			//Drawing the 3 rectangular blocks onto the menu layer:
			graphics_draw_round_rect(ctx, GRect(5,(5+((x-1)*bounds.size.h/3)), bounds.size.w - 15, (bounds.size.h/3)-10), 10);
			
		}
			
		//Initialising rectangular Frames for the menu text and drawing the text onto the menu layer:
		graphics_context_set_text_color(ctx, GColorBlack);
		GRect frames[3];
		frames[0] = GRect(5, 7, bounds.size.w - 42, (bounds.size.h/3)-13);
		const char* mLabel[] = {"Run", "Stats", "Settings"};
	
		for(int x = 1; x<3; x++) {
			frames[x] = GRect(5, (x*bounds.size.h/3)+7, bounds.size.w - 10, (bounds.size.h/3)-13);
		}
	
		for(int y = 0; y<3; y++ ){
			graphics_draw_text(ctx, mLabel[y], s_font_30, frames[y], GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		}
		
}

void drawRun(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	GRect timerRect = GRect(0, 0, bounds.size.w, 138);
	
	graphics_context_set_fill_color(ctx, GColorMayGreen);
	graphics_fill_rect(ctx, timerRect, 0, GCornerNone);
}