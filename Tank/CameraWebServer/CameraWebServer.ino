#include <esp_camera.h>
#include <WiFi.h>
#include <ESP32Servo.h>


#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"


// Servo Configuration
#define PAN_PIN 14   // GPIO pin for the pan servo
#define TILT_PIN 2  // GPIO pin for the tilt servo


Servo panServo;  // Servo for X-axis (pan)
Servo tiltServo; // Servo for Y-axis (tilt)


#define RED_LED_PIN 13  // GPIO pin for the red LED (adjust as needed)


// Initial servo positions
const int INITIAL_PAN = 90;  // Center position for pan servo
const int INITIAL_TILT = 90; // Center position for tilt servo


// ===========================
// Enter your WiFi credentials
// ===========================
const char* ssid = "HUAWEI-1CFEJ9";
const char* password = "20242024";


// TCP Server Configuration
WiFiServer server(82);  // Create a TCP server on port 81


void startCameraServer();
void setupLedFlash(int pin);

// Function to reset servos to initial positions
void resetServos() {
  panServo.write(INITIAL_PAN);
  tiltServo.write(INITIAL_TILT);
  Serial.println("Servos reset to initial positions");
}


void controlRedLED(bool detected) {
  static unsigned long lastToggleTime = 0;
  static bool ledState = LOW;

  if (detected) {
    unsigned long currentTime = millis();
    if (currentTime - lastToggleTime >= 1000) {  // Toggle every 1 second
      ledState = !ledState;
      digitalWrite(RED_LED_PIN, ledState);
      lastToggleTime = currentTime;
    }
  } else {
    digitalWrite(RED_LED_PIN, LOW);  // Turn off the LED if no detection
  }
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

    // Initialize servos
  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);
  /*panServo.write(INITIAL_PAN); // Initial position
  tiltServo.write(INITIAL_TILT); // Initial position*/


  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();


     // Start the TCP server
  server.begin();
  Serial.println("TCP server started");

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");


}
void loop() {

  // Check for TCP client connections
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    while (client.connected()) {
      if (client.available()) {
        String command = client.readStringUntil('\n');
        command.trim();
        Serial.println("Received data: " + command); // Debug: Print raw data

        // Process servo angles (format: "pan_angle,tilt_angle")
        // Find the positions of the commas
        int commaIndex1 = command.indexOf(',');
        int commaIndex2 = command.indexOf(',', commaIndex1 + 1);
        if (commaIndex1 > 0 && commaIndex2 > 0) {
          int xPos = command.substring(0, commaIndex1).toInt();
          int yPos = command.substring(commaIndex1 + 1, commaIndex2).toInt();
          bool detected = command.substring(commaIndex2 + 1).toInt();
          xPos = constrain(xPos, 0, 180);    // Pan servo range
          yPos = constrain(yPos, 60, 100);   // Tilt servo range
          panServo.write(xPos);
          tiltServo.write(yPos);
          Serial.printf("Received: Pan=%d, Tilt=%d, Detected=%d\n", xPos, yPos, detected);
          client.println("OK"); // Send a response back to the client

         // Control the red LED based on detection status
          controlRedLED(detected);
        } else {
          Serial.println("Invalid data format");
          client.println("ERROR: Invalid data format");
          controlRedLED(false);  // No detection
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected");
  }

}