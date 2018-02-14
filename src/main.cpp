#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <TFT_eSPI.h>
#include <esp_wifi.h>

/** Build time */
const char compileDate[] = __DATE__ " " __TIME__;
/** OTA progress */
int otaStatus = 0;
/** TFT_eSPI class for display */
TFT_eSPI tft = TFT_eSPI();

/** Function definition */
void activateOTA();
void printSTAconfig(bool toSerial);
void printAPconfig(bool toSerial);
String getAuthMode(wifi_auth_mode_t authmode);

void setup() {
	// Start serial connection
	Serial.begin(115200);

	// Get stored information before connecting to WiFi
    printSTAconfig(true);
    printAPconfig(true);

	// Initialize TFT screen
	tft.init();
	// Clear screen
	tft.fillScreen(TFT_BLACK);
	tft.setCursor(0, 40);
	tft.setTextColor(TFT_WHITE);

	// Put some information on the screen
	tft.println("Build: ");
	tft.setTextSize(1);
	tft.println(compileDate);

	// Connect to WiFi
	WiFi.mode(WIFI_STA);
	WiFi.begin("MHC2", "teresa1963");
	uint32_t startTime = millis();
	while (WiFi.waitForConnectResult() != WL_CONNECTED) {
		if (millis()-startTime > 30000) { // wait maximum 30 seconds for a connection
			tft.println("Failed to connect to WiFI");
			tft.println("Rebooting in 30 seconds");
			delay(30000);
			esp_restart();
		}
	}
	// WiFi connection successfull
	tft.println("Connected to ");
	tft.println(WiFi.SSID());
	tft.println("with IP address ");
	tft.println(WiFi.localIP());

	// Activate OTA
	activateOTA();
	tft.println("\n\nWaiting for OTA to start");
}

void loop() {
	ArduinoOTA.handle();
}

/**
 * Print saved STA configuration
 */
void printSTAconfig (bool toSerial) {
    String staConfigTxt;

	// Get stored information of WiFi STA
	wifi_config_t conf;
	WiFi.mode(WIFI_AP_STA);
	staConfigTxt = "Stored STA configuration:\n";
	staConfigTxt += "========================\n";
	esp_wifi_get_config(WIFI_IF_STA, &conf);
	staConfigTxt += "SSID: " + String(reinterpret_cast<const char*>(conf.sta.ssid)) + "\n";
	staConfigTxt += "Password: " + String(reinterpret_cast<const char*>(conf.sta.password)) + "\n";
	staConfigTxt += "BSSID: " + String(reinterpret_cast<const char*>(conf.sta.bssid)) + "\n";
	staConfigTxt += "Channel: " + String(conf.sta.channel) + "\n";
	if (conf.sta.scan_method == WIFI_FAST_SCAN) {
		staConfigTxt += "WiFi scan method fast\n";
	} else {
		staConfigTxt += "WiFi scan method all\n";
	}
	if (conf.sta.sort_method == WIFI_CONNECT_AP_BY_SIGNAL) {
		staConfigTxt += "WiFi scan results sorted by RSSI\n";
	} else {
		staConfigTxt += "WiFi scan results sorted by auth type\n";
	}
	wifi_fast_scan_threshold_t staTreshold = conf.sta.threshold;
	staConfigTxt += "Minimum RSSI accepted: " + String(staTreshold.rssi) + "\n";
	staConfigTxt += "Minimum required auth method: " + getAuthMode(staTreshold.authmode) + "\n";

    if (toSerial) {
        Serial.print(staConfigTxt);
    } else {
        tft.fillRect(0,64+32,128,128,TFT_BLUE);
        tft.setCursor(0,64+32+8);
        tft.print(staConfigTxt);
    }
}

/**
 * Print saved AP configuration
 */
void printAPconfig(bool toSerial) {
    String apConfigTxt;
	// Get stored information of WiFi AP
	wifi_config_t conf;
	WiFi.mode(WIFI_AP_STA);
	apConfigTxt = "\nStored AP configuration:\n";
	apConfigTxt += "=========================\n";
	esp_wifi_get_config(WIFI_IF_AP, &conf);
	apConfigTxt += "SSID: " + String(reinterpret_cast<const char*>(conf.ap.ssid)) + "\n";
	apConfigTxt += "Password: " + String(reinterpret_cast<const char*>(conf.ap.password)) + "\n";
	apConfigTxt += "Channel: " + String(conf.ap.channel) + "\n";
	if (conf.ap.ssid_hidden == 0) {
		apConfigTxt += "SSID is broadcasted\n";
	} else {
		apConfigTxt += "SSID is hidden\n";
	}
	apConfigTxt += "Auth method: " + getAuthMode(conf.ap.authmode) + "\n";
	apConfigTxt += "Max allowed connections: " + String(conf.ap.max_connection) + "\n";
	apConfigTxt += "Beacon interval: " + String(conf.ap.beacon_interval) + "ms\n";

    if (toSerial) {
        Serial.print(apConfigTxt);
    } else {
        tft.fillRect(0,64+32,128,128,TFT_BLUE);
        tft.setCursor(0,64+32+8);
        tft.print(apConfigTxt);
    }
}

/**
 * Get authentification mode
 */
String getAuthMode(wifi_auth_mode_t authmode) {
	switch (authmode) {
		case WIFI_AUTH_OPEN:
			return "OPEN";
			break;
		case WIFI_AUTH_WEP:
			return "WEP";
			break;
		case WIFI_AUTH_WPA_PSK:
			return "WPA PSK";
			break;
		case WIFI_AUTH_WPA2_PSK:
			return "WPA2 PSK";
			break;
		case WIFI_AUTH_WPA_WPA2_PSK:
			return "WPA WPA2 PSK";
			break;
		case WIFI_AUTH_WPA2_ENTERPRISE:
			return "WPA2 ENTERPRISE";
			break;
		case WIFI_AUTH_MAX:
			return "MAX";
			break;
	}
}

/**
 * Activate OTA
 */
void activateOTA() {
	uint8_t baseMac[6];
	// Get MAC address for WiFi station
	esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
	char baseMacChr[19] = {0}; 
	sprintf(baseMacChr, "ESP32-%02X%02X%02X%02X%02X%02X"
		, baseMac[0], baseMac[1], baseMac[2]
		, baseMac[3], baseMac[4], baseMac[5]);
	baseMacChr[18] = 0;

	ArduinoOTA
		.setHostname(baseMacChr)
		.onStart([]() {
			/**********************************************************/
			// Close app tasks and sto p timers here
			/**********************************************************/
			// Prepare LED for visual signal during OTA
			pinMode(16, OUTPUT);
			// Clear the screen and print OTA text
			tft.fillScreen(TFT_BLUE);
			tft.setTextDatum(MC_DATUM);
			tft.setTextColor(TFT_WHITE);
			tft.setTextSize(2);
			tft.drawString("OTA",64,50);
			tft.drawString("Progress:",64,75);
		})
		.onEnd([]() {
			// Clear the screen and print OTA finished text
			tft.fillScreen(TFT_GREEN);
			tft.setTextDatum(MC_DATUM);
			tft.setTextColor(TFT_BLACK);
			tft.setTextSize(2);
			tft.drawString("OTA",64,50);
			tft.drawString("FINISHED!",64,80);
			delay(10);
		})
		.onProgress([](unsigned int progress, unsigned int total) {
			// Calculate progress
			unsigned int achieved = progress / (total / 100);
			// Update progress every 1 %
			if (otaStatus == 0 || achieved == otaStatus + 1) {
				// Toggle the LED
				digitalWrite(16, !digitalRead(16));
				otaStatus = achieved;
				// Print the progress
				tft.setTextDatum(MC_DATUM);
				tft.setTextSize(2);
				tft.fillRect(32,91,64,28,TFT_BLUE);
				String progVal = String(achieved) + "%";
				tft.drawString(progVal,64,105);
			}
		})
		.onError([](ota_error_t error) {
			// Clear the screen for error message
			tft.fillScreen(TFT_RED);
			tft.setTextDatum(MC_DATUM);
			tft.setTextColor(TFT_WHITE);
			// Print error message
			tft.setTextSize(2);
			tft.drawString("OTA",64,60);
			tft.drawString("ERROR:",64,90);
			// Get detailed error and print error reason
			if (error == OTA_AUTH_ERROR) {
				tft.drawString("Auth Failed",64,120);
			}
			else if (error == OTA_BEGIN_ERROR) {
				tft.drawString("Begin Failed",64,120);
			}
			else if (error == OTA_CONNECT_ERROR) {
				tft.drawString("Connect Failed",64,120);
			}
			else if (error == OTA_RECEIVE_ERROR) {
				tft.drawString("Receive Failed",64,120);
			}
			else if (error == OTA_END_ERROR) {
				tft.drawString("End Failed",64,120);
			}
            otaStatus = 0;
		});

	// Initialize OTA
	ArduinoOTA.begin();

	// Add some extra service text to the mDNS service
	MDNS.addServiceTxt("_arduino", "_tcp", "service", "OTA-TEST");
	MDNS.addServiceTxt("_arduino", "_tcp", "type", "ESP32");
	MDNS.addServiceTxt("_arduino", "_tcp", "id", "BeeGee");
}
