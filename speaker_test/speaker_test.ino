#include <driver/i2s.h>
#include <math.h>

#define I2S_NUM         I2S_NUM_0
#define I2S_BCK_PIN     14 // BCLK
#define I2S_WS_PIN      12 // LRC
#define I2S_DATA_PIN    32 // DIN

// Frequencies for "Happy Birthday" in Hz
#define NOTE_C4  261
#define NOTE_D4  294
#define NOTE_E4  329
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  493
#define NOTE_C5  523


// Melody for "Happy Birthday"
const int melody[] = {
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_F4, NOTE_E4, // Line 1
  NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_G4, NOTE_F4, // Line 2
  NOTE_C4, NOTE_C4, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_E4, NOTE_D4, // Line 3
  NOTE_C5, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_G4, NOTE_F4   // Line 4
};

// Note durations
const int noteDurations[] = {
  4, 4, 4, 4, 4, 2, // Line 1
  4, 4, 4, 4, 4, 2, // Line 2
  4, 4, 4, 4, 4, 4, 2, // Line 3
  4, 4, 4, 4, 4, 1  // Line 4
};

void setup() {
  Serial.begin(115200);
  
  // Configure I2S
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 22050,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .dma_buf_count = 25,
    .dma_buf_len = 35
  };

  // Install I2S driver
  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  
  // Set I2S pins
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_DATA_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_set_pin(I2S_NUM, &pin_config);
}

void loop() {
  playHappyBirthday();
  delay(2000); // Delay before playing again
}

void playHappyBirthday() {
  for (int thisNote = 0; thisNote < 25; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote]; // Calculate note duration
    int samples = noteDuration * (22050 / 1000); // Calculate number of samples
    int16_t *buffer = (int16_t *)malloc(samples * sizeof(int16_t));

    // Generate the note's sine wave
    for (int i = 0; i < samples; i++) {
      buffer[i] = (int16_t)(32767 * sin(2 * PI * melody[thisNote] * i / 22050)*0.5);
    }

    // Write samples to I2S
    size_t bytes_written;
    i2s_write(I2S_NUM, buffer, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    
    free(buffer); // Free the allocated memory
    delay(noteDuration); // Delay for the duration of the note
  }

   // Mute by writing zeros to the I2S output for a duration
  int muteDuration = 2000; // Mute for 2 seconds
  int muteSamples = muteDuration * (22050 / 1000); // Calculate number of samples for mute duration
  int16_t *muteBuffer = (int16_t *)malloc(muteSamples * sizeof(int16_t));

  // Fill the mute buffer with zeros
  memset(muteBuffer, 0, muteSamples * sizeof(int16_t));

  // Write mute samples to I2S
  size_t bytes_written;
  i2s_write(I2S_NUM, muteBuffer, muteSamples * sizeof(int16_t), &bytes_written, portMAX_DELAY);

  free(muteBuffer); // Free the allocated mute buffer
}
