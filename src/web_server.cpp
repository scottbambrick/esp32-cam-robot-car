#include <Arduino.h>
#include <WiFi.h>
#include "esp_http_server.h"
#include "camera.h"
#include "car.h"
#include "web_server.h"
#include "index_html.h"
#include "icons.h"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static httpd_handle_t control_httpd = NULL;
static httpd_handle_t stream_httpd = NULL;

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
  return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

// Lets the page be added to a phone's home screen and open full-screen.
static const char MANIFEST_JSON[] =
    "{\"name\":\"Robot Car\",\"short_name\":\"Robot Car\",\"start_url\":\"/\",\"scope\":\"/\","
    "\"display\":\"fullscreen\",\"orientation\":\"landscape\","
    "\"background_color\":\"#05070a\",\"theme_color\":\"#05070a\",\"icons\":["
    "{\"src\":\"/icon-192.png\",\"sizes\":\"192x192\",\"type\":\"image/png\",\"purpose\":\"any maskable\"},"
    "{\"src\":\"/icon-512.png\",\"sizes\":\"512x512\",\"type\":\"image/png\",\"purpose\":\"any maskable\"}]}";

static esp_err_t manifest_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/manifest+json");
  return httpd_resp_send(req, MANIFEST_JSON, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t icon_handler(httpd_req_t *req) {
  bool big = strstr(req->uri, "512") != NULL;
  httpd_resp_set_type(req, "image/png");
  httpd_resp_set_hdr(req, "Cache-Control", "max-age=604800");
  return big ? httpd_resp_send(req, (const char *)ICON_512_PNG, sizeof(ICON_512_PNG))
             : httpd_resp_send(req, (const char *)ICON_192_PNG, sizeof(ICON_192_PNG));
}

static esp_err_t capture_handler(httpd_req_t *req) {
  camera_fb_t *fb = camera_grab();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  camera_release(fb);
  return res;
}

// Live tuning and diagnostics, e.g. /set?var=txpower&val=44 (units of 0.25
// dBm), /set?var=xclk&val=24 (MHz, restarts the camera and is saved),
// /set?var=camera&val=0.
// With no query it just reports the current values.
static esp_err_t set_handler(httpd_req_t *req) {
  char query[64], var[16], val[16];
  bool ok = true;
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK &&
      httpd_query_key_value(query, "var", var, sizeof(var)) == ESP_OK &&
      httpd_query_key_value(query, "val", val, sizeof(val)) == ESP_OK) {
    int v = atoi(val);
    if (!strcmp(var, "txpower")) {
      ok = WiFi.setTxPower((wifi_power_t)v);
    } else if (!strcmp(var, "xclk")) {
      camera_stop();
      ok = camera_start(v * 1000000);
    } else if (!strcmp(var, "camera")) {
      if (v) ok = camera_start(0);
      else camera_stop();
    } else {
      ok = false;
    }
  }
  char out[128];
  snprintf(out, sizeof(out), "{\"ok\":%s,\"txpower\":%d,\"xclk\":%d,\"camera\":%d,\"channel\":%d,\"rssi\":%d,\"heap\":%u}",
           ok ? "true" : "false", (int)WiFi.getTxPower(), camera_xclk() / 1000000,
           camera_running() ? 1 : 0, (int)WiFi.channel(), (int)WiFi.RSSI(), (unsigned)ESP.getFreeHeap());
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, out, HTTPD_RESP_USE_STRLEN);
}

// Runs on its own server (port 81) because it never returns while a client
// is watching, and would otherwise block the control WebSocket.
static esp_err_t stream_handler(httpd_req_t *req) {
  esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  char part[64];
  while (true) {
    camera_fb_t *fb = camera_grab();
    if (!fb) return ESP_FAIL;   // camera stopped or capture failed; page retries
    size_t hlen = snprintf(part, sizeof(part), STREAM_PART, (unsigned)fb->len);
    res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, part, hlen);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    camera_release(fb);
    if (res != ESP_OK) return res;   // client went away
  }
}

static void ws_reply(httpd_req_t *req, const char *text) {
  httpd_ws_frame_t out = {};
  out.type = HTTPD_WS_TYPE_TEXT;
  out.payload = (uint8_t *)text;
  out.len = strlen(text);
  httpd_ws_send_frame(req, &out);
}

// Only one page drives at a time. Whoever last moved the car owns it until
// they've sent no movement for DRIVER_IDLE_MS; anyone else's drive commands
// are refused with "K" meanwhile (and their stops ignored, so an idle tab
// can't interfere).
static const uint32_t DRIVER_IDLE_MS = 1500;
static int driver_fd = -1;
static uint32_t driver_active = 0;

static void handle_drive(httpd_req_t *req, int left, int right) {
  int fd = httpd_req_to_sockfd(req);
  uint32_t now = millis();
  bool moving = left != 0 || right != 0;
  if (fd != driver_fd) {
    if (!moving) return;
    if (driver_fd >= 0 && now - driver_active < DRIVER_IDLE_MS) {
      ws_reply(req, "K");
      return;
    }
    driver_fd = fd;
  }
  if (moving) driver_active = now;
  car_drive(left, right);
}

// Text messages from the page, one command per frame:
//   D <left> <right>   drive, percent -100..100
//   A <n>              acceleration limit, percent per second (0 = off)
//   B <0|1>            brake when stopping
//   L <0-255>          headlight brightness
//   F <n> / Q <n>      camera frame size / JPEG quality
//   R <0|1>            rotate video 180 degrees
//   P <id>             ping; answered with "P <id> <rssi>"
static esp_err_t ws_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) return ESP_OK;   // handshake

  httpd_ws_frame_t frame = {};
  esp_err_t res = httpd_ws_recv_frame(req, &frame, 0);   // length only
  if (res != ESP_OK) return res;
  char buf[64];
  if (frame.len >= sizeof(buf)) return ESP_FAIL;
  if (frame.len == 0) return ESP_OK;
  frame.payload = (uint8_t *)buf;
  res = httpd_ws_recv_frame(req, &frame, sizeof(buf) - 1);
  if (res != ESP_OK) return res;
  if (frame.type != HTTPD_WS_TYPE_TEXT) return ESP_OK;
  buf[frame.len] = '\0';

  int a = 0, b = 0;
  switch (buf[0]) {
    case 'D':
      if (sscanf(buf + 1, "%d %d", &a, &b) == 2) handle_drive(req, a, b);
      break;
    case 'A':
      if (sscanf(buf + 1, "%d", &a) == 1) car_set_accel(a);
      break;
    case 'B':
      if (sscanf(buf + 1, "%d", &a) == 1) car_set_brake(a != 0);
      break;
    case 'L':
      if (sscanf(buf + 1, "%d", &a) == 1) car_light(constrain(a, 0, 255));
      break;
    case 'F':
    case 'Q':
    case 'R':
      if (sscanf(buf + 1, "%d", &a) == 1) camera_set(buf[0], a);
      break;
    case 'P':
      if (sscanf(buf + 1, "%d", &a) == 1) {
        char reply[32];
        int rssi = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
        snprintf(reply, sizeof(reply), "P %d %d", a, rssi);
        ws_reply(req, reply);
      }
      break;
  }
  return ESP_OK;
}

static void register_uri(httpd_handle_t server, const char *uri, esp_err_t (*handler)(httpd_req_t *), bool websocket = false) {
  httpd_uri_t u = {};
  u.uri = uri;
  u.method = HTTP_GET;
  u.handler = handler;
  u.is_websocket = websocket;
  httpd_register_uri_handler(server, &u);
}

void web_server_start() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.lru_purge_enable = true;
  // The camera driver's task is pinned to core 0 at high priority (fixed in
  // the precompiled core), so keep the HTTP work on the otherwise idle core 1.
  config.core_id = 1;

  if (httpd_start(&control_httpd, &config) == ESP_OK) {
    register_uri(control_httpd, "/", index_handler);
    register_uri(control_httpd, "/capture", capture_handler);
    register_uri(control_httpd, "/set", set_handler);
    register_uri(control_httpd, "/manifest.json", manifest_handler);
    register_uri(control_httpd, "/icon-192.png", icon_handler);
    register_uri(control_httpd, "/icon-512.png", icon_handler);
    register_uri(control_httpd, "/ws", ws_handler, true);
    Serial.printf("Control server on port %d\n", config.server_port);
  }

  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    register_uri(stream_httpd, "/stream", stream_handler);
    Serial.printf("Stream server on port %d\n", config.server_port);
  }
}
