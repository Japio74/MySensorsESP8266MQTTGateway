

#define MAX_NODES (10)
#define MAX_SENSORS (30)

typedef struct 
{
  String        sketchName;
  String        sketchVersion;
  String        mySensorsVersion;
  int           nodeId;
  unsigned long presentationTime;
} nodeInfoType;

typedef struct 
{
  int nodeId;
  int sensorId;
  int type;
  int unit;
  char data[20];
  unsigned long presented;
  unsigned long lastSet;
} sensorInfoType;

