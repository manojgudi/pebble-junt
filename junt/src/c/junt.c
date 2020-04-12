#include <pebble.h>
#include "junt.h"

#define WINDOW_SIZE 4
#define MAX_TEXT_SIZE 40

static Window *s_window;
static TextLayer *s_text_layer;

// Hardcoded Variables for the algorithm
static const float PEAK_POINT_INFLECTION_RATIO_THRESHOLD = 1.6*1.6;
static const float PEAK_POINT_DEFLECTION_RATIO_THRESHOLD = 0.8*0.8;
static AccelReading *(accelReadingsWindow[WINDOW_SIZE]);
static JumpStatistics *jumpStatistics;
//static AccelReading *(accelReadingsWindow[]); // TODO Initialize this 
static int jumpCount = 0;
static char *staticText;
static bool isAccelDataSubscribed = false;


// Window to append new AccelReading into the window
void pushInWindow(AccelReading* accelReading){
    accelReadingsWindow[WINDOW_SIZE - 1] = accelReading; 
    for (int i=0; i < (WINDOW_SIZE-1); i++){
        accelReadingsWindow[i] = accelReadingsWindow[i+1];
    }
  }

// Update the data structure
void showStatsOnWindow(){
    //APP_LOG(APP_LOG_LEVEL_INFO, "showStats1 %d %d ", heap_bytes_used(), heap_bytes_free());
    snprintf(staticText, MAX_TEXT_SIZE, "Jump Count: %d\n Rate: %d", jumpStatistics->jumpCount, jumpStatistics->averageJumpsPerMin);
    //sprintf(buffer, "Jump Count: %d", jumpCount);
    text_layer_set_text(s_text_layer, staticText);
    APP_LOG(APP_LOG_LEVEL_INFO, staticText) ;
    //APP_LOG(APP_LOG_LEVEL_INFO, "showStats2 %d %d ", heap_bytes_used(), heap_bytes_free());
    }

// Peak_Detected hence Jump Detected
bool peakDetected(){
    //APP_LOG(APP_LOG_LEVEL_INFO, "PeakD1 %d %d ", heap_bytes_used(), heap_bytes_free());
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
        float ratio = (float) (x*1.0 / firstAccelReadingX) * (x*1.0 / firstAccelReadingX);  // Float Division
        //APP_LOG(APP_LOG_LEVEL_INFO, "Ratio %d, %d, %d", firstAccelReadingX, x, (int) (ratio * 1000) ) ;

        if (ratio >= PEAK_POINT_INFLECTION_RATIO_THRESHOLD)
            inflectionDetected = true;

        if (inflectionDetected && ( ratio <= PEAK_POINT_DEFLECTION_RATIO_THRESHOLD))
            deflectionDetected = true;
        
        firstAccelReadingX = x;
      }
    
    APP_LOG(APP_LOG_LEVEL_INFO, "PeakD2 %d %d ", heap_bytes_used(), heap_bytes_free());
    //APP_LOG(APP_LOG_LEVEL_INFO, "Memory Free %d", heap_bytes_free());
    return (inflectionDetected && deflectionDetected);
    }


static void initAccelReadingsWindow(){
    //APP_LOG(APP_LOG_LEVEL_INFO, "initAcc1 %d %d ", heap_bytes_used(), heap_bytes_free());
     // Initialize accelReadingsWindow
     for(int i=0; i<WINDOW_SIZE; i++){
        // Check if any content exists, free it up if yes, and then realloc
        //if (accelReadingsWindow[i]){
            free(accelReadingsWindow[i]);
         //}

        accelReadingsWindow[i] = newAccelReading(0,0,0,0);
        //AccelReading *accelReading_ = newAccelReading(0, 0, 0, 0);
        //accelReadingsWindow[i] = accelReading_;
     }
    //APP_LOG(APP_LOG_LEVEL_INFO, "initAcc2 %d %d ", heap_bytes_used(), heap_bytes_free());
 }

// Acceleration Handler
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
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "Jump Count: %d", jumpCount);
        //text_layer_set_text(s_text_layer, ("JC %d", jumpCount));
     
      // Update the jumpStatistics
      if (jumpStatistics->jumpCount != 0){
          int estimatedJumpsPerMin = (int) (60000.0/ (timestamp - jumpStatistics->previousEpochTimeMS));
          int averageJumpsPerMin = (int) ((jumpStatistics->averageJumpsPerMin * jumpStatistics->jumpCount) + estimatedJumpsPerMin) / (jumpStatistics->jumpCount + 1);
          jumpStatistics->averageJumpsPerMin = averageJumpsPerMin;

       }

      // Update the statistics
      jumpStatistics->jumpCount = jumpCount;
      jumpStatistics->previousEpochTimeMS = timestamp;

      // Finally Show Stats on the Window
      showStatsOnWindow();
    }

    //free(accelReading);
  }   else {
    // Discard with a warning
    APP_LOG(APP_LOG_LEVEL_WARNING, "Vibration occured during collection");
  }
}

/*
Reset the application state
*/
static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Tell User its active
  vibes_short_pulse();
  jumpCount = 0;

  // Subscribe if not subscribed
  if (!isAccelDataSubscribed){
    uint32_t num_samples = 3;  // Number of samples per batch/callback
    accel_data_service_subscribe(num_samples, accel_data_handler);
    isAccelDataSubscribed = true;
    text_layer_set_text(s_text_layer, "Session Started");
  } else {
    // Unsubscribe the service  
    text_layer_set_text(s_text_layer, "Session Paused\nPress Start to resume.");
    accel_data_service_unsubscribe();
    isAccelDataSubscribed = false;
  }
  
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  //text_layer_set_text(s_text_layer, "Up");
  showStatsOnWindow();
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  //text_layer_set_text(s_text_layer, "Down");
  showStatsOnWindow();
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, 72, bounds.size.w, 40));
  text_layer_set_text(s_text_layer, "Press Select to Start");
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

  // Enable Text Wrapping
  text_layer_set_overflow_mode(s_text_layer, GTextOverflowModeWordWrap);

  // Initialize the Static Text to show Jump Count
  staticText = malloc(sizeof(char) * (MAX_TEXT_SIZE + 1));

  // Initialize jumpStatistics;
  jumpStatistics = newJumpStatistics();

  // Initialize the Window
  initAccelReadingsWindow();

}

static void prv_deinit(void) {
  window_destroy(s_window);
  // Unsubscribe
  accel_data_service_unsubscribe();  

  // XXX Why does the app crash when we free this?
  // Release all objects with malloc
  for (int i=0; i<WINDOW_SIZE; i++){
     //free(accelReadingsWindow[i]); 
  }
  
  // Free staticText
  free(staticText);
  //free(accelReadingsWindow);
  free(jumpStatistics);
}



int main(void) {

  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  // Subscribe to batched data events

  //*reading;

  app_event_loop();
  prv_deinit();
}
