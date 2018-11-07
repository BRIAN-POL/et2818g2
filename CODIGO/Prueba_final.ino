// Runner Heart 

/*
Este es un proyecto realizado con el fin de llegar a un producto comerciable apuntado a un público 
deportivo, los deportistas podrán controlar y disfrutar de su actividad fisica 
tomando en cuenta su esfuerzo y salud de su corazón
*/

// Pines del Display que van conectados al Arduino 



//      SCK  (7) - Pin 13
//      MOSI (6) - Pin 12
//      DC   (5) - Pin 11
//      RST  (4) - Pin 10
//      SCE  (3) - Pin 9



#include "LCD5110_Graph.h"                                                // Incluyo libreria del LCD 5110
#include <TimerOne.h>                                                     // Incluyo Libreria del manejo del manejo del timer
#include "RTClib.h"                                                       // Incluyo libreria para manejar el RTC
#include <Wire.h> 


LCD5110 Display(13, 12, 11, 10, 9);                                       // Indico en pines del Arduino se conecta el Display

extern uint8_t SmallFont[];                                               // Indicamos el tipo de fuente SmallFont
extern uint8_t TinyFont [];                                               // Indicamos el tipo de fuente TinyFont
extern uint8_t MediumNumbers [];                                          // Indicamos el tipo de fuente MediumNumbers
extern uint8_t RunnerHeart [];                                            // Indicamos el logo RunnerHeart

RTC_DS1307 RTC;                                                           // Defino el RTC_DS1307 con el nombre de RTC

// Variable del tipo char Para los días de la semana
char daysOfTheWeek[7][12] = {"DOM", "LUN", "MAR", "MIE", "JUE", "VIE", "SAB"};  


// Estas variables son volatiles porque son usadas durante la rutina de interrupcion en la segunda Pestaña

int pulsePin = 0;                                                         // Sensor de Pulso conectado al puerto A0
volatile int BPM ;                                                        // Pulsaciones por minuto
volatile int Signal;                                                      // Entrada de datos del sensor de pulsos
volatile int IBI = 600;                                                   // Tiempo entre pulsaciones
volatile boolean Pulse = false;                                           // Verdadero cuando la onda de pulsos es alta, falso cuando es Baja
volatile boolean QS = false;                                              // Verdadero cuando el uC Busca un pulso del Corazon
volatile int rate[10];                                                    // Matriz para mantener los últimos diez valores IBI
volatile unsigned long sampleCounter = 0;                                 // Utilizado para determinar el tiempo de pulso
volatile unsigned long lastBeatTime = 0;                                  // Se utiliza para encontrar IBI
volatile int P =512;                                                      // Se utiliza para encontrar el pico en la onda del pulso
volatile int T = 512;                                                     // Se usa para encontrar canal en la onda del pulso
volatile int thresh = 512;                                                // Se utiliza para encontrar momento instantáneo de latido del corazón
volatile int amp = 100;                                                   // Se utiliza para mantener la amplitud de la forma de onda del pulso
volatile boolean firstBeat = true;                                        // Se utiliza para sembrar la matriz de velocidad, así que iniciamos con BPM razonables
volatile boolean secondBeat = false;                                      // Se utiliza para sembrar la matriz de velocidad, así que iniciamos con BPM razonables
int pulso;                                                                // Variable que guarda lo leido por el canal analogico A0


// Variables constantes referidas a los botones de manejo

const byte pinEnter= 2;                                                   // Cronometro = StartStop-----Temporizador = Horas
const byte pinUp= 3;                                                      // Cronometro = TParcial-----Temporizador = Minutos
const byte pinDown= 4;                                                    // Cronometro = Reset-----Temporizador =  Segundos
const byte pinEmpieza= 5;                                                 // Temporizador = Arranque
const byte VIBRADOR= 6;                                                   // Temporizador = Arranque

// Variables Referidas al Cronometro y temporizador
volatile uint8_t DecimaC = 0;
volatile uint8_t Segundos = 0;
volatile uint8_t Minutos = 0;
volatile uint8_t Horas = 0;
uint8_t DecimaC2 = 0;
uint8_t Segundos2 = 0;
uint8_t Minutos2 = 0;

// Estas son algunas varibles necesarias para guardar estados de un mismo boton y salir de la interrupción

bool  flagStart = false;                                                  //Variable utilizada para guardar estados del boton Start
bool  salir = false;                                                      // Variable importante utilizada para cabiar de pantallas en el Display
volatile bool END_TEMP = false;                                           // Variable utilizada para salir de la atención a la interrupción proveniente del Temporizador
volatile bool VIBRA_ON = false;                                           // Variable utilizada para una de las alertas


int INDICE = 1;                                                           // Variable que sirve como marcador en la opción que se eliga de las diferentes pantallas
int MENU;                                                                 // Variable que almacena las opciones elegidas en la pantalla de Menu
int alerta = 0;
int ARRITMIA;                                                             // Variable que guarda si uno tiene o no Arritmia
int EDAD;                                                                 // Varible que almacena el valor de edad elegido
int retardo = 3000;                                                       // Variable que se utiliza para generar el delay de los dibujos en la presentación
float INT_FISICA;                                                         // Variable que almacena el valor de intensidad fisica elegido
float INT_FISICA_MIN = 0.5;                                               // Intensidad fisica sugerida para persona con arritmia
float FCM;                                                                // Variable que almacenara todos los valores elegidos, los cuales se utilizarán en una operación para la FCM  
                                                                          // en una operación para la FCM de cada persona

// Variables para la bateria 
float VIN = 0;                                                            // Variable que almacena el valor del canal analogico
float VBAT = 0;                                                           // Variable utizada para hallar la tensión de la bateria
int VPOR = 0;                                                             // Variable que establece el porcentaje de la bateria

#define NO 0
#define SI 1


void setup() {

// Configuración inicial de los pusadores como entradas 
  pinMode(pinEnter, INPUT_PULLUP); 
  pinMode(pinUp, INPUT_PULLUP);
  pinMode(pinDown, INPUT_PULLUP);
  pinMode(pinEmpieza, INPUT_PULLUP);
  
// Motor Vibrador como salida
  pinMode(VIBRADOR, OUTPUT);
  
  Wire.begin();                                                           // Inicia el puerto I2C
  RTC.begin();                                                            // Inicia la comunicaci¢n con el RTC
  //RTC.adjust(DateTime(2018, 10, 23, 17, 22, 0)); // Establece la fecha y hora (Comentar una vez establecida la hora)
  //el anterior se usa solo en la configuracion inicial

  Serial.begin(9600);                                                     // Puerto serial configurado a 9600 Baudios
  Display.InitLCD();                                                      // Se define el valor del contraste en este caso lo dejaremos el que viene por DEFAULT
  Display.clrScr();                                                       // Limpiamos la pantalla 
  
  interruptSetup();                                                       // Configuro la interrucion para leer el sensor de pulsos cada 2mS  
  Timer1.initialize(10000);                                               // Configuro un temporizador de 10000 microsegundos de duración 
  Timer1.attachInterrupt( timerIsr );                                     // Atención al servicio de Interrupción
  Timer1.stop();                                                          // Se detiene el Timer1 para poder activarlo en cada función relacionada con el tiempo
}

void loop() {

  
  pulso = analogRead(0);                                                  //Lee el valor del pulsometro conectado al puerto Analogo A0
  Display.setFont(SmallFont);                                             // Decimos el tipo de fuente "SmallFont"
                                                                          // Escribimos un texto que sera colocado en el Centro en X y en Y su posicion sera 20

// == Pantallas de presentación ====================================================================================================================================
  Display.print("RUNNER HEART",CENTER,20); 
  Display.update();                                                       // Refresca la pantalla para que se muestren los datos introducidos
  delay(retardo);                                                         // Retardo general
  Display.update();
  Display.clrScr();                                                       // Limpiamos la pantalla 
  Display.drawBitmap(0, 0, RunnerHeart, 80, 45);                          // Hacemos el llamado a nuestra imagen 
  Display.update();
  delay(retardo);                                                         // Retardo general

  
// == Pantalla Menu ================================================================================================================================================
menu:;                                                                    // Punto donde retornan todos los goto dirigidos a aqui
  while (!salir){                                                         // Mientras salir sea falso hará lo siguiente 
    drawMenu();                                                           // Dibuja en pantalla todo el menu con las distintas opciones
                                                                          // A partir de aqui se hace el chequo de los pulsadores 
    if(digitalRead(pinEnter)==0){ 
          while(!digitalRead(pinEnter)){}                                 // si el boton de enter se presiona salir se pone en verdadero y se sale del while
                                                                          // esto se hará en cada opción que se presione que se de cambio de pantalla
          
          salir = true;
          
          }
         if(digitalRead(pinUp)==0){ 
          while(!digitalRead(pinUp)){}
          INDICE ++; //Incremento indice una vez
          if(INDICE == 5){                                                // Si indice es igual a 5, osea que bajo hasta la ultima opción y se siguio presionando
            INDICE = 1;                                                   // Indice se coloca en la primera opción 
            }
          }
          if(digitalRead(pinDown)==0){
            while(!digitalRead(pinDown)){}
          INDICE --;                                                      // Decremento indice una vez
          if(INDICE == 0){                                                // Si indice es igual a 0, osea que se subio hasta la primera opción y se siguio presionando
            INDICE = 4;                                                   // Indice se coloca en la última opción 
            }
          }
          
        }
        salir = false;                                                    // salir se coloca en falso para poder cambiar de pantallas y asi usarla en todas
        MENU = INDICE;                                                    // El valor elgido por indice se carga en la varible Menu
        INDICE = 0;                                                       // Indice se lo resetea para poder usarlo en las demás pantallas
// ===== Reloj ==========================================================================================================================================================      
        do {
          drawReloj();
          if(BPM > 100){                                                  // Si los pulsos reales superan las 100 BPM
                      
           
           alerta = 3;                                                    // Alerta se coloca en 3 y se detecta taquicardia
           VIBRA_ON = true;                                               // Y la varible correspondiente a la alerta vibratoria se coloca en 1
                     
          }
          
                                                                         // Detalle: La alerta permanecerá encendida hasta que el usuario presione el pulsador
          if(digitalRead(pinEmpieza)==0){                                // Si se presiona el pin de empieza
            while(!digitalRead(pinEmpieza)){}
            digitalWrite(VIBRADOR, LOW);                                 // La alerta se apaga
            INDICE = 1;                                                  // Indice se pone en 1  para que marque la primera opción del menu
            VIBRA_ON = false;                                            // Esta bandera se pone en 0
            alerta =  0;                                                 // Alerta en 0 para poder reutilizarla
            goto menu;                                                   // Se sale de la pantalla del reloj y sse vuelve a Menu
          }
          
          }while(MENU == 1);                                             // Hacer todo lo mencionado siempre que la opción elegida sea la 
                                                                         // del reloj
// ===== Pantalla Modo Deporte ==================================================================================================================
        if (MENU == 2){                                                  // Si la opción elegida fue el modo deporte se realiza lo siguiente
// ----- Subpantalla Arritmia -------------------------------------------------------------------------------------------------------------------
          while (!salir){
            drawArritmia();                                              // Función grafica correspondiente a arritmia
            if(digitalRead (pinEnter)== 0){
              while(!digitalRead(pinEnter)){}
              salir = true;
              }
            if(digitalRead (pinUp)== 0){
              while(!digitalRead(pinUp)){}
              INDICE++;
              if (INDICE==2)                                            // si indice llego a dos
              {INDICE=0;}                                               // Indice se coloca en  la primera opción 
              }
            if(digitalRead (pinDown)== 0){
              while(!digitalRead(pinDown)){}
              INDICE--;
              if (INDICE<0)                                             //si indice llega ser menor que 0 
              {INDICE=1;}                                               // Indice se coloca en la ultima opción
                }
              }

                                                                       // Aqui cualquiera sea el valor elegido se guarda en la variable Arritmia
              if (INDICE == 1 ){ 
                ARRITMIA = INDICE;
                }
              else{
                ARRITMIA = INDICE;
                
                }
                salir=false;
                
// ----- Subpantalla Edad --------------------------------------------------------------------------------------------------------------
              INDICE = 15;                                            //El valor inicial de edad comienza desde los 15 años
              while(!salir){
                drawEdad();                                           // Función grafica de edad
                if (digitalRead(pinEnter) == 0){
                  while(!digitalRead(pinEnter)){}
                  salir = true;
                  }
                  if(digitalRead (pinUp)== 0){
                    while(!digitalRead(pinUp)){}
                    if (INDICE < 60)
                    {INDICE++;}                                       // Cada vez que se presiona se incrementa en uno edad hasta el 
                                                                      // valor de los 60 años    
                      }
                  if(digitalRead (pinDown)== 0){
                    while(!digitalRead(pinDown)){}
                    if (INDICE > 15)
                    {INDICE--;}                                       // Cada vez que se presiona se decrementa en uno edad hasta el 
                                                                      //valor de los 15 años         
                    }
                 }
                 salir = false;
                 EDAD = INDICE;                                      // El valor de indice se guarda en edad
                 INDICE = 1;                                         // Indice se coloca en 1

// Aqui surgen dos ramas a partir de lo que haya elegido 
// el usuario en la pantalla de arritmia sea cual haya sido la edad elegida
                 switch(ARRITMIA){
                  
                  case SI:                                           // Si en la variable arritmia el caso fue que SI
                  
                  FCM = (208.75-(0.73*EDAD))*INT_FISICA_MIN;         // Se calcula la FCM con la intensidad fisica minima para un arritmico
// -------- Subpantalla de Muestreo de datos para un arritmico -------------------------------------------------------------------------------------
                  while(!salir){
                    drawMuestreoDeDatos();                           // Función gráfica de muestreo de los datos 
                    if(digitalRead(pinEnter) == 0){
                      while(!digitalRead(pinEnter)){}
                      salir = true;
                     }
                   }
                   salir = false;
                   
                   break;
                   
                   case NO:                                         // En el caso de que haya sido que NO la opción elegida 
                   
// -------- Subpantalla de Intensidad fisica -----------------------------------------------------------------------------------------   
               
                   while(!salir){
                    drawIntensidadFisica();                         // Función gráfica de Intensidad fisica
                                                                    // Aqui se hace lo mismo que las anteriores pantallas se elige la 
                                                                    // opción de intensidad fisica a usar
                    if(digitalRead(pinEnter)==0){
                      while(!digitalRead(pinEnter)){}
                      salir = true;
                      }
                    if(digitalRead(pinUp)==0){
                      while(!digitalRead(pinUp)){}
                      INDICE ++;
                      if(INDICE == 6)
                      {INDICE = 1;}
                      }
                    if(digitalRead(pinDown)==0){
                      while(!digitalRead(pinDown)){}
                      INDICE --;
                      if(INDICE == 0)
                      {INDICE = 5;}
                     }
                   }
                   salir = false;
                   
                   break;
                }
                FCM = (208.75-(0.73*EDAD))*INT_FISICA;              // Con los datos elegidos se realiza el 
                                                                    //calculo de la FCM y se pasa la pantalla de Muestro de datos
                
// -------- Subpantalla de Muestreo de datos para un no arritmico ------------------------------------------------------------------------
                while(!salir){
                  
                  drawMuestreoDeDatos();
                 if(BPM > FCM){                                     // Si las pulsaciones reales son mayores a las fijadas                     
                      digitalWrite(VIBRADOR, HIGH);                 // Se activa alerta vibratoria
                      alerta = 1;                                   // Alerta correspondiente a que se supero la FCM elegida por el usuario                   
                      }
                      
                  if(BPM < 60){                                     // Si las pulsaciones reales son menores a 60  
                      digitalWrite(VIBRADOR, HIGH);                 // Se activa alerta vibratoria
                      alerta = 2;                                   //Alerta correspondiente a bradicardia
                      }
                  
                  if(digitalRead(pinEnter) == 0){
                    while(!digitalRead(pinEnter)){}
                    salir = true;
                    
                    } 
                  
                    if(digitalRead(pinEmpieza)==0){                  //Si fue presionado el boton de empieza
                    while(!digitalRead(pinEmpieza)){}

                    digitalWrite(VIBRADOR,LOW);                      // se apaga la alerta y se reestablecen las variables utilizadas
                    alerta = 0;
                    INDICE = 1;
                    goto menu;                                      // Se vuelve al menu principal
                    
                    }
                    
                  }
                  salir = false;
                  
       }
       Display.clrScr();                                            // Se limpia el Display para pasar a una nueva pantalla
       
// ====== Cronometro ===================================================================================================================
       do{
        drawCronometro();                                           // Función gráfica para el cronometro
                
          if (!digitalRead(pinEnter))
         {
          while(!digitalRead(pinEnter)){}
                                                                    // Dependiendo el valor del flag guardo cuando se pulso
          if (flagStart)
          {
            Timer1.stop();                                          // El Timer 1 se detiene, se detiene el cronometro
            flagStart = false;
            }
          else 
          {
            Timer1.start();                                         // El Timer 1 se recontinua, se retoma valor del cronometro
            flagStart= true;
            }
           }
          
          if(!digitalRead(pinUp))
          {
            while(!digitalRead(pinUp)){}                            // Si se presiono el boton de parcial
            
            DecimaC2 = DecimaC;                                     // Se capturan los valores de DecimaC se pasan a una variable DecimaC2
            Segundos2 = Segundos;                                   // Idem
            Minutos2 = Minutos;                                     // Idem
            drawParcial();                                          //Función grafica de parcial
            
            }
            
            if(!digitalRead(pinDown))
            {
            while(!digitalRead(pinDown)){}                          // Si se presiono el boton de reset se resetean todas las variables a 0
            
            DecimaC2 = 0;
            Segundos2 = 0;
            Minutos2 = 0;
            drawReset();
            DecimaC = 0;
            Segundos = 0;
            Minutos = 0;
            Timer1.stop();                                          // El Timer 1 se frena
            
            if(!digitalRead(pinEnter))                              // Si se presiono el boton de Start/Stop 
            {
              while(!digitalRead(pinEnter)){}
              Timer1.start();                                       // El Timer1 comienza de nuevo a contar
              }
             }
            if(digitalRead(pinEmpieza)==0){                         //Si se presiona el boton de empieza
              while(!digitalRead(pinEmpieza)){}
              INDICE = 1;                                           // Indice se coloca en su valor inicial
              Timer1.stop();                                       // El Timer1 se frena
              DecimaC = 0;                                         // Y todas las variables se llevan a 0
              Segundos = 0;
              Minutos = 0;
              Horas = 0;
              goto menu;                                           //Se sale del Cronometro y se vuelve al menu principal
              }          
            }while(MENU == 3);                                     // Siempre se hará esto mientras la opción elegida sea la del cronometro
            Display.clrScr();                                      //Se reliza una limpieza de pantalla
            
// ===== Pantalla del Temporizador ====================================================================================            
       do{
          
          drawTemp();                                              // Función correspondiente al temporizador 
          
          if(END_TEMP){                                            // Si el Timer1 llego a 0, EN_TEMP se puso en 1 y se hace o siguiente
            Display.clrScr();                                      // Limpieza de pantalla 
            Timer1.stop();                                         // Se para el Timer1
            DecimaC = 0;                                           // Todas las variables de tiempo se resetean 
            Segundos = 0;
            Minutos = 0;
            Horas = 0;
            digitalWrite(VIBRADOR,HIGH);                           // Se activa el Vibrador
            Display.setFont(TinyFont);
            Display.print("TIEMPO TERMINADO!", CENTER, 40);        // Se imprime en pantalla que l tiempo se termino
            Display.invertText(true);
 
            }
                             
          END_TEMP = false;                                        // Se coloca END_TEMP se coloca en 0 para poder reutilizarla nuevamente 
          if (!digitalRead(pinEmpieza)){ 
            while(!digitalRead(pinEmpieza)){}                      // Si se presiono el boton de empieza
            if (flagStart){                                        // Si el flag es verdaero
              flagStart= false;                                    // Se coloca en falso
              digitalWrite(VIBRADOR,LOW);                          // Se apaga el vibrador
              Timer1.stop();                                       // Se para el Timer 1
              DecimaC = 0;                                         // Se resetean todas las variables de tiempo
              Segundos = 0;
              Minutos = 0;
              Horas = 0;
              goto menu;                                           // Se sale de la pantalla del temporizador y se regresa al menu
              }
            else{                                                  // El temporizador comienza a descontar 
              Timer1.start();                                      // El  Timer1 comienza
              INDICE = 1;                                         // Indice se reestablece a su valor inicial 
              flagStart= true;                                    // flagStart se pone en verdadero
              }
            }
        
          if(!digitalRead(pinEnter))                              //Si el boton de horas ha sido pulsado
          {
            while(!digitalRead(pinEnter)){}
            if(Horas == 99){                                      // Si horas es igual a 99 
              Horas = 0;                                          // Horas se coloca en 0
            }
            else{
              Horas++;                                            //aumentamos las horas en una unidad
            }
           }  

           if(!digitalRead(pinUp))                                //Si el boton de minutos ha sido pulsado
           {
            while(!digitalRead(pinUp)){}
            if(Minutos == 59){                                    //Si Minutos es igual a 59
              Minutos = 0;                                        // Minutos se coloca en 0
              }
            else{
              Minutos++;                                          //aumentamos los minutos en una unidad
            }
           }  

           if(!digitalRead(pinDown))                              //Si el boton de segundos ha sido pulsado
           {
            while(!digitalRead(pinDown)){}
            if(Segundos == 59){                                   //Si segundos es 59
              Segundos = 0;                                       // Segundos se coloca en 0
            }
            else{
              Segundos++;                                         //aumentamos los segundos en una unidad
            } 
           }
           
    }while(MENU == 4);                                            // Se hará esto mientras la opción elegida haya sido Temporizador
}
 


//== Funciones graficas =========================================================================================================================

void drawMenu()                                                   // Pantalla de Menu
  {
   DateTime now = RTC.now();                                      // Obtiene la fecha y hora del RTC
   VIN = analogRead(2);                                           // Leo el canal analogico 2 y dicho valor se lo paso a la variable VIN
   VBAT = (VIN*5)/1023;                                           // Convierto el valor a tensión
   VPOR = (VBAT*100)/4.2;                                         // Teniendo la tensión con esta cuenta calculo el porcentaje de la bateria
   
   Display.clrScr();                                              // Limpieza de Pantalla
   Display.setFont(TinyFont); 
   Display.printNumI(now.hour(), 62, 0, +2, '0');                 // Imprimo hora real
   Display.print(":", 71, 0);
   Display.printNumI(now.minute(), 76, 0, +2, '0');               // Imprimo minutos reales
   
   Display.setFont(SmallFont); 
   Display.print("Menu", 0, 0);                                   // Imprimo Menu

   drawPower();                                                   // Función grafica del dibujo de la bateria
   
   Display.setFont(TinyFont);
   if (INDICE == 1)                                               // Si Indice se encuentra en posición 1
    {Display.invertText(true);                                    // Invierto el texto
     }
   else // De lo contrario
    {Display.invertText(false);                                   // No invierto el texto
     }    
   Display.print("MODO RELOJ", CENTER, 10);                       // Imprimo Modo Reloj
   
   if (INDICE == 2)                                               // Si indice se encuentra en la posición 2
    {Display.invertText(true);                                    // Invierto el texto
     }
   else                                                           // De lo contrario
    {Display.invertText(false);                                   // No invierto el texto
     }    
   Display.print("MODO DEPORTE",CENTER, 20);                      // Imprimo en pantalla MODO DEPORTE
   
   if (INDICE == 3)                                               // Si indice esta en la posición 3 
    {Display.invertText(true);                                    // Invierto el texto
     }
   else                                                           // De lo contrario
    {Display.invertText(false);                                   // No invierto el texto
     }    
     Display.print("CRONOMETRO", CENTER, 30);                     // Imprimo CRONOMETRO
     
   
   if (INDICE == 4)                                               // Si indice esta en la posición 4
    {Display.invertText(true);                                    // Invierto el texto
     }
   else                                                           // De lo contrario
    {Display.invertText(false);                                   // No invierto el texto
     }    
   Display.print("TEMPORIZADOR", CENTER, 40);                     // Imprimo en pantalla TEMPORIZADOR
   Display.update();                                              // Refresco del Display
    
   }
void drawArritmia()                                               // Pantalla de arritmia
  {DateTime now = RTC.now();                                      // Obtiene la fecha y hora del RTC
   VIN = analogRead(2);                                           // Leo el canal analogico 2 y dicho valor se lo paso a la variable VIN
   VBAT = (VIN*5)/1023;                                           // Convierto el valor a tensión
   VPOR = (VBAT*100)/4.2;                                         // Teniendo la tensión con esta cuenta calculo el porcentaje de la bateria
   
   Display.clrScr();
   Display.setFont(TinyFont);
   Display.printNumI(now.hour(), 62, 0, +2, '0');
   Display.print(":", 71, 0);
   Display.printNumI(now.minute(), 76, 0, +2, '0');
   Display.print("Tiene Arritmia?", CENTER, 7);                   // Imprimo en pantalla
   
   drawPower();                                                   // Función grafica de la bateria
   Display.setFont(SmallFont);
     if (INDICE==1)                                               // Si indice esta en la posición 1
     {Display.invertText(true);
     }
     else 
       {Display.invertText(false);
       }
  
   
   Display.print("SI", CENTER, 15);                               // Imprimo en pantalla
   if (INDICE==0)                                                 // Si indice esta en la posición 2
     {Display.invertText(true);
     }
   else 
     {Display.invertText(false);
      }    
   
   Display.print("NO", CENTER, 25);                               //Imprimo en pantalla
   Display.update();
  }
  
void drawEdad ()// Pantalla de edad

  {DateTime now = RTC.now();                                      // Obtiene la fecha y hora del RTC
   VIN = analogRead(2);                                           // Leo el canal analogico 2 y dicho valor se lo paso a la variable VIN
   VBAT = (VIN*5)/1023;                                           // Convierto el valor a tensión
   VPOR = (VBAT*100)/4.2;                                         // Teniendo la tensión con esta cuenta calculo el porcentaje de la bateria
   Display.clrScr();
   drawPower();                                                   // Función grafica de la bateria
   Display.setFont(TinyFont);
   Display.printNumI(now.hour(), 62, 0, +2, '0');
   Display.print(":", 71, 0);
   Display.printNumI(now.minute(), 76, 0, +2, '0');
   Display.setFont(SmallFont);
   Display.print("EDAD", 0, 0);                                   // Imprimo EDAD
   Display.setFont(SmallFont);
   Display.printNumI(INDICE, CENTER, 25, +2);                     // Imprimo la variable indice que corrsponde en este caso a la edad
   Display.update();
  }

void  drawIntensidadFisica(){                                     // Pantalla de intensidad fisica
   DateTime now = RTC.now();                                      // Obtiene la fecha y hora del RTC
   VIN = analogRead(2);                                           // Leo el canal analogico 2 y dicho valor se lo paso a la variable VIN
   VBAT = (VIN*5)/1023;                                           // Convierto el valor a tensión
   VPOR = (VBAT*100)/4.2;                                         // Teniendo la tensión con esta cuenta calculo el porcentaje de la bateria
   Display.clrScr();
   drawPower();                                                   // Función grafica de la bateria
   Display.setFont(TinyFont);
   Display.printNumI(now.hour(), 62, 0, +2, '0');
   Display.print(":", 71, 0);
   Display.printNumI(now.minute(), 76, 0, +2, '0');
   
   Display.print("INT FIS", 0, 0);                                // Imprimo en pantalla
   
   
     if (INDICE == 1){                                            // Si indice esta en la posición 1
     INT_FISICA = 0.6;                                            // Intensidad fisica elegida esta al 60%
     Display.invertText(true);
     }
     else 
       {Display.invertText(false);
       }
     
   Display.print("MUY LIGERA", CENTER, 8);
    
   if (INDICE == 2) {                                             // Si indice esta en la posición 2
      INT_FISICA = 0.7;                                           // Intensidad fisica elegida esta al 70%
      Display.invertText(true);
     }
   else 
     {Display.invertText(false);
      }    
   
     Display.print("LIGERA", CENTER, 16);

   if (INDICE == 3){                                              // Si indice esta en la posición 3 
      INT_FISICA = 0.8;                                           // Intensidad fisica elegida esta al 80%
      Display.invertText(true);
     }
   else 
      {Display.invertText(false);
       }    
   
   Display.print("MODERADA", CENTER, 24);

   if (INDICE == 4) {                                             // Si indice esta en la posición 4
      INT_FISICA = 0.9;                                           // Intensidad fisica elegida esta al 90%
      Display.invertText(true);
     }
   else 
     {Display.invertText(false);
      }    
   
   Display.print("INTENSA", CENTER, 32);

   if (INDICE == 5){                                              // Si indice esta en la posición 5
     INT_FISICA = 1;                                              // Intensidad fisica elegida esta al 100%
     Display.invertText(true);
     }
   else 
     {Display.invertText(false);
      }    
   
   Display.print("MAXIMA", CENTER, 40);
   Display.update();
}

void drawMuestreoDeDatos(){                                       // Pantalla de Muestreo de datos
  DateTime now = RTC.now();                                       // Obtiene la fecha y hora del RTC
  VIN = analogRead(2);                                            // Leo el canal analogico 2 y dicho valor se lo paso a la variable VIN
  VBAT = (VIN*5)/1023;                                            // Convierto el valor a tensión
  VPOR = (VBAT*100)/4.2;                                          // Teniendo la tensión con esta cuenta calculo el porcentaje de la bateria
  Display.clrScr();
  drawPower();                                                    // Función grafica de la bateria
  Display.setFont(TinyFont);
  Display.printNumI(now.hour(), 62, 0, +2, '0');
  Display.print(":", 71, 0);
  Display.printNumI(now.minute(), 76, 0, +2, '0');
  Display.print("DATOS MOSTRADOS",CENTER, 7);
  Display.setFont(TinyFont);
  
  Display.print("FCM = ", 0, 30);
  Display.print("FCR = ", 0, 20);

  Display.printNumF(FCM, 3, 35, 30, '.', +3);                     // Imprimo el resultado calculado de FCM en pantalla
  Display.printNumI(BPM, 35, 20, +3);                             // Imprimo en pantalla el valor de las pulsaciones reales leidas por el sensor

  if(alerta == 1){                                                // Si alerta fue puesta en 1 
    
    Display.print("FCM SUPERADA", CENTER, 40);
    Display.invertText(true);
    
    }
  if(alerta == 2){                                                // Si alerta fue puesta en 2  
    
    Display.print("  BRADICARDIA  ", CENTER, 40);
    Display.invertText(true);
    }
  Display.update();
  
   //Serial.print("BPM = ");  
   //Serial.println(BPM);                                         // Habilitar estas linea para ver BPM en el monitor serial pero deshabilitar la siguiente
   Serial.println(pulso);                                         // envia el valor del pulso por el puerto serie  (desabilitarla si habilita la anterior linea)
  if (QS == true){                                                // Bandera del Quantified Self es verdadera cuando el Arduino busca un pulso del corazon
    QS = false;                                                   // Reset a la bandera del Quantified Self 
  }
}

void drawReloj(){                                                 // Pantalla del reloj

  DateTime now = RTC.now();                                       // Obtiene la fecha y hora del RTC
  VIN = analogRead(2);                                            // Leo el canal analogico 2 y dicho valor se lo paso a la variable VIN
  VBAT = (VIN*5)/1023;                                            // Convierto el valor a tensión
  VPOR = (VBAT*100)/4.2;                                          // Teniendo la tensión con esta cuenta calculo el porcentaje de la bateria
  Display.clrScr();
  drawPower();
  Display.setFont(MediumNumbers);
  Display.printNumI(now.hour(), 10, 10, +2, '0');
  Display.printNumI(now.minute(), 50, 10, +2, '0');
  Display.setFont(SmallFont);
  Display.print(".", 39, 9);
  Display.print(".", 39, 17);
  Display.setFont(TinyFont);
  Display.print(daysOfTheWeek[now.dayOfTheWeek()], 5, 30);        // Imprime en pantalla el nombre del día actual
  Display.print(".,", 18, 30);
  Display.printNumI(now.year(), 67, 30, +2, '0');                 // Imprime en pantalla el año
  Display.print("/", 62, 30);
  Display.printNumI(now.month(), 49, 30, +2, '0');                // Imprime en pantalla el mes
  Display.print("/", 43, 30);
  Display.printNumI(now.day(), 30, 30, +2,'0');                   // Imprime en pantalla el año
  delay(1000);                                                    // La información se actualiza cada 1 seg.

  if((alerta ==  3) && (VIBRA_ON == true)){

    digitalWrite(VIBRADOR, HIGH);

    Display.print("TAQUICARDIA", CENTER, 40);
    Display.invertText(true);
    VIBRA_ON = false;
    }
  Display.update();
  
  
  }

void drawCronometro(){                                            // Pantalla del Cronometro
   DateTime now = RTC.now();                                      // Obtiene la fecha y hora del RTC
   VIN = analogRead(2);                                           // Leo el canal analogico 2 y dicho valor se lo paso a la variable VIN
   VBAT = (VIN*5)/1023;                                           // Convierto el valor a tensión
   VPOR = (VBAT*100)/4.2;                                         // Teniendo la tensión con esta cuenta calculo el porcentaje de la bateria
   Display.setFont(TinyFont);
   drawPower();
   Display.printNumI(now.hour(), 62, 0, +2, '0');
   Display.print(":", 71, 0);
   Display.printNumI(now.minute(), 76, 0, +2, '0');
   
   Display.print("CRONOMETRO",CENTER, 7);
   Display.print("DC", 59, 17);
   Display.print("SEG", 33, 17);
   Display.print("MIN", 10, 17);
   Display.setFont(SmallFont);
   Display.print(":", 23, 25);
   Display.print(".", 46, 25 );
   Display.print(":", 23, 40);
   Display.print(".", 46, 40);
   Display.printNumI(DecimaC, 55, 25, +2, '0');                   // Imprimo en pantalla las variables de tiempo 
   Display.printNumI(Segundos, 31, 25, +2, '0');
   Display.printNumI(Minutos, 7, 25, +2,'0');
   Display.update();    
 }

void drawParcial(){                                               // Impresión de parciales tomados
  
  Display.printNumI(DecimaC2, 55, 40, +2, '0');
  Display.printNumI(Segundos2, 31, 40, +2, '0');
  Display.printNumI(Minutos2, 7, 40, +2,'0'); 
  Display.update();  
  }

void drawReset(){                                                 // Impresión del reset con las variables colocadas en 0
  Display.printNumI(DecimaC2, 55, 40, +2, '0');
  Display.printNumI(Segundos2, 31, 40, +2, '0');
  Display.printNumI(Minutos2, 7, 40, +2, '0');
  }
  
void drawTemp(){                                                  // Pantalla del temporizador

   DateTime now = RTC.now();                                      // Obtiene la fecha y hora del RTC
   VIN = analogRead(2);                                           // Leo el canal analogico 2 y dicho valor se lo paso a la variable VIN
   VBAT = (VIN*5)/1023;                                           // Convierto el valor a tensión
   VPOR = (VBAT*100)/4.2;                                         // Teniendo la tensión con esta cuenta calculo el porcentaje de la bateria
   drawPower();
   Display.setFont(TinyFont);
   Display.printNumI(now.hour(), 62, 0, +2, '0');
   Display.print(":", 71, 0);
   Display.printNumI(now.minute(), 76, 0, +2, '0');
   Display.print("TEMPORIZADOR", CENTER, 7);
   Display.setFont(TinyFont);
   Display.print("SEG", 59, 17);
   Display.print("MIN", 33, 17);
   Display.print("HS", 10, 17);
   Display.setFont(SmallFont);
   Display.print(":", 23, 25);
   Display.print(".", 46, 25 );
   Display.printNumI(Segundos, 55, 25, +2, '0');                  // Imprime el valor de las variables de tiempo que van decrementandose cuando el Timer1 descuenta
   Display.printNumI(Minutos, 31, 25, +2, '0');
   Display.printNumI(Horas, 7, 25, +2,'0');
   Display.update();
  }

void drawPower(){                                                 // Función grafica de la bateria

   if( VBAT > 4.1){
    
    Display.setFont(TinyFont);                                    // Seteo el tipo de fuente
    
                                                                  //Estas dos intrucciones corresponden al dibujo de la bateria
    Display.drawRect(46, 0, 57, 5); 
    Display.drawRect(58, 1, 59, 4);
    
                                                                  // Este conjunto de instrucciones son los dibujos correspondientes al
                                                                  // llenado de la bateria cuando esta está totalmente cargada

    if(VPOR <= 100){Display.printNumI(VPOR, 28, 0, 3 );}          // Indico el porcentaje de la bateria
    Display.print("%", 40, 0);
    Display.drawRect(48, 2, 49, 3);
    Display.drawRect(51, 2, 52, 3);
    Display.drawRect(54, 2, 55, 3);
       
    }
    
  if((VBAT > 3.6) && (VBAT < 4.1)){
        
    Display.setFont(TinyFont);
    
// Estas dos intrucciones corresponden al dibujo de la bateria
    Display.drawRect(46, 0, 57, 5);
    Display.drawRect(58, 1, 59, 4);
    
// Este conjunto de instrucciones son los dibujos correspondientes al
// llenado de la bateria cuando esta está con carga normal

    if(VPOR <= 100){Display.printNumI(VPOR, 28, 0, 3 );}           // Indico el porcentaje de la bateria
    Display.print("%", 40, 0);
    Display.drawRect(48, 2, 49, 3);
    Display.drawRect(51, 2, 52, 3);
        
    }
    
  if(VBAT < 3.0){

    Display.setFont(TinyFont);
    
//Estas dos intrucciones corresponden al dibujo de la bateria

    Display.drawRect(46, 0, 57, 5);
    Display.drawRect(58, 1, 59, 4);
    
// Esta instruccion son los dibujos correspondientes al
// llenado de la bateria cuando esta baja

    if(VPOR <= 100){Display.printNumI(VPOR, 28, 0, 3 );}          // Indico el porcentaje de la bateria
    
    Display.print("%", 40, 0);
    Display.drawRect(48, 2, 49, 3);
        
    }
   
}
  
void timerIsr(){                                                  // Atención a la interrupción del Timer1
  if(END_TEMP == 1){}                                             // Si END_TEMP esta en verdadero sale de la interrupción
  if(MENU == 3){                                                  // Si la iterrupción viene del Cronometro

    DecimaC++;                                                    // Incremento en uno el valor de las decimas y centecimas
    if (DecimaC == 100){                                          // Si se llagaron a 100 Decimas de segundo
    
      DecimaC = 0;                                                // DecimaC la reseteo
      Segundos++;                                                 // E incremento en uno segundos
      }
      
    
    if (Segundos == 60){                                          // Si se alcanzaron los 60 segundos

      Segundos = 0;                                               // Reseteo segundos 
      Minutos++;                                                  // E incremento en uno minutos
      }

    
    if (Minutos == 60){                                           // Si se alcanzaron los 60 minutos

      Minutos = 0;                                                // Reseteo minutos 
      
      }
  }


  if(MENU == 4){                                                  // Si la iterrupción viene del Temporizador

    if (DecimaC == 0){                                            // Si se llego a 0 Decimas de segundo
      if (Segundos == 0){                                         // Si se llego a 0 segundos
        if (Minutos == 0){                                        // Si se llego a 0 Minutos
          if (Horas == 0){                                        // Si se llego a 0 Horas
            //*Detalle Importante: Como son if encadenados cuando todas las variables de tiempo 
            // sean 0 se cumplira lo siguiente, de no ser asi se seguirán decrementando dichas variables 
           
            END_TEMP = true;                                      // END_TEMP se pone en verdadero para así poder salir de la interrupción
            VIBRA_ON = true;                                      // Activo la bandera de la alerta vibratoria
            }
            
            Minutos = 60;                                         // Coloco Minutos en 60
            Horas--;                                              // Decremento en uno las horas
          
          }
          Segundos = 60;                                          // Coloco Segundos en 60
          Minutos--;                                              // Decremento en uno los Minutos
        
        }
        DecimaC = 100;                                            // Coloco las Decimas en 60
        Segundos--;                                               // Decremento en uno los Segundos
      }
      DecimaC--;                                                  // Decremento en uno las Decimas
   }
}

void interruptSetup(){     
  // Inicializa Timer2 para lanzar una interrupción cada 2 ms.
  TCCR2A = 0x02;                                                  // DESACTIVAR PWM EN LOS PINS DIGITALES 3 Y 11, Y ENTRAR EN EL MODO CTC
  TCCR2B = 0x06;                                                  // NO FUERCE COMPARAR, 256 PRESCALER
  OCR2A = 0X7C;                                                   // ESTABLECE LA TAPA DE LA CUENTA A 124 PARA 500Hz TASA DE MUESTRA
  TIMSK2 = 0x02;                                                  // HABILITAR LA INTERRUPCIÓN EN EL PARTIDO ENTRE TIMER2 Y OCR2A
  sei();                                                          // ASEGÚRESE DE QUE LOS INTERRUPTOS GLOBALES ESTÁN HABILITADOS    
} 

// ESTE ES LA RUTINA DEL SERVICIO DE INTERRUPCIÓN TIMER 2.
// Timer 2 se asegura de que tomemos una lectura cada 2 milisegundos
ISR(TIMER2_COMPA_vect){                                           // activado cuando el Timer2 cuenta hasta 124
  cli();                                                          // inhabilitar interrupciones mientras hacemos esto
  Signal = analogRead(pulsePin);                                  // Lectura del sensor de pulsos
  sampleCounter += 2;                                             // Haga un seguimiento del tiempo en mS con esta variable
  int N = sampleCounter - lastBeatTime;                           // Monitoriza el tiempo transcurrido desde el último tiempo para evitar ruidos.

    // Encuentra el pico y el canal de la onda del pulso
  if(Signal < thresh && N > (IBI/5)*3){                           // evitar el ruido dicrótico esperando 3/5 del último IBI
    if (Signal < T){                                              // T es el canal
      T = Signal;                                                 // Realizar un seguimiento del punto más bajo en la onda de pulso
      
    }
  }

  if(Signal > thresh && Signal > P){                              // condición de Thresh ayuda a evitar el ruido
    P = Signal;                                                   // P es el pico
  }                                                               // Realizar un seguimiento del punto más alto en la onda del pulso

if (N > 250){                                                     // evitar el ruido de alta frecuencia
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) ){        
      Pulse = true;                                               // establecer la bandera de pulso cuando creemos que hay un pulso

        IBI = sampleCounter - lastBeatTime;                       // medir el tiempo entre tiempos en mS
      lastBeatTime = sampleCounter;                               // mantener un registro del tiempo para el siguiente pulso

      if(secondBeat){                                             // si este es el segundo tiempo, si secondBeat == VERDADERO
        secondBeat = false;                                       // Limpiar bandera secondBeat 
        for(int i=0; i<=9; i++){                                  // Siembra el total acumulado para obtener un BPM realista en el inicio
          rate[i] = IBI;                      
        }
      }

      if(firstBeat){                                              // si es la primera vez que encontramos un tiempo, si firstBeat == TRUE
        firstBeat = false;                                        // Limpiza de la bandera firstBeat
        secondBeat = true;                                        // establecer la segunda bandera de tiempo
        sei();                                                    // habilitar interrupciones de nuevo
        return;                                                   // El valor de IBI no es confiable, por lo tanto, deséchelo
      }   


      // mantener un total acumulado de los últimos 10 valores IBI
      word runningTotal = 0;                                      // Borrar la variable runningTotal

      for(int i=0; i<=8; i++){                                    // desplazar datos en la matriz de velocidad
        rate[i] = rate[i+1];                                      // y suelte el valor IBI más antiguo
        runningTotal += rate[i];                                  // Sume los 9 valores IBI más antiguos.
      }

      rate[9] = IBI;                                              // agregar el último IBI a la matriz de tasas
      runningTotal += rate[9];                                    // agrega el último IBI a runningTotal
      runningTotal /= 10;                                         // promedia los últimos 10 valores IBI
      BPM = 60000/runningTotal;                                   // ¿Cuántos latidos pueden caber en un minuto? eso es BPM!
      QS = true;                                                  // establecer la bandera de auto cuantificada
                                                                  // LA BANDERA QS NO ESTÁ BORRADA DENTRO DE ESTA ISR
    }                       
  }

  if (Signal < thresh && Pulse == true){                          // Cuando los valores están bajando, el ritmo ha terminado.

    Pulse = false;                                                // restablece la bandera de pulso para que podamos hacerlo de nuevo
    amp = P - T;                                                  // obtener la amplitud de la onda del pulso
    thresh = amp/2 + T;                                           // ajustar la trilla al 50% de la amplitud
    P = thresh;                                                   // restablecer estos para la próxima vez
    T = thresh;
  }

  if (N > 2500){                                                  // Si pasan 2,5 segundos sin latido
    thresh = 512;                                                 // establecer trilla por defecto
    P = 512;                                                      // Establecer P por defecto
    T = 512;                                                      // Establecer T por defecto
    lastBeatTime = sampleCounter;                                 // actualizar lastBeatTime      
    firstBeat = true;                                             // Configurarlos para evitar el ruido
    secondBeat = false;                                           // cuando recuperemos el latido del corazón
  }

  sei();                                                          // ¡Habilita las interrupciones cuando hayas terminado! 
  }                                                               // end isr
 
 
   


