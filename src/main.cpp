#include <Arduino.h>
#include <az3166WiFi.h>
#include <IoT_DevKit_HW.h>
#include <MQTTClient.h>

// global variables
int arrivedcount = 0;
bool hasWifi = false;
const char *mqttServer = "192.168.127.204";
int port = 1883;

// function declaration
void messageArrived(MQTT::MessageData &md);

void initWifi(void);

void setup()
{
  // put your setup code here, to run once:

  // Initialize the serial port
  Serial.begin(115200);
  while (!Serial)
  {
    delay(500);
  }

  Screen.clean();
  Screen.print("IoT DevKit\r\n \r\nConnecting...\r\n");

  // Initialize the WiFi module
  initWifi();

  // initialize device
  int rc = initIoTDevKit(1);

  if (rc)
  {
    Screen.clean();
    Screen.print(1, "Init failed");
    Serial.println("Device Init failed");
    Serial.print("Error code: ");
    Serial.println(rc);

    while (1)
      ;
  }

  delay(2000);
}

void loop()
{
  // put your main code here, to run repeatedly:
}

void initWifi()
{
  Screen.print(0, "IoT DevKit\r\n \r\nConnecting...\r\n");

  if (WiFi.begin() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    Screen.print(1, ip.get_address());
    hasWifi = true;
    Screen.print(2, "Running... \r\n");
  }
  else
  {
    Screen.print(1, "No Wi-Fi\r\n ");
  }
}

int runMqttExample()
{
  char topic[128];
  sprintf(topic, "data/home/%s/%s", "livingroom", "az3166");

  MQTTNetwork mqttNetwork;
  MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(mqttNetwork);
  arrivedcount = 0;

  char msgBuf[100];
  sprintf(msgBuf, "Connecting to MQTT server %s:%d", mqttServer, port);
  Serial.println(msgBuf);

  int rc = mqttNetwork.connect(mqttServer, port);
  if (rc != 0)
  {
    Serial.println("Connected to MQTT server failed");
  }
  else
  {
    Serial.println("Connected to MQTT server successfully");
  }

  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.MQTTVersion = 3;
  data.clientID.cstring = "mqtt-sample";
  data.username.cstring = "";
  data.password.cstring = "";

  if ((rc = client.connect(data)) != 0)
  {
    Serial.println("MQTT client connect to server failed");
  }

  if ((rc = client.subscribe(topic, MQTT::QOS2, messageArrived)) != 0)
  {
    Serial.println("MQTT client subscribe from server failed");
  }

  MQTT::Message message;

  // QoS 0
  char buf[100];
  sprintf(buf, "QoS 0 message from AZ3166!");
  message.qos = MQTT::QOS0;
  message.retained = false;
  message.dup = false;
  message.payload = (void *)buf;
  message.payloadlen = strlen(buf) + 1;
  rc = client.publish(topic, message);
  while (arrivedcount < 1)
  {
    client.yield(100);
  }

  // QoS 1
  sprintf(buf, "QoS 1 message from AZ3166!");
  message.qos = MQTT::QOS1;
  message.payloadlen = strlen(buf) + 1;
  rc = client.publish(topic, message);

  while (arrivedcount < 2)
  {
    client.yield(100);
  }

  if ((rc = client.unsubscribe(topic)) != 0)
  {
    Serial.println("MQTT client unsubscribe from server failed");
  }

  if ((rc = client.disconnect()) != 0)
  {
    Serial.println("MQTT client disconnect from server failed");
  }

  mqttNetwork.disconnect();
  Serial.print("Finish message count: ");
  Serial.println(arrivedcount);

  return 0;
}

void messageArrived(MQTT::MessageData &md)
{
  MQTT::Message &message = md.message;

  char msgInfo[60];
  sprintf(msgInfo, "Message arrived: qos %d, retained %d, dup %d, packetid %d", message.qos, message.retained, message.dup, message.id);
  Serial.println(msgInfo);

  sprintf(msgInfo, "Payload: %s", (char *)message.payload);
  Serial.println(msgInfo);
  ++arrivedcount;
}