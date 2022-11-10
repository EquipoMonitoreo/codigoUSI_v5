/*
 *  PROGRAMA EN DESARROLLO
 *  
 *  Version 5 programa integrado USI.
 *  
 *  Incluye display color tft touch 2.4"
 *  
 */

#include <NewPing.h>
#include <SoftwareSerial.h>
#include <SIM808.h>
#include <EEPROM.h>
#include <Contenedor.h>
#include <TimerThree.h>
#include "MCUFRIEND_kbv.h"
//#include <Fonts/Branding_Bold72pt7b.h>
//#include <Fonts/Branding_Bold62pt7b.h>
#include <Fonts/Branding_Bold52pt7b.h>
#include <Fonts/Branding_Bold18pt7b.h>
#include <Fonts/Branding_Medium10pt7b.h>
#include <TouchScreen.h>
#include "config.h"
#include "iconos.h"     // Imagen logo
#include "colores.h"

MCUFRIEND_kbv tft;

#define LOWFLASH (defined(__AVR_ATmega328P__) && defined(MCUFRIEND_KBV_H_))

//#define TOUCH_DELAY 50
//bool reading;
//bool actualState;
//bool lastState;
//unsigned long lastDebounceTime;
 


//------ Configurar touch screen ------
// max: tp.x = 821, tp.y = 821; min: tp.x = 178, tp.y = 125
const int XP=8,XM=A2,YP=A3,YM=9; //240x320 ID=0x9341  (valores resultado de calibracion)
const int TS_LEFT=144,TS_RT=883,TS_TOP=100,TS_BOT=862;  // valores de calibracion

uint16_t xpos, ypos;  //screen coordinates
bool flagTouch = false;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;


//------ Establecer Pines ------
//SIM808:
const char pin_Rx = 51;   // pines tx y rx de arduino
const char pin_Tx = 49;
const char RST = 53;
const char PWR = 47;
//Ultrasonido:
const char pinUS_echo = 39;
const char pinUS_trigger = 37;
//Infrarrojos:
const char pinIR1 = 21;
const char pinIR2 = 20;
//Jumper borrar EEPROM:
const byte eepromPin = 43;     // Pin para borrar memoria eeprom en la direccion establecida.
//LEDs:
const byte pinLedVerde = 41;      // Pin modulo rele canal correspondiente a led Verde.
const byte pinLedRojo = 45;       // Pin modulo rele canal correspondiente a led Rojo.
const byte pinLock = 35;          // Cerradura - solenoide
const byte statusLed = 33;        // Led indicador de estado.
//Sensor magnetico apertura:
const byte pinDoor = 19;

//------ Declaracion Variables Globales ------
String URLString = "https://monitoreoambiental.sanjuan.gob.ar/api/62d01da68401a5082fbfd8f3/00/";    // EDITAR ACA URL INTEGRACION. USI Tecnopolis

volatile bool IR1_act = false;
bool  bottleFlag = false;
volatile bool timerFlag = false;
unsigned int timerCount = 0;

bool flagShowCode = false;

uint16_t contBotellas = 0;      //Contador de botellas ingresadas por usuario.

char resultCode[8];             // Variable global que almacena el codigo a mostrar por pantalla

int nivelCm = 0;

bool simStatus = false;
bool estadoEnvio = false;
bool resendFlag = false;
unsigned long lastTime = 0;
uint8_t cantIntentos = 0;

unsigned long lastTimeBlink = 0;

int eeAddress = 0;              //Direccion memoria EEPROM.

float m, b;

bool flagDoor = false;
//bool contenedorLleno = false;

//------ Declarar Objetos ------
NewPing sonar(pinUS_trigger, pinUS_echo, 200);
SoftwareSerial SIM(pin_Rx, pin_Tx);
SIM808 sim808(RST, PWR);
Contenedor ambiente(&sim808, &SIM, URLString);      // Crea objeto 'ambiente' de la clase Contenedor.

//------ Declaracion Subrutinas ------
//void updateTime();    // Subrutina Timer3


//-------------------------- Funciones pantalla --------------------------

void init_display(){
  uint16_t ID;
  
  ID = tft.readID();
  Serial.print(F("Display driver ID = 0x"));
  Serial.println(ID, HEX);
  tft.begin(ID);
  
  // Mostrar dimensiones:
  Serial.print("Width: "); Serial.println(String(tft.width()));     // Ancho
  Serial.print("Height: "); Serial.println(String(tft.height()));   // Alto
  
}

void progressBar(int x, int y){
  tft.fillRect(x, y, 50, 10, RED);
  delay(500);
  tft.fillRect(x+50, y, 30, 10, RED);
  delay(500);
  tft.fillRect(x+80, y, 80, 10, RED);
  delay(500);
  tft.fillRect(x+160, y, tft.width(), 10, RED);
  delay(500);
}

void showmsgXY(int x, int y, int sz, const GFXfont *f, String msg, uint16_t fontColor)     // Probando const char *msg --> String &msg
{
    int16_t x1, y1;
    unsigned int str_len = msg.length()+2;
    char localBuff[str_len];

    msg.toCharArray(localBuff, str_len);

    tft.setFont(f);
    tft.setCursor(x, y);
    tft.setTextColor(fontColor);
    tft.setTextSize(sz);
    tft.print(localBuff);
    delay(100);
}

void pantallaPrincipal(int nivel_Llenado, uint32_t totalBotellas, bool estadoSIM = true){

  String buffDisplay;
  
  tft.fillScreen(USI_GREEN2);    // Fondo
  tft.fillRect(5, 5, 309, 49, USI_NATURAL2); // Rectangulo cabecera

  // Texto
  showmsgXY(12, 24, 1, &Branding_Medium10pt7b, "Nivel de llenado: ", USI_GREEN2);
  buffDisplay = String(nivel_Llenado);
  buffDisplay = buffDisplay + " %";
  showmsgXY(165, 24, 1, &Branding_Medium10pt7b, buffDisplay, USI_GREEN2);
  
  showmsgXY(12, 45, 1, &Branding_Medium10pt7b, "Cantidad de botellas: ", USI_GREEN2);
  buffDisplay = String(totalBotellas);
  showmsgXY(210, 45, 1, &Branding_Medium10pt7b, buffDisplay, USI_GREEN2);
  
  showmsgXY(60, 82, 1, &Branding_Bold18pt7b, "BIENVENIDOS", USI_NATURAL2);
  //showmsgXY(70, 220, 1, &Branding_Medium10pt7b, "Presione para iniciar", USI_NATURAL2);

  if(ambiente.info.nivel < LIMITE_LLENADO){ showmsgXY(70, 220, 1, &Branding_Medium10pt7b, "Presione para iniciar", USI_NATURAL2); }
  else{showmsgXY(90, 220, 1, &Branding_Medium10pt7b, "Unidad Llena", USI_NATURAL2); }
  
  // Iconos
  if(estadoSIM){ tft.drawBitmap(281, 18, networkIcon, 22, 26, USI_GREEN2); }
  tft.drawBitmap(110, 96, logoUSI, 100, 95, USI_NATURAL2);

}

void pantallaFin(){

  tft.fillRect(0, 55, 310, 180, USI_GREEN2);  // Rectangulo borrar

  // Iconos
  tft.drawBitmap(17, 96, fondoCuadrado, 129, 119, USI_NATURAL2);
  tft.fillRect(21, 100, 115, 110, USI_NATURAL2);                    // Rectangulo nro botellas
  showmsgXY(45, 185, 1, &Branding_Bold52pt7b, "0", USI_GREEN2);     // Contador inicia en cero
  showmsgXY(30, 210, 1, &Branding_Medium10pt7b, "Botella(s)", USI_GREEN2);
  
  tft.drawBitmap(174, 96, fondoCuadrado, 129, 119, USI_ORANGE2);
  tft.fillRect(178, 100, 115, 110, USI_ORANGE2);  // Rectangulo cruz
  tft.drawBitmap(198, 116, logoCruz, 77, 71, USI_GREEN2);
  showmsgXY(198, 210, 1, &Branding_Medium10pt7b, "Finalizar", USI_GREEN2);
  
}

void pantallaCodigo(const char* text){

  //tft.fillScreen(USI_GREEN2);    // Fondo
  //tft.fillRect(5, 5, 309, 49, USI_NATURAL2); // Rectangulo cabecera

  // Actualizar cant botellas en cabecera:
  tft.fillRect(5, 28, 280, 20, USI_NATURAL2);   // borrar
  showmsgXY(12, 45, 1, &Branding_Medium10pt7b, "Cantidad de botellas: ", USI_GREEN2);
  showmsgXY(210, 45, 1, &Branding_Medium10pt7b, String(ambiente.info.cantBotellas), USI_GREEN2);
  
  tft.fillRect(0, 55, 310, 180, USI_GREEN2);  // Rectangulo para borrar

  // Texto
  showmsgXY(12, 85, 1, &Branding_Bold18pt7b, "CODIGO A CARGAR", USI_NATURAL2);

  tft.drawBitmap(95, 110, rect_borde, 150, 59, USI_NATURAL2);

//  tft.fillRect(103, 110, 150, 95, USI_NATURAL2);
//  tft.fillRect(105, 112, 145, 90, USI_GREEN2);

  tft.setFont(&Branding_Bold18pt7b);
  tft.setCursor(110, 145);
  tft.setTextColor(USI_NATURAL2);
  tft.setTextSize(1);
  tft.print(text);
  delay(100);

  // Mostrar instrucciones
  showmsgXY(5, 190, 1, &Branding_Medium10pt7b, "Desde Ciudadano Digital, ingresa al", USI_NATURAL2);
  showmsgXY(25, 210, 1, &Branding_Medium10pt7b, "sistema USI, carga el codigo", USI_NATURAL2);
  showmsgXY(35, 230, 1, &Branding_Medium10pt7b, "y canjealo por beneficios.", USI_NATURAL2);
  
//  showmsgXY(60, 220, 1, &Branding_Medium10pt7b, "Presione para continuar", USI_NATURAL2);

}

void pantallaResetSIM(){
  
  tft.fillRect(0, 55, 310, 180, USI_GREEN2);  // Rectangulo borrar
    
  showmsgXY(12, 85, 1, &Branding_Bold18pt7b, "Espere por favor", USI_NATURAL2);    // Texto

  showmsgXY(12, 150, 1, &Branding_Medium10pt7b, "Reiniciando modulo GSM/GPRS...", USI_NATURAL2);

  
}

//void pantallaLLeno(){
//
//  tft.fillRect(0, 55, 310, 180, USI_GREEN2);  // Rectangulo borrar
//  showmsgXY(60, 82, 1, &Branding_Bold18pt7b, "USI LLENA", USI_NATURAL2);
//  showmsgXY(5, 100, 1, &Branding_Medium10pt7b, "No ingresar mas botellas", USI_NATURAL2);
//
//}

void readTouchScreen(){
  
  tp = ts.getPoint();   //tp.x, tp.y are ADC values
  
  // if sharing pins, you'll need to fix the directions of the touchscreen pins   REVISAR! poner en setup() ???
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  if (tp.z > MINPRESSURE && tp.z < MAXPRESSURE) {
    xpos = map(tp.y, TS_TOP, TS_BOT, 0, tft.width());
    ypos = map(tp.x, TS_RT, TS_LEFT, 0, tft.height());

    if (ypos > 90 && ypos < 200) {
      if(xpos > 20 && xpos < 300){
        // Detectar presion zona boton inicio. OJO! puede entrar varias veces con tocar una sola vez. Utilizar func simil anti rebote (debounce)
        
        //delay(100);
        //Serial.println("Se presiono la pantalla");
        
        flagTouch = true;
        //return true;
        
      }
    }
    delay(500);
  }
  //return false;
}

//-------------------------- Funciones auxiliares --------------------------

char* codeGenerator(uint16_t nroBotellas){

  String strBuffer, aux;
  char charArrayBuff[10];
  static uint8_t indexCode = 0;    // Variable contador o indice de codigos generados

  strBuffer = UBICACION;

  // PROBAR: Codificar 256-nroBotellas. Por Ej: nroBotellas = 3; 256-3 = 253; en HEX = FD
  
  if (nroBotellas >= 0 && nroBotellas <= 15) {
      aux = String(nroBotellas, HEX);
      aux = "0" + aux;
  }else{
      aux = String (nroBotellas, HEX);
  }

  strBuffer = strBuffer + aux;
  
  if (indexCode >= 0 && indexCode <= 15) {
      aux = String(indexCode, HEX);
      aux = "0" + aux;
  }else{
      aux = String (indexCode, HEX);
  }

  strBuffer = strBuffer + aux;

  Serial.print("Sin codificar: "); Serial.println(strBuffer);

  strBuffer.toCharArray(charArrayBuff, sizeof(charArrayBuff));

  resultCode[2] = charArrayBuff[0];
  resultCode[0] = charArrayBuff[1];
  resultCode[5] = charArrayBuff[2];
  resultCode[3] = charArrayBuff[3];
  resultCode[1] = charArrayBuff[4];
  resultCode[4] = charArrayBuff[5];

  Serial.print("Codificado: ");
  Serial.println(resultCode);

  // Incrementar contador:

  indexCode++;

  if(indexCode >= 255){
    indexCode = 0;
  }

  return resultCode;
  
}

void setUS_values(){

  float aux;
  
  // Calcular pendiente
  m = float(100) / float((XMAX - XMIN));
  m = -1 * m;
  Serial.print("m: "); Serial.println(m);

  // Calcular ordenada
  aux = m * XMIN;
  Serial.print("m * x1: "); Serial.println(aux);
  b = abs(aux) + 100;
  Serial.print("b: "); Serial.println(b);
  
}

int aPorcentaje(int distCm){

  float aux = 0.0;

  Serial.println(distCm);
  
  if(distCm > XMAX){
    distCm = XMAX;
  }

  if(distCm < XMIN){
    distCm = XMIN;
  }

  aux = m*distCm;     // % = m*distCm + b
  aux = aux + b;
  aux = aux + 0.5;   // sumo 0.5 para redondear al entero mas cercano.
  
  Serial.println(aux);
  
  return int(aux);
}

bool sendAttempt(){

  bool result;

  Serial.print("Enviando datos a: "); Serial.println(URLString);    // Enviar datos.
  ambiente.updateDate();                                            // Actualizar fecha y hora.
  Serial.println(ambiente.timeStamp);                               // Mostrar estampa de tiempo actualizada.
  result = ambiente.enviarInfo();                              // Ejecutar envío.
  Serial.print("Estado Envío: "); Serial.println(result);

  return result;
  
}

void readClearEEPROM(){

  uint32_t dataRead = 0;          //Almacenar aca datos leidos.

  // Acá leer total botellas de memoria. Asignar a ambiente.info.cantBotellas (4 bytes).
  Serial.println();
  Serial.print("Tamaño de memoria EEPROM: "); Serial.println(EEPROM.length());
  Serial.print("Leyendo EEPROM en direccion "); Serial.println(eeAddress);
  EEPROM.get(eeAddress, dataRead);    
  Serial.println(dataRead);

  ambiente.info.cantBotellas = dataRead;

  if(digitalRead(eepromPin)==LOW){          // Si eepromPin esta en bajo borra variable guardada (cantidad de botellas).
    ambiente.info.cantBotellas = 0;
    Serial.print("Borrando datos eeprom...");   
    EEPROM.put(eeAddress, ambiente.info.cantBotellas);
    Serial.println("Ok");
    Serial.print("Se borraron: "); Serial.print(sizeof(ambiente.info.cantBotellas)); Serial.println(" Bytes");

    delay(1000);

    // Chequear borrado
    Serial.print("Check data EEPROM: ");
    EEPROM.get(eeAddress, dataRead);
    Serial.println(dataRead);
    Serial.println();
  }
}

//-------------------------- Subrutinas Interrupcion --------------------------
void ISR_IR1(){
  IR1_act = true;
}

void ISR_IR2(){
  if (IR1_act == true){
    IR1_act = false;
    bottleFlag = true;
  }
}

void ISR_Puerta(){
  flagDoor = true;
}

void updateTime(){
  
  timerCount++;
  
  if (timerCount >= TIMEOUT){
    timerCount = 0;
    timerFlag = true;
  }
}

//-------------------------- Funciones setup() y loop() --------------------------

void setup() {
  
  Serial.begin(BAUD_RATE);
  
  init_display();
  
  Serial.println("Configurando...");

  //tft.setRotation(1);   // establecer orientacion de pantalla (0 = vertical, 1 = horizontal)
  tft.setRotation(3);   // establecer orientacion de pantalla (2 = vertical, 3 = horizontal  180°)

  tft.fillScreen(BLACK);
  showmsgXY(0, 0, 2, NULL, "Iniciando Unidad", GREEN);
  showmsgXY(0, 30, 2, NULL, "Configurando...", RED);
  progressBar(0, 60);
  

  // Configurando pines
  pinMode(pinLock, OUTPUT);                // Ctrl cerradura (solenoide).
  digitalWrite(pinLock, HIGH);            // HIGH = Rele en descanso (Cerrado); LOW = Acciona rele (Abierto)
  
  pinMode(pinLedVerde, OUTPUT);           // Ctrl tira leds.
  digitalWrite(pinLedVerde, HIGH);        // HIGH = Rele en descanso; LOW = Acciona rele
  
  pinMode(pinLedRojo, OUTPUT);            // Ctrl tira leds.
  digitalWrite(pinLedRojo, LOW);

  pinMode(statusLed, OUTPUT);         // Led estado
  digitalWrite(statusLed, LOW);

  pinMode(pinDoor, INPUT_PULLUP);     // pin sensor magnetico puerta

  pinMode(eepromPin, INPUT_PULLUP);

  // Ultrasonido
  pinMode(pinUS_trigger, OUTPUT);     //pin como salida
  pinMode(pinUS_echo, INPUT);         //pin como entrada
  digitalWrite(pinUS_trigger, LOW);   //Inicializamos el pin con 0

  setUS_values();                     // Calcular valores conversion a porcentaje UltraSonido

  pinMode(RST, OUTPUT);               // pin RST SIM808
  pinMode(PWR, OUTPUT);               // pin encendido PWR SIM808

  // Habilitar interrupciones
  attachInterrupt(digitalPinToInterrupt(pinIR1), ISR_IR1, RISING);
  //pinMode(pinIR1, INPUT);         // Probando otra logica de deteccion botellas: Si IR1 HIGH y INT RISING cuenta
  attachInterrupt(digitalPinToInterrupt(pinIR2), ISR_IR2, RISING);

  attachInterrupt(digitalPinToInterrupt(pinDoor), ISR_Puerta, FALLING);

  // Iniciar SIM808
  Serial.print("Iniciando SIM808...");
  //simStatus = ambiente.SIM_Initialize();
  //Serial.print("SIM Status: "); Serial.println(simStatus);
  //Serial.print("Power Status: "); Serial.println(sim808.powered());

  digitalWrite(statusLed, HIGH);

  // Setup Timer
  Timer3.initialize(5000000);               // Establecer en ms. Iniciar timer con periodo de 5 seg.
  Timer3.attachInterrupt(updateTime);       // Subrutina interrupcion.
  Timer3.stop();                            // Detener Timer.

  // Memoria EEPROM: Borrar si pin jumper en bajo.
  readClearEEPROM();

  // Lectura nivel llenado
  nivelCm = sonar.convert_cm(sonar.ping_median(10));    //Utilizando libreria NewPing  
  ambiente.info.nivel=aPorcentaje(nivelCm);

  delay(1000);
  
  showmsgXY(0, 90, 2, NULL, "OK", GREEN);

  pantallaPrincipal(ambiente.info.nivel, ambiente.info.cantBotellas, simStatus);

  lastTimeBlink = millis();

  digitalWrite(pinLedVerde, LOW);     // Encender Led verde

  Serial.println("Setup OK");

}

void loop() {

  // Conmutar LED onboard.
  if(millis()-lastTimeBlink >= BLINKTIME){
    digitalWrite(statusLed, !digitalRead(statusLed));
    lastTimeBlink = millis();
  }

//  if(digitalRead(pinDoor)==HIGH){    // Puerta abierta ??
//    Serial.println("Puerta abierta");
//    
//  }

  // consultar variable nivel de llenado
  if(ambiente.info.nivel < LIMITE_LLENADO){
    readTouchScreen();    // Polling touch pantalla
  }else{
    digitalWrite(pinLedRojo, LOW);  // Encender Led rojo
    digitalWrite(pinLedVerde, HIGH);  // Apagar Led verde
  }

  if(flagTouch && !flagShowCode){
    flagTouch = false;
    Serial.println("Ingreso de botellas iniciado");
    
    pantallaFin();    // Mostrar pantalla contador y finalizacion ingreso
    
    // Iniciar Timer.
    Serial.println("Timer Start");
    timerFlag = false;
    timerCount = 0;   // Reset timer
    Timer3.start();
    
    digitalWrite(pinLedRojo, HIGH);  // Apagar Led rojo
    digitalWrite(pinLock, LOW);      // Habilitar ingreso
    
    //  Inicializar banderas subrutinas INT.
    IR1_act = false;
    bottleFlag = false;
    
    contBotellas = 0;
    
    while(true){

      readTouchScreen();        // Polling touch pantalla
      
      if(bottleFlag){             // Se ingreso botella? contar; reset timer; actualizar display
        //bottleFlag = false;
        contBotellas++;
        timerCount = 0;       // Reset timer

        Serial.print("botellas ingresadas: "); Serial.println(contBotellas);
        
        // actualizar valor en pantalla:
        //tft.fillRect(205, 22, 40, 25, USI_NATURAL2);                     // Borrar
        //showmsgXY(210, 45, 1, &Branding_Medium10pt7b, buffCont, USI_GREEN2);     // Actualizar

        digitalWrite(pinLedRojo, LOW);  // Encender Led rojo

        if(contBotellas < 10){
          tft.fillRect(25, 100, 100, 93, USI_NATURAL2);                         // Borrar
          showmsgXY(50, 185, 1, &Branding_Bold52pt7b, String(contBotellas), USI_GREEN2);     // Actualizar
        }else{
          tft.fillRect(25, 100, 115, 93, USI_NATURAL2);                         // Borrar
          showmsgXY(25, 185, 1, &Branding_Bold52pt7b, String(contBotellas), USI_GREEN2);     // Actualizar
        }

        delay(500);
        digitalWrite(pinLedRojo, HIGH);  // Apagar Led rojo
        delay(500);

         // Espera en total 1 seg luego de un ingreso
         
        bottleFlag = false;

      }

      if(timerFlag){                    // Timer > TIMEOUT? break;
        Serial.println("TIMEOUT\n\r Ingreso finalizado");
        
        // Detener Timer.
        Timer3.stop();
        timerFlag = false;
        timerCount = 0;   // Reset timer
        Serial.println("Timer Stop");

        // Actualizar nivel llenado
        nivelCm = sonar.convert_cm(sonar.ping_median(10));  
        ambiente.info.nivel=aPorcentaje(nivelCm);

        // Sumar posibles botellas ingresadas? Guardar en EEPROM ?

        // Actualizar a pantalla principal:
        pantallaPrincipal(ambiente.info.nivel, ambiente.info.cantBotellas, simStatus);

        digitalWrite(pinLedRojo, LOW);   // Encender Led Rojo
        digitalWrite(pinLock, HIGH);      // Bloquear ingreso

        break;
      }

      if(flagTouch){                    // Finalizar ingreso
        flagTouch = false;
        Serial.println("Ingreso finalizado");

        // Detener Timer.
        Timer3.stop();
        Serial.println("Timer Stop");
        timerCount = 0;   // Reset timer

        // Generar codigo beneficio utilizando contBotellas
        //Serial.println("Generando codigo...");
        //codeGenerator(contBotellas);

        // Actualizar nivel llenado
        nivelCm = sonar.convert_cm(sonar.ping_median(10));  
        ambiente.info.nivel=aPorcentaje(nivelCm);

        // Sumar botellas ingresadas a total:
        ambiente.info.cantBotellas = ambiente.info.cantBotellas + contBotellas;

        // Guardar total botellas en EEPROM.
        Serial.print("Iniciando escritura con metodo put()...");
        EEPROM.put(eeAddress, ambiente.info.cantBotellas);
        Serial.println("OK");
        Serial.print("Se escribieron: "); Serial.print(sizeof(ambiente.info.cantBotellas)); Serial.println(" Bytes");

        // Actualizar pantalla: Mostrar codigo si contBotellas >= 1
        if(contBotellas >= 1){
          Serial.println("Generando codigo...");
          pantallaCodigo(codeGenerator(contBotellas));
          flagShowCode = true;

          // Iniciar Timer. Temporizar pantallaCodigo()
          Serial.println("Timer Start");
          timerFlag = false;
          Timer3.start();
        }else{
          pantallaPrincipal(ambiente.info.nivel, ambiente.info.cantBotellas, simStatus);
        }

        digitalWrite(pinLedRojo, LOW);  // Encender Led Rojo
        digitalWrite(pinLock, HIGH);      // Bloquear ingreso

        // Realizar intento de envio GPRS
        //estadoEnvio = sendAttempt();

        // Chequear si intento no fue exitoso habilitar intentos posteriores
        if (estadoEnvio == false){

          Serial.println("Error al enviar datos");

          // Habilitar intentos de envio:
          resendFlag = true;
          lastTime = millis();

        }else{
          Serial.println("Datos enviados:");
          resendFlag = false;
          Serial.println(ambiente.info.nivel);
          Serial.println(ambiente.info.cantBotellas);
        }
        
        break;
      }
    }
    
  }

  // Chequear bandera 'flagShowCode' para cambiar pantalla:
  if(flagShowCode){
    if (timerFlag){
      timerFlag = false;
      flagShowCode = false;
      Timer3.stop();
  
      Serial.println("Timer Stop: Fin pantalla codigo");
      pantallaPrincipal(ambiente.info.nivel, ambiente.info.cantBotellas, simStatus);
    }
  
    if(flagTouch){
      flagTouch = false;
      flagShowCode = false;
      Timer3.stop();      // Detener timer.
      Serial.println("Timer Stop");
      pantallaPrincipal(ambiente.info.nivel, ambiente.info.cantBotellas, simStatus);   // Regresar a pantalla principal.
    }
  }

  if(resendFlag){
    if(millis() - lastTime >= INTERVALCHECK){
      
      Serial.println("Realizando intento de envio");
      //estadoEnvio = sendAttempt();    // Iniciar intento de envio GPRS

      if (estadoEnvio == false){
        Serial.println("Error al enviar datos");
        cantIntentos++;                   // contar intento fallido
        // Habilitar intentos de envio:
        resendFlag = true;

      }else{
        Serial.println("Datos enviados:");
        resendFlag = false;
        cantIntentos = 0;
        Serial.println(ambiente.info.nivel);
        Serial.println(ambiente.info.cantBotellas);
      }

      lastTime = millis();
    }

    if(cantIntentos>= INTENTOS){
      
      digitalWrite(pinLedVerde, HIGH);    // Apaga color verde.
      digitalWrite(pinLedRojo, LOW);      // Enciende color rojo.          
      
      pantallaResetSIM(); // Mostrar msj por pantalla 

      Serial.println("Apagando SIM808...");
      //Serial.print("Disable Gprs: "); Serial.println(sim808.disableGprs());
      //sim808.powerOnOff(false);
      //Serial.print("Power Status: "); Serial.println(sim808.powered());
      
      delay(10000); // Espera 10 seg.
      
      Serial.print("Iniciando SIM808...");
      //simStatus = ambiente.SIM_Initialize();                      // Llama a metodo de inicializacion
      Serial.print("GPRS Status: "); Serial.println(simStatus);

      pantallaPrincipal(ambiente.info.nivel, ambiente.info.cantBotellas, simStatus);   // Regresar a pantalla principal.
  
      digitalWrite(pinLedVerde, LOW);    // Enciende color verde.
      digitalWrite(pinLedRojo, LOW);      // Enciende color rojo. 
      
      cantIntentos = 0;
      
    }
  }

  // Chequear si contenedor esta lleno, cambia la pantalla:
  if(flagDoor == true){
    flagDoor = false;

    // Actualizar nivel llenado
    nivelCm = sonar.convert_cm(sonar.ping_median(10));  
    ambiente.info.nivel=aPorcentaje(nivelCm);

    pantallaPrincipal(ambiente.info.nivel, ambiente.info.cantBotellas, simStatus);   // Actualizar pantalla principal.

    digitalWrite(pinLedRojo, LOW);  // Encender Led rojo
    digitalWrite(pinLedVerde, LOW);  // Encender Led verde
    
  }

  
}
