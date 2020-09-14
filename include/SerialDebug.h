#ifdef SERIAL_DEBUG
  #define SerialDebugln(x) Serial.println(x);
  #define SerialDebug(x) Serial.print(x);
#else
  #define SerialDebug(x) (void)0;
  #define SerialDebugln(x) (void)0;
#endif