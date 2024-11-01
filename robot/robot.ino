#define l1 13
#define l2 12
#define r1 14
#define r2 25

//int l1Val =0;
//int l2Val =0;
//int r1Val = 0;
//int r2Val = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
pinMode(l1,INPUT);
pinMode(l2,INPUT);
pinMode(r1,INPUT);
pinMode(r2,INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
   int l1Val = analogRead(l1);
    int l2Val = analogRead(l2);
    int r1Val = analogRead(r1);
     int r2Val = analogRead(r2);
     Serial.println("values:" );
     Serial.print(l1Val);
     Serial.print(" ,");
     Serial.print(l2Val);
     Serial.print(" ,");
     Serial.print(r1Val);
     Serial.print(" ,");
     Serial.print(r2Val);
     Serial.println(" ,");
     
     delay(500);
  
}
