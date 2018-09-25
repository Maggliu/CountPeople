#include<SoftwareSerial.h>
SoftwareSerial wifiSerial(4, 5);//设置wifi用软串口
SoftwareSerial blueSerial(6, 7);
String ssid;//wifi名
String passcode;//wifi密码
const int trig1 = 10;//1号超声波管脚
const int echo1 = 9;
const int trig2 = 12;//2号超声波管脚
const int echo2 = 11;
float distance1;//即时距离
float distance2;
float standDistance1 = 0.0;//标准距离，判断的标准
float standDistance2 = 0.0;
float lastDistance1 = 0.0;//上一次的测量距离
float lastDistance2 = 0.0;
int timeCount = 0;//时间计数
int firstTime;//1号超声波触发时间
int secondTime;//2号超声波触发时间
int delayTime;//差距时间
int temp = 0;
boolean netok = false; //网络准备标志
boolean wifiOk = false;
void setup() {
  // put your setup code here, to run once:
  wifiSerial.begin(115200);
  blueSerial.begin(9600);
  pinMode(trig1, OUTPUT);
  pinMode(trig2, OUTPUT);
  pinMode(echo1, INPUT);
  pinMode(echo2, INPUT);
  Serial.begin(115200);//开启串口
  setWifi();
  connectWifi();//链接wifi
  delay(5000);
  connectServer();//链接服务器
  delay(5000);
  //  attachInterrupt(0, doorchange, CHANGE);//2，3管脚设置电平变化触发
  //  attachInterrupt(1, manchange, CHANGE);
}
void setWifi() {
  blueSerial.listen();
  boolean firsts=true;
  while (true) {
    while (blueSerial.available()) {
      if(firsts){
        blueSerial.read();
        blueSerial.read();
        Serial.print("f");
        }
        firsts=false;
      ssid += (char)blueSerial.read();
      if ((char)blueSerial.peek() == '%') {
        blueSerial.read();
        wifiOk = true;
        break;
      }
    }

    while (blueSerial.available()) {
      passcode += (char)blueSerial.read();
    }
    Serial.print(ssid);
    if (wifiOk)
      break;
    delay(300);
  }
}
void connectWifi() {//链接wifi
  wifiSerial.listen();
  Serial.print("wificonnecting\n");
  wifiSerial.print("AT\r\n");
  delay(1000);
  while (wifiSerial.available())
    Serial.print((char)wifiSerial.read());
  wifiSerial.print("AT+CWJAP=\"" + ssid + "\",\"" + passcode + "\"\r\n"); //接入wlan
  delay(1000);
  while (wifiSerial.available())
    Serial.print((char)wifiSerial.read());
}
void connectServer() {//以TCP方式链接服务器
  wifiSerial.print("AT+CIPSTART=\"TCP\",\host\",80\r\n");
  delay(1000);
  wifiSerial.print("AT+CIPMODE=1\r\n");
  delay(500);
  wifiSerial.print("AT+CIPSEND\r\n");
  delay(500);
  netok = true;
  while (wifiSerial.available())
    Serial.print((char)wifiSerial.read());//返回wifi软串口数据，调试用
}

void updatePeople(String a) { //将变化情况发送至服务器
  wifiSerial.print("GET http://host/countPeople.php?what="  + a + "\r\n");
}
void trigThetrig1() {//触发 超声波函数
  digitalWrite(trig1, LOW);
  delay(5);
  digitalWrite(trig1, HIGH);
  delay(10);
  digitalWrite(trig1, LOW);
}
void trigThetrig2() {
  digitalWrite(trig2, LOW);
  delay(5);
  digitalWrite(trig2, HIGH);
  delay(10);
  digitalWrite(trig2, LOW);
}
void loop() {
  // put your main code here, to run repeatedly:
  trigThetrig1();//触发及测距
  distance1 = pulseIn(echo1, HIGH) / 58.00;
  trigThetrig2();
  distance2 = pulseIn(echo2, HIGH) / 58.00;
  //初始化标准距离
  if (abs(distance1 - lastDistance1) < 8 && abs(distance2 - lastDistance2) < 8)
    timeCount += 100;
  else {
    timeCount = 0;
    lastDistance1 = distance1;
    lastDistance2 = distance2;
  }
  if (timeCount == 3000) {//当时间计数标志为3000时，认为此时超声波已固定，可以开始检测
    standDistance1 = lastDistance1;
    standDistance2 = lastDistance2;
    timeCount = 0;
  }
  if (standDistance1 != 0.0) {
    //1号超声波测得的距离于标准距离大于20cm，判定为触发，记录此时的触发时间
    if (standDistance1 - distance1 > 20) {
      firstTime = millis();
      Serial.println("first t");
    }
    if (standDistance2 - distance2 > 20) {//同上
      secondTime = millis();
      Serial.println("second t");
    }
  }
  delayTime = firstTime - secondTime; //计算时间差
  if (netok) {
    //判断时间差是否符合判断进出的要求，符合即向服务器更新
    if (delayTime > 300 && delayTime < 20000 && firstTime != 0 && secondTime != 0) {
      Serial.println(1);
      updatePeople("1");
      firstTime = secondTime = delayTime = 0;
      delay(1000);
    }
    if (delayTime < -300 && delayTime > -20000 && firstTime != 0 && secondTime != 0) {
      Serial.println(-1);
      updatePeople("-1");
      firstTime = secondTime = delayTime = 0;
      delay(1000);
    }
  }
  while (wifiSerial.available())
    Serial.print((char)wifiSerial.read());//返回wifi软串口数据，调试用
}
