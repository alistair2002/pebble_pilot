#include <pebble.h>

static Window *window;
static TextLayer *heading_layer;
static TextLayer *wanted_layer;
static Layer *speed_layer;

static char speed_buffer[16] = {0};
static char value_buffer[16] = {0};
static char rudder_buffer[16] = {0};
static char cog_buffer[16] = {0};

typedef enum {
	ctrl_off,
	ctrl_compass,
	ctrl_rudder,
	ctrl_cog
} pid_ctrl_t;

static pid_ctrl_t ctrl_control = ctrl_cog;
static pid_ctrl_t ctrl_view = ctrl_compass;

static void send(int key, int value) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  dict_write_int(iter, key, &value, sizeof(int), true);

  app_message_outbox_send();
}

static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
  // Ready for next command
  //text_layer_set_text(text_layer, "Press up or down.");
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  text_layer_set_text(heading_layer, "Send failed!");
  APP_LOG(APP_LOG_LEVEL_ERROR, "Fail reason: %d", (int)reason);
}

static void received_handler(DictionaryIterator *iter, void *context) {
	Tuple *t = dict_read_first(iter);
	bool dirty = false;
							
	while(t != NULL) {

		if (t->key == 0) {
			snprintf(value_buffer, sizeof(value_buffer), "%d", (int)t->value->int32);
			dirty = true;
		} else if (t->key == 1) { /* what */
			static char wanted_buffer[16] = {0};
			snprintf(wanted_buffer, sizeof(wanted_buffer), "%d", (int)t->value->int32);
			text_layer_set_text(wanted_layer, wanted_buffer);
		
		} else if (t->key == 2) { /* who */
			switch (t->value->int32)
			{
				case 0:
					ctrl_control = ctrl_off;
					//text_layer_set_text(heading_layer, "Off");
					break;
				case 1:
					ctrl_control = ctrl_rudder;
					//text_layer_set_text(heading_layer, "Rudder");
					break;
				case 2:
					ctrl_control = ctrl_compass;
					//text_layer_set_text(heading_layer, "Compass");
					break;
				default:
					ctrl_control = ctrl_cog;
					//text_layer_set_text(heading_layer, "COG");
					break;
			}
		} else if (t->key == 3) { /* speed */
			snprintf(speed_buffer, sizeof(speed_buffer), "%d.%d", (int)t->value->int32/10, (int)t->value->int32%10);
			dirty = true;

		}else if (t->key == 4) { /* rudder */
			snprintf(rudder_buffer, sizeof(rudder_buffer), "%d", (int)t->value->int32);
			dirty = true;
			
		}else if (t->key == 5) { /* course over ground */
			snprintf(cog_buffer, sizeof(cog_buffer), "%d", (int)t->value->int32);
			dirty = true;
		}
		// Finally
		t = dict_read_next(iter);
	}

	if (dirty)
	{
		layer_mark_dirty(speed_layer);
	}
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
	//send(0, 3);
	if (ctrl_compass == ctrl_view) { ctrl_view = ctrl_rudder; }
	else if (ctrl_rudder == ctrl_view) { ctrl_view = ctrl_cog; }
	else /* if (ctrl_cog == ctrl_view)*/ { ctrl_view = ctrl_compass; } /* don't check as we should be one of the three */

	layer_mark_dirty(speed_layer);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  send(0, 1);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  send(0, 2);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void speed_update_proc(Layer *this_layer, GContext *ctx) {
  // Draw things here using ctx
  GRect bounds = layer_get_bounds(this_layer);

  GRect frame_speed = GRect(4, 12, 50, 50);
  GRect frame_value = GRect(30, 38, 110, 110);

  GPoint little_centre = GPoint(30, 30);
  GPoint compass_centre = GPoint(85, 10);
  GPoint rudder_centre = GPoint(112, 16);
  GPoint cog_centre = GPoint(133, 33);
  static char* p_value = value_buffer;

  graphics_fill_rect( ctx, bounds, 0, 0 );

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, little_centre, 30);

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, little_centre, 27);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(85, 70), 55);

  graphics_context_set_stroke_color(ctx, GColorWhite);
  
  switch (ctrl_view)
  {
	  case ctrl_compass:
		  graphics_fill_circle(ctx, compass_centre, 6);
		  p_value = value_buffer;
		  break;
	  case ctrl_rudder:
		  graphics_fill_circle(ctx, rudder_centre, 6);
		  p_value = rudder_buffer;
		  break;
	  case ctrl_cog:
		  graphics_fill_circle(ctx, cog_centre, 6);
		  p_value = cog_buffer;
		  break;
	  default:
		  break;
  }

  switch (ctrl_control)
  {
	  case ctrl_compass:
		  graphics_draw_circle(ctx, compass_centre, 9);
		  break;
	  case ctrl_rudder:
		  graphics_draw_circle(ctx, rudder_centre, 9);
		  break;
	  case ctrl_cog:
		  graphics_draw_circle(ctx, cog_centre, 9);
		  break;
	  default:
		  break;
  }
  
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx,
					 speed_buffer,
					 fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
					 frame_speed,
					 GTextOverflowModeTrailingEllipsis,
					 GTextAlignmentCenter,
					 NULL
	  );

  /* if the value is negative we will have to draw the line our selves
	 the fonts do not contain negative values */
  if ((p_value) && ('-' == *p_value)) {
	  graphics_context_set_fill_color(ctx, GColorBlack);
	  graphics_fill_rect( ctx, GRect(40, 70, 15, 5), 1, 0x0F);
  }
  
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx,
					 p_value,
					 fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49),
					 frame_value,
					 GTextOverflowModeTrailingEllipsis,
					 GTextAlignmentCenter,
					 NULL
	  );
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  heading_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { 100, 24 } });
  wanted_layer = text_layer_create((GRect) { .origin = { 100, 0 }, .size = { bounds.size.w-100, 24 } });
  speed_layer = layer_create((GRect) { .origin = { 0, 24 }, .size = { bounds.size.w, bounds.size.h-24 } });
	  
  // Update the font and text for the demo message
  text_layer_set_font(heading_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_font(wanted_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));

  //text_layer_set_text(heading_layer, "");
  text_layer_set_text_alignment(heading_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(heading_layer));
  text_layer_set_background_color( heading_layer, GColorBlack );
  text_layer_set_text_color( heading_layer, GColorWhite );

  text_layer_set_text(wanted_layer, "0");
  text_layer_set_text_alignment(wanted_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(wanted_layer));
  text_layer_set_background_color( wanted_layer, GColorBlack );
  text_layer_set_text_color( wanted_layer, GColorWhite );

  layer_add_child(window_layer, speed_layer);
  layer_set_update_proc(speed_layer, speed_update_proc);
}

static void window_unload(Window *window) {
  text_layer_destroy(heading_layer);
  text_layer_destroy(wanted_layer);
  layer_destroy(speed_layer);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = false;
  window_stack_push(window, animated);

  // Open AppMessage
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_register_inbox_received(received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
