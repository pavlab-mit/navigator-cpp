/*************************************************************
 * SBUS Parser Example for Raspberry Pi
 * 
 * Demonstrates how to use the SbusHandler class to read and decode
 * RC receiver data
 * 
 * Compile with: 
 *   g++ -o sbus_example main.cpp sbus_handler.cpp -lrt -pthread
 * 
 * Run with:
 *   sudo ./sbus_example
 ************************************************************/

 #include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <signal.h>
 #include <time.h>
 #include <math.h>
 #include "sbus_handler.h"
 
 // Flag for program exit
 static volatile int keep_running = 1;
 
 // Signal handler for clean exit
 void signal_handler(int sig) {
     keep_running = 0;
 }
 
 // Helper function to get time in microseconds
 uint64_t get_micros() {
     struct timespec ts;
     clock_gettime(CLOCK_MONOTONIC, &ts);
     return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
 }
 
 int main(int argc, char *argv[]) {
     // Set up signal handling
     signal(SIGINT, signal_handler);
     signal(SIGTERM, signal_handler);
     
     printf("\n=================================================\n");
     printf("SBUS Parser Example for Raspberry Pi\n");
     printf("=================================================\n");
     
     // Initialize SBUS handler
     SbusHandler sbus;
     if (!sbus.initialize()) {
         fprintf(stderr, "Failed to initialize SBUS handler\n");
         return 1;
     }
     
     printf("SBUS handler initialized. Waiting for frames...\n\n");
     
     // For display timing
     uint64_t last_display_time = 0;
     uint64_t start_time = get_micros();
     
     // For tracking channel changes
     uint16_t prev_channels[SBUS_NUM_CHANNELS] = {0};
     uint64_t change_time[SBUS_NUM_CHANNELS] = {0};
     
     // Main loop
     while (keep_running) {
         // Process available SBUS data
         bool new_frame = sbus.update();
         
         // Check if it's time to update the display
         uint64_t now = get_micros();
         if (now - last_display_time > 100000) {  // 100ms (10Hz) update rate
             last_display_time = now;
             
             // Display runtime and frame stats
             float runtime = (now - start_time) / 1000000.0f;
             printf("\033[2J\033[H");  // Clear screen and move cursor to top
             printf("SBUS Parser for RadioLink AT9S Pro\n");
             printf("Runtime: %.1f s  |  Frames: %lu  |  Errors: %lu  |  Rate: %.1f Hz\n\n", 
                     runtime, sbus.getFramesReceived(), sbus.getFrameErrors(),
                     sbus.getFrameRate());
             
             // Display channels in a readable format with highlighted changes
             printf("RC Channels:\n");
             for (int i = 0; i < SBUS_NUM_CHANNELS; i += 4) {
                 printf("Ch%2d-%-2d: ", i+1, i+4);
                 for (int j = 0; j < 4 && (i+j) < SBUS_NUM_CHANNELS; j++) {
                     int ch_idx = i+j;
                     uint16_t channel_value = sbus.getChannel(ch_idx);
                     
                     // Check for changes
                     if (abs((int)channel_value - (int)prev_channels[ch_idx]) > 10) {
                         change_time[ch_idx] = now;
                         prev_channels[ch_idx] = channel_value;
                     }
                     
                     // Highlight recently changed values
                     if (now - change_time[ch_idx] < 1000000) { // Highlight for 1 second
                         printf("\033[1;32m%4d\033[0m ", channel_value); // Green and bold
                     } else {
                         printf("%4d ", channel_value);
                     }
                 }
                 printf("\n");
             }
             
             // Display scaled values for first 8 channels (-100 to 100%)
             printf("\nScaled Values (Sticks and Switches):\n");
             for (int i = 0; i < 8; i++) {
                 int percent = sbus.getChannelScaled(i, -100, 100);
                 
                 // Highlight recently changed values
                 if (now - change_time[i] < 1000000) { // Highlight for 1 second
                     printf("\033[1;32mCh%d: %3d%%\033[0m  ", i+1, percent);
                 } else {
                     printf("Ch%d: %3d%%  ", i+1, percent);
                 }
                 
                 // Print a simple bar graph
                 printf("[");
                 int bar_width = 20;
                 int bar_pos = (percent + 100) * bar_width / 200;
                 
                 for (int j = 0; j < bar_width; j++) {
                     if (j == bar_width/2) printf("|");
                     else if (j < bar_pos) printf("#");
                     else printf(" ");
                 }
                 printf("]\n");
             }
             
             // Display a dedicated section for switches (typically channels 5-8 for RadioLink)
             printf("\nSwitch Positions:\n");
             for (int i = 4; i < 8; i++) {
                 int value = sbus.getChannel(i);
                 const char *position;
                 
                 // Determine switch position (customize thresholds based on your transmitter)
                 if (value < 500) position = "LOW";
                 else if (value > 1500) position = "HIGH";
                 else position = "MID";
                 
                 // Highlight recently changed values
                 if (now - change_time[i] < 1000000) { // Highlight for 1 second
                     printf("\033[1;32mSwitch %d: %s (%d)\033[0m\n", i+1, position, value);
                 } else {
                     printf("Switch %d: %s (%d)\n", i+1, position, value);
                 }
             }
             
             // Display flags
             printf("\nFlags: ");
             printf("CH17=%s  ", sbus.getChannel17() ? "ON" : "off");
             printf("CH18=%s  ", sbus.getChannel18() ? "ON" : "off");
             printf("FrameLost=%s  ", sbus.isFrameLost() ? "YES" : "no");
             printf("Failsafe=%s\n", sbus.isFailsafe() ? "ACTIVE" : "inactive");
             
             // NEW: Display controller connection status and metrics
             printf("\nController Status: ");
             if (sbus.isControllerConnected()) {
                 printf("\033[1;32mCONNECTED\033[0m  ");
             } else {
                 printf("\033[1;31mDISCONNECTED\033[0m  ");
             }
             
             // Show time since last frame
             uint64_t time_since_frame = sbus.getTimeSinceLastFrame();
             printf("Last Frame: %.2f ms ago  ", time_since_frame / 1000.0f);
             
             // Show consecutive frame losses
             unsigned long consec_losses = sbus.getConsecutiveFrameLosses();
             if (consec_losses > 0) {
                 printf("Consecutive Losses: \033[1;31m%lu\033[0m\n", consec_losses);
             } else {
                 printf("Consecutive Losses: 0\n");
             }
         }
         
         // Small delay to reduce CPU usage when not running in a dedicated thread
         usleep(1000);  // 1ms
     }
     
     printf("\nShutting down...\n");
     
     // Clean up is handled by the SbusHandler destructor
     
     return 0;
 }