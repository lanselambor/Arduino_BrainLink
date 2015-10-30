#include "SoftwareSerial.h"

SoftwareSerial ble(7, 8); // RX, TX
String BT_DATA = "";

void setup() { 
  Serial.begin(115200);
  while(!Serial){
  }
  Serial.println("Begin test...");
  ble.begin(38400);
  delay(100);
  SetupMasterMode();  
  //setupSlaveMode();
}

void loop() {  
  while(ble.available()) //Receivedata              
  {                                                             
    Serial.print(char(ble.read())); 
  }
  
  if(Serial.available())  //Tx data
  {  
    do{
      BT_DATA += char(Serial.read());
      delay(2);
    }while (Serial.available() > 0);
    
    if (BT_DATA.length() > 0){
      ble.println(BT_DATA);
      Serial.println(BT_DATA);
      BT_DATA = "";
    }
  } 
}

void SetupMasterMode()
{
  sendBlueToothCommand("AT\r\n");
  sendBlueToothCommand("AT+ORGL\r\n");  
  sendBlueToothCommand("AT+NAME=HC-05\r\n");//命名模块名
  sendBlueToothCommand("AT+ROLE=1\r\n");//设置主从模式：0从机，1主机
  sendBlueToothCommand("AT+BIND=8CDE,52,9294C0\r\n");
  sendBlueToothCommand("AT+PSWD=0000\r\n");//设置配对密码，如0123
  sendBlueToothCommand("AT+INIT\r\n");
  sendBlueToothCommand("AT+BIND\r\n");
  sendBlueToothCommand("AT+ADDR\r\n");
  sendBlueToothCommand("AT+LINK=8CDE,52,9294C0\r\n");
  sendBlueToothCommand("AT+PAIR=8CDE,52,9294C0\r\n");
  delay(100);
  Serial.println("Master mdoe setup complete"); 
}
void setupSlaveMode()
{
  sendBlueToothCommand("AT\r\n");
  sendBlueToothCommand("AT+NAME=HC-05\r\n");//命名模块名
  sendBlueToothCommand("AT+ROLE=0\r\n");//设置主从模式：0从机，1主机
  sendBlueToothCommand("AT+PSWD=1234\r\n");//设置配对密码，如0123
  sendBlueToothCommand("AT+UART=38400,0,0\r\n");//设置波特率9600，停止位1，校验位无 
  sendBlueToothCommand("AT+RMAAD\r\n");//清空配对列表
  delay(100);
  Serial.println("Slave mdoe setup complete"); 
}

void sendBlueToothCommand(char command[])
{
  char a;
  ble.print(command);
  Serial.print(command);                        
  delay(200);
  while(ble.available())            
  {                                                
     Serial.print(char(ble.read())); 
  }
  delay(100);                                             
}

  

