//
// Projeto de Controle e Automação
// Alunos: Ismael Vianna e Lucas Santos
// Disciplina: Controle e Automação
// Data atualizado: 3 nov 2026
//
// Estudo de sensoriamento com sensores 
// ultrassônicos e de velocidade.
//

#include <SoftwareSerial.h>

SoftwareSerial bluetooth(10, 11); // RX (10), TX (11 com divisor)

// Sentido do Carrinho Automático
// -----
// |  0  | -> roda boba
// X     X -> Sensores
// |     |
//[]     [] -> motores
// |_____|

// Indicadores do sistema atomático
//const int referencia = 200 // Corresponde a 20cm
const float KP = (float)2.45;//1; // Ganho KP proporcional
const float KI = (float)5.1;//1; // Ganho KI integral
const float KD = (float)0.35;//0; // Ganho KD derivativo
float sensor = (float)0.0; // resultado do sensoriamento
float erroDistancia = (float)0.0; // Erro atual de distância
float erroAnterior = (float)0.0; // Erro anterior de distância
float P = (float)0.0; // indicador proporcional atual: (Erro*KP)
float I = (float)0.0; // indicador integral atual: (I+Erro*varTempo)
float D = (float)0.0; // indicador derivativo atual: ((Erro-ErroAnterior)/varTempo)*KD
float PID = (float)0.0; // indicador PID atual: P+I+D
//long tempoAnterior = 0;

//Erro giro dos motores (motor direito é mais rápido que o esquerdo)
int erroMotor = 0; 
int velocidadeNovaB = 0;
float voltasPorSegundoMA = (float)0.0;
float voltasPorSegundoMB = (float)0.0;

volatile unsigned long pulsosNoSegundoMA = 0;
volatile unsigned long pulsosNoSegundoMB = 0;
unsigned long tempoAnterior = 0;

//Curva de reacao
//int segundosTempoReacao = 2;

// Led que informa interações como início da curva de 
// reação e reage a cada interação no modo automatico
const int pinLED = 4;

// Sensores ultrassônicos e indicadores de distância
const int pinoTriggerE = A1;
const int pinoEchoE = A0;
const int pinoTriggerD = A3;
const int pinoEchoD = A2;
//float distanciaEntreSensores = 4.5; //centímetros
long duracaoPulsoE; // Tempo de viagem da onda sonora do lado esquerdo
float distanciaCME; // Distância em centímetros do lado esquerdo
long duracaoPulsoD; // Tempo de viagem da onda sonora do lado direito
float distanciaCMD; // Distância em centímetros do lado direito

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

int velocidadeAtual = 255; //Velocidade máxima
// 'F' (para frente), 'T' (para trás), 
// 'P' (Parada Suave), 'B' (Freio), 
// 'D' (para direita), 'E' (para esquerda)
char sentidoAtualMA = 'P'; 
char sentidoAtualMB = 'P'; 



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
//Linha reta Frente e trás
void atualizarMotores(char Motor) {
  int pinEN, pinINX, pinINY;
  int velNovaA = velocidadeAtual; //255
  int velNovaB = velocidadeAtual; //255
  int velNova = velocidadeAtual; //255

  // if (voltasPorSegundoMA > voltasPorSegundoMB){
  //   //Motor A está mais rápido que B: Reduzir A
  //   //int resultado = (int)(a / b); 
  //   velNovaA = (int) velNovaA-(velNovaA * ((voltasPorSegundoMA-voltasPorSegundoMB)/voltasPorSegundoMA));
  // }else if (voltasPorSegundoMA < voltasPorSegundoMB){
  //   velNovaB = (int) velNovaB-(velNovaB * ((voltasPorSegundoMB-voltasPorSegundoMA)/voltasPorSegundoMB));
  // }

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

// Retorna o erro de distância entre os sensores
float sensoriamento_erro(){
  digitalWrite(pinoTriggerE, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinoTriggerE, LOW);
  duracaoPulsoE = pulseIn(pinoEchoE, HIGH);
  distanciaCME = (duracaoPulsoE * 0.0343) / 2.0; //vel som 343 m/s

  delay(30);

  digitalWrite(pinoTriggerD, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinoTriggerD, LOW);
  duracaoPulsoD = pulseIn(pinoEchoD, HIGH);
  distanciaCMD = (duracaoPulsoD * 0.0343) / 2.0;

  Serial.print("Distancia Esq: ");
  Serial.print(distanciaCME);
  //Serial.print(distanciaCME > 0);
  Serial.print(" cm  | ");
  Serial.print("Distancia Dir: ");
  Serial.print(distanciaCMD);
  //Serial.print(distanciaCMD > 0);
  Serial.print(" cm");

  //Serial.print("Distancia entre paredes: ");
  //float totalDist = distanciaCMD + distanciaCME + distanciaEntreSensores;
  //Serial.print(totalDist);
  //Serial.print(" cm  | ");
  //Serial.print("Distancia Entre: ");
  //Serial.print(distanciaEntreSensores);
  //Serial.print(distanciaEntreSensores > 0);
  //Serial.println(" cm");
  
  //delay(400);
  erroDistancia = distanciaCMD - distanciaCME;
  return erroDistancia;
}

float controlador(float Erro){
  float varTempo = millis() - tempoAnterior;
  tempoAnterior = millis(); // Pega o tempo atual
  varTempo = varTempo / 1000; // Quantos segundos se passaram?

  P = Erro * KP;
  if (P>255) P=255;
  if (P<-255) P=-255;

  I = I + (Erro * KI * varTempo);
  if (I>255) I=255;
  if (I<-255) I=-255;

  D = ((Erro - erroAnterior)/varTempo) * KD;
  if (D>255) D=255;
  if (D<-255) D=-255;

  PID = P + I + D;
  if (PID>255) PID=255;
  if (PID<-255) PID=-255;

  erroAnterior = Erro;
  return PID;
}

void atuador(float cPID){
  // Leitura Bluetooth
  if (bluetooth.available() > 0) {
    char comando = bluetooth.read();
    if (comando == 'P' || comando == 'p') {
      digitalWrite(pinLED, LOW);
      analogWrite(pinENA, 0);
      analogWrite(pinENB, 0);
      digitalWrite(pinIN1, LOW);
      digitalWrite(pinIN2, LOW);
      digitalWrite(pinIN3, LOW);
      digitalWrite(pinIN4, LOW);
      bluetooth.println(F(">> FREIO ATIVADO!"));
    }else if (comando == 'K' || comando == 'k') {
      digitalWrite(pinLED, HIGH);
      analogWrite(pinENA, 255);
      analogWrite(pinENB, 255);
      digitalWrite(pinIN1, HIGH);
      digitalWrite(pinIN2, LOW);
      digitalWrite(pinIN3, HIGH);
      digitalWrite(pinIN4, LOW);
      bluetooth.println(F(">> MODO AUTOMATICO ATIVADO!"));
    }else if (sentidoAtualMA == 'F' && sentidoAtualMB == 'F'){
      int iPID = (int)cPID;
      if(iPID > 0){
        analogWrite(pinENA, 255-iPID);
        analogWrite(pinENB, 255);
        digitalWrite(pinIN1, HIGH);
        digitalWrite(pinIN2, LOW);
        digitalWrite(pinIN3, HIGH);
        digitalWrite(pinIN4, LOW);
      }else if(iPID < 0){
        analogWrite(pinENA, 255);
        analogWrite(pinENB, 255+iPID);
        digitalWrite(pinIN1, HIGH);
        digitalWrite(pinIN2, LOW);
        digitalWrite(pinIN3, HIGH);
        digitalWrite(pinIN4, LOW);
      }else{
        sentidoAtualMA = 'F';
        sentidoAtualMB = 'F';
        atualizarMotores('A');
        atualizarMotores('B');
      }
    }
  }
}

void modo_normal(){
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

  // Inicialização dos sensores ultrassônicos
  pinMode(pinoTriggerE, OUTPUT);
  pinMode(pinoEchoE, INPUT);
  digitalWrite(pinoTriggerE, LOW);

  pinMode(pinoTriggerD, OUTPUT);
  pinMode(pinoEchoD, INPUT);  
  digitalWrite(pinoTriggerD, LOW);
  
  attachInterrupt(digitalPinToInterrupt(pinEncoderDOA), contarPulsoMA, RISING);
  attachInterrupt(digitalPinToInterrupt(pinEncoderDOB), contarPulsoMB, RISING);

  
  //Comando modo:livre
  //atualizarMotores('A');
  //atualizarMotores('B');

  //Comando modo:automatico
  //
  //

  Serial.println(F("Sistema pronto com Freio Dinamico!"));
  bluetooth.println(F("Comandos: F, T, P (Suave), B (Freio), ou 0-255"));
}

void loop() {
  //modo_normal();

  // modo automatico
  erroDistancia = sensoriamento_erro();
  atuador(controlador(erroDistancia));

  Serial.print(F("erro: "));
  Serial.print(erroDistancia, 2);
  Serial.print(F("P: "));
  Serial.print(P, 2);
  Serial.print(F(" - I: "));
  Serial.print(I, 2);
  Serial.print(F(" D: "));
  Serial.print(D, 2);
  Serial.print(F(" PID: "));
  Serial.println(PID, 2);    

  bluetooth.print(F("erro: "));
  bluetooth.print(erroDistancia, 2);
  bluetooth.print(F("P: "));
  bluetooth.print(P, 2);
  bluetooth.print(F(" - I: "));
  bluetooth.print(I, 2);
  bluetooth.print(F(" D: "));
  bluetooth.print(D, 2);
  bluetooth.print(F(" PID: "));
  bluetooth.println(PID, 2);
}