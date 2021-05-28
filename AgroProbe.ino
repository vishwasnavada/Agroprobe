
#include "arduino_secrets.h"
#include "thingProperties.h"
#include <SAMD21turboPWM.h>
#include <cstdarg>
#define __STATIC_FORCEINLINE                   __attribute__((always_inline)) static inline
#define __SSAT(ARG1, ARG2) \
  __extension__ \
  ({                          \
    int32_t __RES, __ARG1 = (ARG1); \
    __ASM volatile ("ssat %0, %1, %2" : "=r" (__RES) :  "I" (ARG2), "r" (__ARG1) : "cc" ); \
    __RES; \
  })
#include <agro_probe_inference.h>

static const float features[] = {9.8300, 0.0900, -0.8900, 9.8200, -0.0100, -0.8400, 9.8500, 0.1000, -0.9700, 9.8200, 0.0000, -0.9600, 9.8300, 0.0300, -0.9400, 9.8300, 0.1000, -0.9200, 9.8300, 0.0400, -0.9100, 9.8400, 0.0700, -0.9900, 9.8200, 0.0500, -0.9700, 9.8300, 0.0300, -0.8500, 9.8300, 0.1000, -0.8600, 9.8200, 0.0100, -0.9700, 9.8400, 0.0500, -0.9600, 9.8400, 0.0800, -0.9900, 9.8200, 0.0300, -0.8100, 9.8200, 0.0600, -0.8900, 9.8400, 0.0400, -1.0600, 9.8400, 0.0400, -0.9800, 9.8200, 0.0600, -0.8500, 9.8200, 0.0700, -0.8300, 9.8300, 0.0500, -0.9800, 9.8400, 0.0500, -1.0000, 9.8300, 0.0500, -0.9100, 9.8300, 0.0500, -0.8700, 9.8200, 0.1500, -0.8600, 9.8200, -0.0500, -0.9800, 9.8500, 0.1200, -1.0400, 9.8200, 0.0400, -0.9000, 9.8100, 0.0000, -0.7500, 9.8400, 0.1300, -0.9400, 9.8500, -0.0400, -1.1100, 9.8400, 0.1000, -0.9100, 9.8200, 0.0400, -0.8200, 9.8300, 0.0400, -0.9200, 9.8500, 0.0700, -1.0200, 9.8200, 0.0700, -0.8700, 9.8300, 0.0500, -0.8600, 9.8300, 0.0300, -0.9500, 9.8300, 0.0500, -0.9800, 9.8200, 0.0400, -0.8900, 9.8400, 0.0700, -0.9000, 9.8300, 0.0600, -0.9300, 9.8300, 0.0300, -0.9500, 9.8300, 0.0500, -0.9300, 9.8300, 0.0600, -0.8800, 9.8200, 0.0600, -0.8900, 9.8300, 0.0300, -0.9600, 9.8500, 0.1300, -0.9600, 9.8100, -0.0500, -0.8300, 9.8600, 0.1200, -1.0400, 9.8300, 0.0400, -0.9300
                                 // copy raw features here (for example from the 'Live classification' page)
                                 // see https://docs.edgeimpulse.com/docs/running-your-impulse-arduino
                                };

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
  memcpy(out_ptr, features + offset, length * sizeof(float));
  return 0;
}
TurboPWM pwm;
const int probe = 7;   //       change them for other boards
int Moistlevel1 = 0;
int result1, result2, result3, result4;
void setup() {

  pwm.setClockDivider(1, true);     // Input clock is divided by 1 and sent to Generic Clock, Turbo is On
  pwm.timer(2, 256, 40000, false);  // Timer 2 is set to Generic Clock divided by 256, resolution is 40000, phase-correct aka dual-slope PWM
  pwm.timer(1, 1, 250, true);       // Timer 1 is set to Generic Clock divided by 1, resolution is 250, normal aka fast aka single-slope PWM

  /* Initialize serial and wait up to 5 seconds for port to open */
  Serial.begin(115200);
  delay(1500);

  // Defined in thingProperties.h
  initProperties();


  /* Initialize Arduino IoT Cloud library */
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  setDebugMessageLevel(DBG_INFO);
  ArduinoCloud.printDebugInfo();
}


void loop() {
  ArduinoCloud.update();
  //Edge impulse detection part
  ei_printf("Edge Impulse standalone inferencing (Arduino)\n");

  if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
    ei_printf("The size of your 'features' array is not correct. Expected %lu items, but had %lu\n",
              EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(features) / sizeof(float));
    delay(1000);
    return;
  }

  ei_impulse_result_t result = { 0 };

  // the features are stored into flash, and we don't want to load everything into RAM
  signal_t features_signal;
  features_signal.total_length = sizeof(features) / sizeof(features[0]);
  features_signal.get_data = &raw_feature_get_data;

  // invoke the impulse
  EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, true /* debug */);
  ei_printf("run_classifier returned: %d\n", res);

  if (res != 0) return;

  // print the predictions
  ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);

  // print the predictions
  ei_printf("[");
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    ei_printf("%d", static_cast<int>(result.classification[ix].value * 100));
    result1 = result.classification[0].value * 100;
    result2 = result.classification[1].value * 100;
    result3 = result.classification[2].value * 100;
    result4 = result.classification[3].value * 100;
#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf(", ");
#else
    if (ix != EI_CLASSIFIER_LABEL_COUNT - 1) {
      ei_printf(", ");
    }
#endif
  }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
  ei_printf("%d", static_cast<int>(result.anomaly));
#endif
  ei_printf("]\n");
  // human-readable predictions
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
  }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
  ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif

  delay(1000);


}


void ei_printf(const char *format, ...) {
  static char print_buf[1024] = { 0 };

  va_list args;
  va_start(args, format);
  int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
  va_end(args);

  if (r > 0) {
    Serial.write(print_buf);
  }
}
int measure()
{
  pwm.timer(1, 1, 96, true);
  pwm.analogWrite(probe, 500);
  Serial.print("PWM frequency: "); Serial.print(pwm.frequency(1)); Serial.println("Hz");
  Serial.println("Duty cycle: 500/1000\n");
  Moistlevel1 = 0;
  delay(200);                                           // allow the circuit to stabilize
  for (int m = 1; m < 6 ; m++)                          // take 5 consecutive measurements in 5 seconds
  {
    Moistlevel1 = Moistlevel1 + analogRead(A0) ;    // Read data from analog pin 4 and add it to MoistLevel1 variable
    delay (1000);
  }
  Moistlevel1 = Moistlevel1 / 5;                       // Determine the average of 5 measurements
  Moistlevel1 = Moistlevel1 / 1725 * 100; // maximum value is 1725
}

void onMoistureChange() {
  measure();
  moisture = Moistlevel1;
}

void onElephantMovementChange() {
  elephant_movement   =  result3;
}

void onStandingChange() {
  standing =  result4;
}

void onEarthquakeChange() {
  earthquake =  result2;
}

void onDeforestationChange() {
  deforestation =  result1;
}
