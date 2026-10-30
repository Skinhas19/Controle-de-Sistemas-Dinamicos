#include <SoftwareSerial.h>

SoftwareSerial bluetooth(10, 11); // RX (10), TX (11 com divisor)

// Sentido do Carrinho Automático
// -----
// |  0  | -> roda boba
// X     X -> Sensores
// |     |
//[]     [] -> motores
// |_____|

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

int velocidadeAtual = 100;
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
  char sentidoAtual;
  if (Motor == 'B') {
    pinEN = pinENB;
    pinINX = pinIN3;
    pinINY = pinIN4;
    sentidoAtual = sentidoAtualMB;
  }else{
    pinEN = pinENA;
    pinINX = pinIN1;
    pinINY = pinIN2;
    sentidoAtual = sentidoAtualMA;
  }

  if (sentidoAtual == 'F') {
    analogWrite(pinEN, velocidadeAtual);
    digitalWrite(pinINX, HIGH);
    digitalWrite(pinINY, LOW);
  } else if (sentidoAtual == 'T') {
    analogWrite(pinEN, velocidadeAtual);
    digitalWrite(pinINX, LOW);
    digitalWrite(pinINY, HIGH);
  } else if (sentidoAtual == 'B') {
    // FREIO RAPIDO: ENA/ENB no maximo e ambas as entradas em HIGH
    analogWrite(pinEN, 255);
    digitalWrite(pinINX, HIGH);
    digitalWrite(pinINY, HIGH);
  } else { 
    // PARADA SUAVE ('P'): entradas em LOW
    analogWrite(pinEN, 0);
    digitalWrite(pinINX, LOW);
    digitalWrite(pinINY, LOW);
  }
}

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);
  bluetooth.setTimeout(100);

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
        sentidoAtualMA = 'F';
        sentidoAtualMB = 'F';
        atualizarMotores('A');
        atualizarMotores('B');
        bluetooth.println(F(">> FRENTE"));
      } else if (comando == 'T' || comando == 't') {
        sentidoAtualMA = 'T';
        sentidoAtualMB = 'T';
        atualizarMotores('A');
        atualizarMotores('B');
        bluetooth.println(F(">> TRAS"));
      } else if (comando == 'P' || comando == 'p') {
        sentidoAtualMA = 'P';
        sentidoAtualMB = 'P';
        atualizarMotores('A');
        atualizarMotores('B');
        bluetooth.println(F(">> PARADA SUAVE"));
      } else if (comando == 'B' || comando == 'b') {
        sentidoAtualMA = 'B';
        sentidoAtualMB = 'B';
        atualizarMotores('A');
        atualizarMotores('B');
        bluetooth.println(F(">> FREIO ATIVADO!"));
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

    float voltasPorSegundoMA = (float)pulsosMA / FUROS_DISCO;
    float voltasPorSegundoMB = (float)pulsosMB / FUROS_DISCO;
    float rpmMA = voltasPorSegundoMA * 60.0;
    float rpmMB = voltasPorSegundoMB * 60.0;
    float perimetroMetros = (3.14159265 * DIAMETRO_RODA_MM) / 1000.0;    
    float velKmPorHoraMA = (voltasPorSegundoMA * perimetroMetros) * 3.6;
    float velKmPorHoraMB = (voltasPorSegundoMB * perimetroMetros) * 3.6;

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