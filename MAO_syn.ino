#include <Encoder.h>
#include <EEPROM.h>
#include <TroykaTextLCD.h>

#define LED A3                          // пин управления триггером со светодиодом
#define BUTTON A0                       // пин кнопки 
#define PHASE_CONTROL 0                 // пин контроля фазы процесса
#define SSR 13                          // пин управления твердотельным реле

TroykaTextLCD lcd;                      // объект для работы с дисплеем
Encoder knob (12, A1);                  // объект для работы с энкодером

volatile long pulse_counter;            //счетчик импульсов
volatile long front_counter = 0;        //счетчик импульсов
long trigger_delay;                     //задержка сигнала триггера
bool start_phase;                       //контрольный флаг запуска процесса. true-анодная фаза; false-катодня
bool process_running = false;           //флаг состояния запуска процесса
volatile bool process_phase;            //индикатор фазы МДО-процесса. true-анодная фаза; false-катодня

long int address = 0;                   //начальный адрес хранения данных в ПЗУ

void setup()
{
  pinMode(LED, OUTPUT);           
  pinMode(BUTTON, INPUT);         
  pinMode(SSR, OUTPUT);           
  pinMode(PHASE_CONTROL, INPUT);  
  attachInterrupt(digitalPinToInterrupt(PHASE_CONTROL), phase_switch, CHANGE);
 
  long knob_position = -999;
  long newposition;

  lcdSetup();

/*-------------------Задание фазы запуска процесса-------------------------*/
    lcd.print("Start Phase");
    lcd.setCursor(0, 1);
    knob_position = -999;
    knob.readAndReset();
    while (digitalRead(BUTTON))
    {
        newposition = knob.read();
        if(newposition != knob_position)
        {
            if(newposition%8)
            {
                lcd.setCursor(0, 1);
                lcd.print("KATOD");
                start_phase = false;
             }
            else 
            {
                lcd.setCursor(0, 1);
                lcd.print("ANOD ");
                start_phase = true;
             }
            knob_position = newposition;
        }
     }
     knob.readAndReset();
     delay(200);
     lcd.clear();
}

void loop()
{
//EEPROM.put(address, long(0));
  EEPROM.get(address, pulse_counter);
  long knob_position = -999;
  long newposition;

/*---------------Установка счетчика импульсов----------------*/
  lcd.print("Set The Counter!");
  lcd.setCursor(0, 1);
  lcd.print("20ms x ");
  
  while (digitalRead(BUTTON))
  {
    newposition = pulse_counter + knob.read();
    if(newposition != knob_position)
    {
      lcd.print("         ");
      lcd.setCursor(7, 1);
      lcd.print(String(newposition));
      lcd.setCursor(7, 1);
      knob_position = newposition;
     }
   }
   knob.readAndReset();
   delay(200);
   pulse_counter = knob_position;
   EEPROM.put(address, pulse_counter);
   lcd.clear();

/*-----------------Установка значения задержки---------------------*/
  lcd.print(" Trigger delay! ");
  lcd.setCursor(0, 1);
  lcd.print("20ms x ");
  
  while (digitalRead(BUTTON))
  {
    newposition = knob.read();
    if(newposition != knob_position)
    {
      lcd.print("         ");
      lcd.setCursor(7, 1);
      lcd.print(String(newposition));
      lcd.setCursor(7, 1);
      knob_position = newposition;
     }
   }
   knob.readAndReset();
   delay(200);
   trigger_delay = knob_position;
   lcd.clear();

/*--------Индикация состояния готовности и ожидание запуска процесса---------*/
  lcd.print(" Trigger Ready!");
  lcd.setCursor(0, 1);
  lcd.print("PUSH THE BUTTON!");
  while (digitalRead(BUTTON));          // ждем нажатия кнопки запуска

 
 /*-------------------------Процесс запущен----------------------------------*/ 
  
  if(trigger_delay == 0) digitalWrite(LED, HIGH);   // если задержка срабатывания триггера не задана, выдаем импульс немедлено
   /* ждем нужной фазы процесса*/
  long i = front_counter;           //вспомогательная переменная 
  if (process_phase==start_phase)  // если фаза запуска совпадает с текущей фазой процесса ждем один переход       
    front_counter -= 1;
  else                             // если фаза запуска не совпадает с текущей фазой процесса ждем два переход а
    front_counter -= 2;
  while (front_counter < i);
  delay(5);

  digitalWrite(SSR, HIGH);                                        //включаем реле
  process_running = true;
  trigger_delay = pulse_counter - trigger_delay;
 
  lcd.clear();
  lcd.print("   RELAY ON!    ");
  lcd.setCursor(0, 1);
  lcd.print("Counter=");
  lcd.print(pulse_counter);
    
  while(pulse_counter > 0)                                        //ждем пока не обнулиться счетчик импульсов
  {
    if((trigger_delay - 1) == pulse_counter) digitalWrite(LED, HIGH);   // ждем наступления момента запуска триггера
    else digitalWrite(LED, LOW);
    
    if(!(pulse_counter%50))              // каждые 50 циклов (1 сек) печатать на экране значение счетчика
    {
      lcd.setCursor(8, 1);
      lcd.print(String(String(pulse_counter)) + ' ');
    }
  }
  delay(15);                  // чтобы последний полупериод не отрезало
  digitalWrite(SSR, LOW);    //выключаем реле
  process_running = false;
  
  lcd.clear();
  lcd.print("   RELAY OFF!   ");
  lcd.setCursor(0, 1);
  lcd.print("    Restart?    ");
  while (digitalRead(BUTTON));          // ждем нажатия кнопки запуска
  delay(200);
  lcd.clear();
}

void lcdSetup(void)
{
  // устанавливаем количество столбцов и строк экрана
  lcd.begin(16, 2);
  // устанавливаем контрастность в диапазоне от 0 до 63
  lcd.setContrast(25);
  // устанавливаем яркость в диапазоне от 0 до 255
  lcd.setBrightness(250);
  // устанавливаем курсор в колонку 0, строку 0
  lcd.setCursor(0, 0);
}

void phase_switch()
{
  //delay (2);
  front_counter++;
  process_phase = !digitalRead(PHASE_CONTROL);
  if(process_running)
  {
    if(process_phase == start_phase) pulse_counter--;
  }
}
