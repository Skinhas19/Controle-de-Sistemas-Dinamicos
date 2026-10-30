#include <SoftwareSerial.h>


SoftwareSerial bluetooth(10, 11); // RX (10), TX (11 com divisor)

// Sentido do Carrinho Automático
// -----
// |  0  | -> roda boba
// X     X -> Sensores
// |     |
//[]     [] -> motores
// |_____|

//Erro giro dos motores (motor direito é mais rápido que o esquerdo)
int erroMotor = 0; 
int velocidadeNovaB = 0;
float voltasPorSegundoMA = (float)0.0;
float voltasPorSegundoMB = (float)0.0;

//Curva de reacao
//int segundosTempoReacao = 2;

//Leds
const int pinLED = 4;

//Motor A (Esquerda)
const int pinENA = 6;  // PWM
const int pinIN1 = 8;
const int pinIN2 = 9;
const int pinEncoderDOA = 2;

//Motor B (Direita)
const int pinENB = 7;  // PWM
const int pinIN3 = 12;
const int pinIN4 = 13;
const int pinEncoderDOB = 3;

const int FUROS_DISCO = 20;
const float DIAMETRO_RODA_MM = 50.5;

int velocidadeAtual = 255;
// 'F' (para frente), 'T' (para trás), 
// 'P' (Parada Suave), 'B' (Freio), 
// 'D' (para direita), 'E' (para esquerda)
char sentidoAtualMA = 'P'; 
char sentidoAtualMB = 'P'; 

volatile unsigned long pulsosNoSegundoMA = 0;
volatile unsigned long pulsosNoSegundoMB = 0;
unsigned long tempoAnterior = 0;

void contarPulsoMA() {
  pulsosNoSegundoMA++;
}

void contarPulsoMB() {
  pulsosNoSegundoMB++;
}

// sentidoAtual = 'F' (para frente), 'T' (para trás), 
// 'P' (Parada Suave), 'B' (Freio), 
// 'D' (para direita), 'E' (para esquerda)
// Motor = 'A' ou 'B'
void atualizarMotores(char Motor) {
  int pinEN, pinINX, pinINY;
  int velNovaA = velocidadeAtual; //255
  int velNovaB = velocidadeAtual; //255
  int velNova = velocidadeAtual; //255

  if (voltasPorSegundoMA > voltasPorSegundoMB){
    //Motor A está mais rápido que B: Reduzir A
    //int resultado = (int)(a / b); 
    velNovaA = (int) velNovaA-(velNovaA * ((voltasPorSegundoMA-voltasPorSegundoMB)/voltasPorSegundoMA));
  }else if (voltasPorSegundoMA < voltasPorSegundoMB){
    velNovaB = (int) velNovaB-(velNovaB * ((voltasPorSegundoMB-voltasPorSegundoMA)/voltasPorSegundoMB));
  }

  char sentidoAtual;
  if (Motor == 'B') {
    pinEN = pinENB;
    pinINX = pinIN3;
    pinINY = pinIN4;
    sentidoAtual = sentidoAtualMB;
    velNova=velNovaB;
  }else{
    pinEN = pinENA;
    pinINX = pinIN1;
    pinINY = pinIN2;
    sentidoAtual = sentidoAtualMA;
    velNova=velNovaA;
  }

  if (sentidoAtual == 'F') {    
    analogWrite(pinEN, velNova);//velocidadeAtual);
    digitalWrite(pinINX, HIGH);
    digitalWrite(pinINY, LOW);
  } else if (sentidoAtual == 'T') {
    analogWrite(pinEN, velNova);//velocidadeAtual);
    digitalWrite(pinINX, LOW);
    digitalWrite(pinINY, HIGH);
  // } else if (sentidoAtual == 'C' && Motor == 'B') {
  //   digitalWrite(pinLED, HIGH);
  //   analogWrite(pinEN, 255);
  //   digitalWrite(pinINX, HIGH);
  //   digitalWrite(pinINY, HIGH);
  // } else if (sentidoAtual == 'C' && Motor == 'A') {
  //   digitalWrite(pinLED, HIGH);
  //   analogWrite(pinEN, 255);//velocidadeAtual);
  //   digitalWrite(pinINX, HIGH);
  //   digitalWrite(pinINY, LOW);
  } else if (sentidoAtual == 'B') {
    // FREIO RAPIDO: ENA/ENB no maximo e ambas as entradas em HIGH
    analogWrite(pinEN, 255);
    digitalWrite(pinINX, HIGH);
    digitalWrite(pinINY, HIGH);
  } else { 
    // PARADA SUAVE ('P'): entradas em LOW
    digitalWrite(pinLED, LOW);

    analogWrite(pinEN, 0);
    digitalWrite(pinINX, LOW);
    digitalWrite(pinINY, LOW);
  }
}

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);
  bluetooth.setTimeout(100);
  pinMode(pinLED, OUTPUT);

  pinMode(pinENA, OUTPUT);
  pinMode(pinIN1, OUTPUT);
  pinMode(pinIN2, OUTPUT);
  pinMode(pinEncoderDOA, INPUT);

  pinMode(pinENB, OUTPUT);
  pinMode(pinIN3, OUTPUT);
  pinMode(pinIN4, OUTPUT);
  pinMode(pinEncoderDOB, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(pinEncoderDOA), contarPulsoMA, RISING);
  attachInterrupt(digitalPinToInterrupt(pinEncoderDOB), contarPulsoMB, RISING);

  atualizarMotores('A');
  atualizarMotores('B');

  Serial.println(F("Sistema pronto com Freio Dinamico!"));
  bluetooth.println(F("Comandos: F, T, P (Suave), B (Freio), ou 0-255"));
}

void loop() {
  // Leitura Bluetooth
  if (bluetooth.available() > 0) {
    char caractere = bluetooth.peek();

    if (isDigit(caractere)) {
      int novaVelocidade = bluetooth.parseInt();
      velocidadeAtual = constrain(novaVelocidade, 0, 255);
      
      atualizarMotores('A');
      atualizarMotores('B');

      bluetooth.print(F(">> Vel PWM: "));
      bluetooth.println(velocidadeAtual);
    } else {
      char comando = bluetooth.read();

      if (comando == 'F' || comando == 'f') {
        digitalWrite(pinLED, LOW);
        sentidoAtualMA = 'F';
        sentidoAtualMB = 'F';
        atualizarMotores('A');
        atualizarMotores('B');
        bluetooth.println(F(">> FRENTE"));
      } else if (comando == 'T' || comando == 't') {
        digitalWrite(pinLED, LOW);
        sentidoAtualMA = 'T';
        sentidoAtualMB = 'T';
        atualizarMotores('A');
        atualizarMotores('B');
        bluetooth.println(F(">> TRAS"));
      } else if (comando == 'P' || comando == 'p') {
        digitalWrite(pinLED, LOW);
        sentidoAtualMA = 'P';
        sentidoAtualMB = 'P';
        atualizarMotores('A');
        atualizarMotores('B');
        bluetooth.println(F(">> PARADA SUAVE"));
      } else if (comando == 'B' || comando == 'b') {
        digitalWrite(pinLED, LOW);
        sentidoAtualMA = 'B';
        sentidoAtualMB = 'B';
        atualizarMotores('A');
        atualizarMotores('B');
        bluetooth.println(F(">> FREIO ATIVADO!"));
      } else if (comando == 'C' || comando == 'c') {
        digitalWrite(pinLED, HIGH);
        sentidoAtualMA = 'B';
        sentidoAtualMB = 'F';
        atualizarMotores('A');
        atualizarMotores('B');
        bluetooth.println(F(">> CURVA ATIVADA!"));
      }
    }
  }

  // Telemetria a cada 1 segundo
  if (millis() - tempoAnterior >= 1000) {
    tempoAnterior = millis();

    noInterrupts();
    unsigned long pulsosMA = pulsosNoSegundoMA;
    unsigned long pulsosMB = pulsosNoSegundoMB;
    pulsosNoSegundoMA = 0;
    pulsosNoSegundoMB = 0;
    interrupts();

    voltasPorSegundoMA = (float)pulsosMA / FUROS_DISCO;
    voltasPorSegundoMB = (float)pulsosMB / FUROS_DISCO;
    float rpmMA = voltasPorSegundoMA * 60.0;
    float rpmMB = voltasPorSegundoMB * 60.0;
    float perimetroMetros = (3.14159265 * DIAMETRO_RODA_MM) / 1000.0;    
    float velKmPorHoraMA = (voltasPorSegundoMA * perimetroMetros) * 3.6;
    float velKmPorHoraMB = (voltasPorSegundoMB * perimetroMetros) * 3.6;

    // Cria um buffer de 100 posições de memória (ajuste se precisar de mais espaço)
    char respostaBT[100]; 

    // Formata a string substituindo os '%d' pelas respectivas variáveis em ordem
    sprintf(respostaBT, "RPM-MA: %d - RPM-MB: %d | VelMA: %d km/h | VelMB: %d km/h", rpmMA, rpmMB, velKmPorHoraMA, velKmPorHoraMB);

    //bluetooth.println(respostaBT);
    //Serial.println(respostaBT);

    Serial.print(F("RPM-MA: "));
    Serial.print(rpmMA, 1);
    Serial.print(F(" - RPM-MB: "));
    Serial.print(rpmMB, 1);
    Serial.print(F(" | VelMA: "));
    Serial.print(velKmPorHoraMA, 2);
    Serial.print(F(" km/h"));
    Serial.print(F(" | VelMB: "));
    Serial.print(velKmPorHoraMB, 2);
    Serial.println(F(" km/h"));

    bluetooth.print(F("RPM-MA: "));
    bluetooth.print(rpmMA, 1);
    bluetooth.print(F(" - RPM-MB: "));
    bluetooth.print(rpmMB, 1);
    bluetooth.print(F(" | VelMA: "));
    bluetooth.print(velKmPorHoraMA, 2);
    bluetooth.print(F(" km/h"));
    bluetooth.print(F(" | VelMB: "));
    bluetooth.print(velKmPorHoraMB, 2);
    bluetooth.println(F(" km/h"));
  }
}