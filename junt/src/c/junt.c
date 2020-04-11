#include <pebble.h>
#include "junt.h"

#define WINDOW_SIZE 5

static Window *s_window;
static TextLayer *s_text_layer;

// Hardcoded Variables for the algorithm
static const float PEAK_POINT_INFLECTION_RATIO_THRESHOLD = 1.5;
static const float PEAK_POINT_DEFLECTION_RATIO_THRESHOLD = 0.9;
static  AccelReading *(accelReadingsWindow[WINDOW_SIZE]);
//static AccelReading *(accelReadingsWindow[]); // TODO Initialize this 
static int jumpCount = 0;


static void initAccelReadingsWindow(){
     // Initialize accelReadingsWindow
     for(int i=0; i<WINDOW_SIZE; i++){
        // Check if any content exists, free it up if yes, and then realloc
        if (!accelReadingsWindow[i]){
            free(accelReadingsWindow[i]);
            }

        AccelReading *accelReading_ = newAccelReading(0, 0, 0, 0);
        accelReadingsWindow[i] = accelReading_;
     }
 }



static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Select");
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Up");
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Down");
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, 72, bounds.size.w, 20));
  text_layer_set_text(s_text_layer, "Press a button");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);

  // Initialize AccelReadingWindow
  // Initialize the Window
  //AccelReading *(accelReadingsWindow[WINDOW_SIZE]);
  initAccelReadingsWindow();

}

static void prv_deinit(void) {
  window_destroy(s_window);

  // TODO 
  // Release all objects with malloc
  for (int i=0; i<WINDOW_SIZE; i++){
     //free(accelReadingsWindow[i]); 
  }
  //free(accelReadingsWindow);
}


// peak_detected
//
bool peakDetected(){
    bool inflectionDetected = false;
    bool deflectionDetected = false;

    // Check if the first element contains
    AccelReading* firstAccelReading = accelReadingsWindow[0];
    int16_t firstAccelReadingX      = firstAccelReading->x;

    if ((firstAccelReading->isEmptyReading) || (firstAccelReadingX == 0))
        return false;

    // Iterate over the accelReadingsWindow to check if there was a peak or not in the given window
    // NOTE accelReadingsWindow is already initiatlized here
    for(int i=0; i<WINDOW_SIZE; i++){
        int16_t x   = (accelReadingsWindow[i]->x != 0) ? accelReadingsWindow[i]->x : 1;
        float ratio = (float) (x / firstAccelReadingX);  // Float Division
        APP_LOG(APP_LOG_LEVEL_INFO, "Ratio %d", (int) (ratio * 100.0)  ) ;

        if (ratio >= PEAK_POINT_INFLECTION_RATIO_THRESHOLD)
            inflectionDetected = true;

        if (inflectionDetected && ( ratio <= PEAK_POINT_DEFLECTION_RATIO_THRESHOLD))
            deflectionDetected = true;
        
        firstAccelReadingX = x;
        }

    return (inflectionDetected && deflectionDetected);
    }

// Window to append new AccelReading into the window
void pushInWindow(AccelReading* accelReading){
    accelReadingsWindow[WINDOW_SIZE - 1] = accelReading; 
    for (int i=0; i < (WINDOW_SIZE-1); i++){
        accelReadingsWindow[i] = accelReadingsWindow[i+1];
    }
  }


static void accel_data_handler(AccelData *data, uint32_t num_samples) {
  // Read sample 0's x, y, and z values
  int16_t x = data[0].x;
  int16_t y = data[0].y;
  int16_t z = data[0].z;

  // Determine if the sample occured during vibration, and when it occured
  bool did_vibrate = data[0].did_vibrate;
  uint64_t timestamp = data[0].timestamp;

  //readings->x_array[0] = x;

  if(!did_vibrate) {
    // Print it out
    //APP_LOG(APP_LOG_LEVEL_INFO, "t: %llu, x: %d, y: %d, z: %d", timestamp, x, y, z);

    // Create a new Accel Reading
    AccelReading* accelReading = newAccelReading(timestamp, x, y, z);
    // Populate accelReadingsWindow
    pushInWindow(accelReading);
    if (peakDetected()){
        jumpCount += 1;
        initAccelReadingsWindow();
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Jump Count: %d", jumpCount);
       }

  } else {
    // Discard with a warning
    APP_LOG(APP_LOG_LEVEL_WARNING, "Vibration occured during collection");
  }
}

int main(void) {

  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  uint32_t num_samples = 3;  // Number of samples per batch/callback
  // Subscribe to batched data events

  //*reading;
  accel_data_service_subscribe(num_samples, accel_data_handler);

  app_event_loop();
  prv_deinit();
}
