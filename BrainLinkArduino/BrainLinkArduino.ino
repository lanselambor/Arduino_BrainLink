/**
 * This is a BrainLink application on machain control
 */
////////////////////////////////////////////////////////////////////////
// Arduino Bluetooth-Serial Interface with Mindwave
//Arduino串口解析脑波蓝牙串口数据流，并解析噪声，专注力及放松力。
////////////////////////////////////////////////////////////////////////

/**
 * BlueTooth  
 */
#define LED  2
#define BAUDRATE 38400
#define DEBUGOUTPUT 0
String BT_DATA = "";

// checksum variables
byte generatedChecksum = 0; //根据累加计算的校验和
byte checksum = 0;          //蓝牙数据包传输过来的校验和
int payloadLength = 0;      //有效数据长度
byte payloadData[64] = {    //有效数据存放数组缓冲区
  0};
byte poorQuality = 0;       //脑波噪声
byte attention = 0;         //注意力
byte meditation = 0;        //放松力

// system variables
long lastReceivedPacket = 0;
boolean bigPacket = false;

int d_attention = 0;
int last_attention = 0;
int attention_rate = 1;


/**
 * Steper pin definition
 */
#define MOTOR_CLOCKWISE      0
#define MOTOR_ANTICLOCKWISE  1
/******Pins definitions*************/
#define MOTORSHIELD_IN1  8
#define MOTORSHIELD_IN2 11
#define MOTORSHIELD_IN3 12
#define MOTORSHIELD_IN4 13
#define CTRLPIN_A   9
#define CTRLPIN_B   10 

#define LIMIT_SWICTH_DOWN     3
#define LIMIT_SWICTH_UP       4
#define  DROPDOWNTIME  5000
#define  RESTART_TIME  10000
unsigned long restart_Time = 0;
unsigned long dropdownTime = 0;
  
const unsigned char stepper_ctrl[]={0x27,0x36,0x1e,0x0f};

struct MotorStruct {
    uint8_t speed;
    uint8_t direction;
    uint8_t position;
};


MotorStruct stepperMotor;
unsigned int number_of_steps = 200;

void step(int steps)
{    
  int steps_left = abs(steps)*4;
  int step_number;
  int millis_delay = 60L * 1000L /number_of_steps/(stepperMotor.speed + 50);
  if (steps > 0) 
  {
    stepperMotor.direction= MOTOR_ANTICLOCKWISE;
    step_number = 0; 
  }
    else if (steps < 0) 
  {
    stepperMotor.direction= MOTOR_CLOCKWISE;
    step_number = number_of_steps;
  }
  else return;
  while(steps_left > 0) 
  {
    PORTB = stepper_ctrl[step_number%4];
    delay(millis_delay);
    if(stepperMotor.direction== MOTOR_ANTICLOCKWISE)
    {
      step_number++;
        if (step_number == number_of_steps)
          step_number = 0;
    }
    else 
    {
      step_number--;
        if (step_number == 0)
          step_number = number_of_steps;
    }
    steps_left --;
    
  }
}

void steper_init()
{  
  pinMode(LIMIT_SWICTH_DOWN, INPUT);  //limit switch  bottom
  pinMode(LIMIT_SWICTH_UP, INPUT);    //limit switch  top
  
  pinMode(MOTORSHIELD_IN1,OUTPUT);
  pinMode(MOTORSHIELD_IN2,OUTPUT);
  pinMode(MOTORSHIELD_IN3,OUTPUT);
  pinMode(MOTORSHIELD_IN4,OUTPUT);
  pinMode(CTRLPIN_A,OUTPUT);
  pinMode(CTRLPIN_B,OUTPUT);
  
  stop();
  stepperMotor.speed = 25;
  stepperMotor.direction = MOTOR_CLOCKWISE;
  
  //init board original height
  while(digitalRead(LIMIT_SWICTH_DOWN) != LOW){
    step(1);
  }
  stop();
  Serial.println("Step go to bottom O.K!");    
}

// Stop steper
void stop()
{
  /*Unenble the pin, to stop the motor. */
  digitalWrite(CTRLPIN_A,LOW);
  digitalWrite(CTRLPIN_B,LOW);
}


//////////////////////////
// Microprocessor Setup //
//////////////////////////
void setup() {  
  pinMode(LED, OUTPUT);
  Serial.begin(BAUDRATE);   // 设置波特率，蓝牙串口的波特率,          
  while(!Serial){}
  Serial.println("Begin...");
  steper_init();  
  delay(100);
    
  dropdownTime = millis();
  restart_Time = millis();
}

////////////////////////////////
// 从串口接收数据
////////////////////////////////
byte ReadOneByte() {
  int ByteRead;

  while(!Serial.available());
  ByteRead = Serial.read();

#if DEBUGOUTPUT  
  Serial.print((char)ByteRead);   // echo the same byte out the USB serial (for debug purposes)
#endif

  return ByteRead;
}

/////////////
//MAIN LOOP//
/////////////
void loop() {  
#if 1  
  // If no data reveived for 10 seconds, board reset
  if(millis() - dropdownTime >= DROPDOWNTIME){
    steper_init();
    dropdownTime = millis();
    last_attention = 0;
    Serial.println("board reset!");
  }
#endif  
  // Protection for height over, bottom
  if(digitalRead(LIMIT_SWICTH_DOWN) == LOW){
    stop();
    Serial.println("Touch bottom limit switch!");
    last_attention = 0;
  }
  
#if 0  
  // Protection for height over, top
  if(digitalRead(LIMIT_SWICTH_UP) == LOW){
    steper_init();
    Serial.println("Touch top limit switch!");
    last_attention = 100;
  }
#endif  
  
  // 首先寻找同步字节，2个AA，代表完整数据包开始传输
  if(ReadOneByte() == 170) {                       //十进制170对应十六进制AA
    if(ReadOneByte() == 170) {                     //十进制170对应十六进制AA，连续监测2个AA即为脑波数据的数据头

      payloadLength = ReadOneByte();               //数据头监测完毕后，读取下一个字节，代表后面数据的长度
      if(payloadLength > 169)                      //Payload length can not be greater than 169
          return;                                  //后面数据个数不可能大于170个，所以一旦大于，直接返回，回到第一步继续监测数据头

      generatedChecksum = 0;                       //如果得到的代表数据长度的小于170，说明正确，继续接收下面的字节
      for(int i = 0; i < payloadLength; i++) {     //由于知道了后面数据的个数，用for循环，将后面所有有用数据全部读取，并存放缓存数组
        payloadData[i] = ReadOneByte();            //Read payload into memory
        generatedChecksum += payloadData[i];       //累加后面的数据，计算校验和
      }   

      checksum = ReadOneByte();                      //Read checksum byte from stream      
      generatedChecksum = 255 - generatedChecksum;   //Take one's compliment of generated checksum

        if(checksum == generatedChecksum) {          //将计算的校验字节和读取的数据流中的校验字节比较，是否相等

        poorQuality = 200;
        attention = 0;
        meditation = 0;                              //将要得到的数据都初始化，下面读取最新的实际值

        for(int i = 0; i < payloadLength; i++) {    // Parse the payload 解析数据包
          switch (payloadData[i]) {
          case 2:                                   //2是噪声信号强度,下一个就是数据0-255
            i++;            
            poorQuality = payloadData[i];
            bigPacket = true;            
            break;
          case 4:                                   //4是专注力信号强度，下一个就是数据0-255
            i++;
            attention = payloadData[i];                        
            break;
          case 5:                                   //5是放松冥想力强度，下一个就是数据0-255
            i++;
            meditation = payloadData[i];
            break;
          case 0x80:
            i = i + 3;
            break;
          case 0x83:
            i = i + 25;      
            break;
          default:
            break;
          } // switch
        } // for loop               
        
#if !DEBUGOUTPUT

        // 这里写应用代码

        if(bigPacket) {
          if(poorQuality == 0)
            digitalWrite(LED, HIGH);
          else
            digitalWrite(LED, LOW);
#if 1            
          Serial.print("PoorQuality: ");
          Serial.print(poorQuality, DEC);
          Serial.print(" Attention: ");
          Serial.print(attention, DEC);
          Serial.print(" Time since last packet: ");
          Serial.print(millis() - lastReceivedPacket, DEC);
          lastReceivedPacket = millis();
          Serial.print("\n"); 

          // calculate delta attention and output to steper            
          d_attention = attention - last_attention;
          Serial.print("d_attention: ");
          Serial.println(d_attention);
          step(-d_attention * attention_rate);
          last_attention = attention;
          
          dropdownTime = millis();  //init dropdownTime
#endif                             
        }     
      
#endif        
        bigPacket = false;        
      }                    
      else {
        Serial.println("Checksum Error...");  
        // Checksum Error
      }  // end if else for checksum
    } // end if read 0xAA byte
  } // end if read 0xAA byte    
}


