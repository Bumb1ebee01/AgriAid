#include "web_server.h"

#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "sensor_manager.h"
#include "irrigation_controller.h"
#include "ml_inference.h"

static const char *TAG = "WEB";

/*
 * Shared state used by the dashboard.
 */
static sensor_data_t latest_sensor_data;

static controller_status_t latest_controller_status;

static ml_result_t latest_ml_result;

/*
 * -----------------------------------------
 * DASHBOARD HTML
 * -----------------------------------------
 */

static const char *DASHBOARD_HTML =

"<!DOCTYPE html>"
"<html>"
"<head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>AgriAid</title>"

"<style>"

"body{"
"font-family:Arial,sans-serif;"
"margin:0;"
"background:#f4f7f4;"
"color:#182018;"
"}"

".header{"
"background:#183d24;"
"color:white;"
"padding:20px;"
"}"

".header h1{"
"margin:0;"
"}"

".container{"
"padding:16px;"
"max-width:900px;"
"margin:auto;"
"}"

".grid{"
"display:grid;"
"grid-template-columns:repeat(auto-fit,minmax(180px,1fr));"
"gap:12px;"
"}"

".card{"
"background:white;"
"border-radius:14px;"
"padding:18px;"
"box-shadow:0 2px 8px rgba(0,0,0,.08);"
"}"

".label{"
"font-size:13px;"
"color:#667066;"
"}"

".value{"
"font-size:28px;"
"font-weight:bold;"
"margin-top:6px;"
"}"

".status{"
"margin-top:16px;"
"padding:16px;"
"border-radius:14px;"
"background:white;"
"}"

"#connection{"
"font-weight:bold;"
"}"

"</style>"
"</head>"

"<body>"

"<div class='header'>"
"<h1>🌱 AgriAid</h1>"
"<div>Smart Irrigation & Plant Intelligence</div>"
"</div>"

"<div class='container'>"

"<p id='connection'>Connecting...</p>"

"<div class='grid'>"

"<div class='card'>"
"<div class='label'>Soil Moisture</div>"
"<div class='value' id='soil'>--</div>"
"</div>"

"<div class='card'>"
"<div class='label'>Temperature</div>"
"<div class='value' id='temperature'>--</div>"
"</div>"

"<div class='card'>"
"<div class='label'>Humidity</div>"
"<div class='value' id='humidity'>--</div>"
"</div>"

"<div class='card'>"
"<div class='label'>Water Flow</div>"
"<div class='value' id='flow'>--</div>"
"</div>"

"<div class='card'>"
"<div class='label'>Light</div>"
"<div class='value' id='light'>--</div>"
"</div>"

"<div class='card'>"
"<div class='label'>Pump</div>"
"<div class='value' id='pump'>--</div>"
"</div>"

"</div>"

"<div class='status'>"
"<h2>🤖 TinyML</h2>"
"<p>Model: <b id='ml'>--</b></p>"
"<p>Plant health score: <b id='health'>--</b></p>"
"<p>Irrigation score: <b id='irrigation'>--</b></p>"
"</div>"

"<div class='status'>"
"<h2>🛡️ System</h2>"
"<p>Safety: <b id='safety'>--</b></p>"
"</div>"

"</div>"

"<script>"

"async function update(){"

"try{"

"const response=await fetch('/api/status');"

"const d=await response.json();"

"document.getElementById('connection').textContent='🟢 ESP32 connected';"

"document.getElementById('soil').textContent=d.soil.toFixed(1)+' %';"

"document.getElementById('temperature').textContent=d.temperature.toFixed(1)+' °C';"

"document.getElementById('humidity').textContent=d.humidity.toFixed(1)+' %';"

"document.getElementById('flow').textContent=d.flow.toFixed(2)+' L/min';"

"document.getElementById('light').textContent=d.day?'DAY':'NIGHT';"

"document.getElementById('pump').textContent=d.pump?'ON':'OFF';"

"document.getElementById('ml').textContent=d.ml_ready?'READY':'NOT LOADED';"

"document.getElementById('health').textContent=d.health.toFixed(2);"

"document.getElementById('irrigation').textContent=d.irrigation.toFixed(2);"

"document.getElementById('safety').textContent="
"d.max_runtime_fault?'MAX RUNTIME FAULT':"
"d.no_flow_fault?'NO FLOW FAULT':'OK';"

"}catch(e){"

"document.getElementById('connection').textContent='🔴 ESP32 disconnected';"

"}"

"}"

"update();"

"setInterval(update,2000);"

"</script>"

"</body>"
"</html>";

/*
 * -----------------------------------------
 * ROOT PAGE
 * -----------------------------------------
 */

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=UTF-8");

    return httpd_resp_send(
        req,
        DASHBOARD_HTML,
        HTTPD_RESP_USE_STRLEN
    );
}

/*
 * -----------------------------------------
 * JSON STATUS API
 * -----------------------------------------
 */

static esp_err_t status_handler(httpd_req_t *req)
{
    char json[512];

    snprintf(
        json,
        sizeof(json),

        "{"
        "\"soil\":%.2f,"
        "\"temperature\":%.2f,"
        "\"humidity\":%.2f,"
        "\"flow\":%.2f,"
        "\"day\":%s,"
        "\"pump\":%s,"
        "\"health\":%.2f,"
        "\"irrigation\":%.2f,"
        "\"ml_ready\":%s,"
        "\"no_flow_fault\":%s,"
        "\"max_runtime_fault\":%s"
        "}",

        latest_sensor_data.soil_moisture,
        latest_sensor_data.temperature,
        latest_sensor_data.humidity,
        latest_sensor_data.flow_rate,

        latest_sensor_data.is_day ? "true" : "false",

        latest_controller_status.pump_state == PUMP_ON
            ? "true"
            : "false",

        latest_ml_result.plant_health_score,

        latest_ml_result.irrigation_score,

        latest_ml_result.model_ready
            ? "true"
            : "false",

        latest_controller_status.no_flow_fault
            ? "true"
            : "false",

        latest_controller_status.max_runtime_fault
            ? "true"
            : "false"
    );

    httpd_resp_set_type(req, "application/json; charset=UTF-8");

    return httpd_resp_send(
        req,
        json,
        HTTPD_RESP_USE_STRLEN
    );
}

/*
 * -----------------------------------------
 * HTTP SERVER
 * -----------------------------------------
 */

static void start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = 8;

    httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(
        httpd_start(&server, &config)
    );

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };

    httpd_register_uri_handler(
        server,
        &root
    );

    httpd_uri_t status = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL
    };

    httpd_register_uri_handler(
        server,
        &status
    );

    ESP_LOGI(TAG, "HTTP server started");
}

/*
 * -----------------------------------------
 * WIFI ACCESS POINT
 * -----------------------------------------
 */

static void wifi_start_ap(void)
{
    ESP_LOGI(TAG, "Starting Wi-Fi Access Point");

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());

        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "AgriAid",
            .ssid_len = 0,
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        }
    };

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_AP)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_AP,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_start()
    );

    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "Wi-Fi AP started");
    ESP_LOGI(TAG, "SSID: AgriAid");
    ESP_LOGI(TAG, "Password: none");
    ESP_LOGI(TAG, "Dashboard: http://192.168.4.1");
    ESP_LOGI(TAG, "=================================");
}

void web_server_start(void)
{
    wifi_start_ap();

    start_http_server();
}

/*
 * -----------------------------------------
 * FUNCTIONS USED BY MAIN
 * -----------------------------------------
 *
 * We expose these internally through weak-ish
 * update functions for this prototype.
 */

void web_server_update_state(
    const sensor_data_t *sensor,
    const controller_status_t *controller,
    const ml_result_t *ml
)
{
    if (sensor != NULL) {
        latest_sensor_data = *sensor;
    }

    if (controller != NULL) {
        latest_controller_status = *controller;
    }

    if (ml != NULL) {
        latest_ml_result = *ml;
    }
}