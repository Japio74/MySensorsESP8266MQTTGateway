
void setup_led();
void set_gateway_mode_application(bool app_mode);
void setTransportInitError(bool error);
void triggerReceiveLed();
void triggerSentLed();
void triggerMqttLed();
void triggerTxError();
void setWifiLed(bool connected);
void setWifiStrengthLed(int strength);
void loopLed();
void setControllerConnectedLed(bool connected);



void setRunningState(bool running);
