// Projeto de Controle e Automação
// Alunos: Ismael Vianna e Lucas Santos
// Controle PID Diferencial Parametrizável via Bluetooth

#include <SoftwareSerial.h>

SoftwareSerial bluetooth(10, 11); // RX (10), TX (11)

// --- GANHOS DO PID E COMPENSAÇÃO FÍSICA (Mutáveis) ---
float KP = 6.40f;
float KI = 26.15f;
float KD = 0.39f; 

float FATOR_PWM_MA = 1.0f;     // Motor A (Direita) 
float FATOR_PWM_MB = 1.0f;     // Motor B (Esquerda) 

unsigned int velMinima = 140; 
unsigned int velMaxima = 255; 

// --- VARIÁVEIS DO PID E TEMPO ---
float erroDistancia = 0.0f; 
float erroAnterior = 0.0f; 
float P = 0.0f, I = 0.0f, D = 0.0f, PID = 0.0f; 

unsigned long tempoAnteriorPID = 0UL;
unsigned long tempoAnteriorTelemetria = 0UL;

// --- VARIÁVEIS DE TELEMETRIA ---
volatile unsigned long pulsosNoSegundoMA = 0;
volatile unsigned long pulsosNoSegundoMB = 0;
float rpmMA = 0.0f;
float rpmMB = 0.0f;
const int FUROS_DISCO = 20;

// --- PINOS E HARDWARE ---
const int pinLED = 4;

const int pinoTriggerE = A1;
const int pinoEchoE = A0;
const int pinoTriggerD = A3;
const int pinoEchoD = A2;

const int pinENA = 6;  // PWM (Direita)
const int pinIN1 = 8;
const int pinIN2 = 9;
const int pinEncoderDOA = 2;

const int pinENB = 5;  // PWM (Esquerda)
const int pinIN3 = 12;
const int pinIN4 = 13;
const int pinEncoderDOB = 3;

char sentidoAtualMA = 'P'; 
char sentidoAtualMB = 'P'; 

// --- INTERRUPÇÕES ---
void contarPulsoMA() { pulsosNoSegundoMA++; }
void contarPulsoMB() { pulsosNoSegundoMB++; }


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

  pinMode(pinoTriggerE, OUTPUT);
  pinMode(pinoEchoE, INPUT);
  digitalWrite(pinoTriggerE, LOW);

  pinMode(pinoTriggerD, OUTPUT);
  pinMode(pinoEchoD, INPUT);  
  digitalWrite(pinoTriggerD, LOW);
  
  attachInterrupt(digitalPinToInterrupt(pinEncoderDOA), contarPulsoMA, RISING);
  attachInterrupt(digitalPinToInterrupt(pinEncoderDOB), contarPulsoMB, RISING);

  Serial.println(F("Sistema pronto! Envie parametros via Bluetooth."));
  bluetooth.println(F(">> SISTEMA PRONTO. Comandos: F, P, B ou VMAX200, KP6.5, etc."));
}

void contaSegundos() {
  if (millis() - tempoAnteriorTelemetria >= 1000UL) {
    tempoAnteriorTelemetria = millis();

    noInterrupts();
    unsigned long pulsosMA = pulsosNoSegundoMA;
    unsigned long pulsosMB = pulsosNoSegundoMB;
    pulsosNoSegundoMA = 0;
    pulsosNoSegundoMB = 0;
    interrupts();

    rpmMA = ((float)pulsosMA / FUROS_DISCO) * 60.0f;
    rpmMB = ((float)pulsosMB / FUROS_DISCO) * 60.0f;
  }
}

void leitura_bluetooth() {
  if (bluetooth.available() > 0) {
    String cmd = bluetooth.readString(); 
    cmd.trim(); 
    cmd.toUpperCase(); 

    // --- COMANDOS DE PARAMETRIZAÇÃO ---
    if (cmd.startsWith("FA")) {
      FATOR_PWM_MA = cmd.substring(2).toFloat();
      bluetooth.print(F(">> OK! Fator MA: ")); bluetooth.println(FATOR_PWM_MA, 3);
    } 
    else if (cmd.startsWith("FB")) {
      FATOR_PWM_MB = cmd.substring(2).toFloat();
      bluetooth.print(F(">> OK! Fator MB: ")); bluetooth.println(FATOR_PWM_MB, 3);
    }
    else if (cmd.startsWith("KP")) {
      KP = cmd.substring(2).toFloat();
      bluetooth.print(F(">> OK! KP: ")); bluetooth.println(KP, 2);
    }
    else if (cmd.startsWith("KI")) {
      KI = cmd.substring(2).toFloat();
      bluetooth.print(F(">> OK! KI: ")); bluetooth.println(KI, 2);
    }
    else if (cmd.startsWith("KD")) {
      KD = cmd.substring(2).toFloat();
      bluetooth.print(F(">> OK! KD: ")); bluetooth.println(KD, 3);
    }
    else if (cmd.startsWith("VMAX")) {
      velMaxima = cmd.substring(4).toInt();
      velMaxima = constrain(velMaxima, 0, 255);
      bluetooth.print(F(">> OK! Vel Maxima: ")); bluetooth.println(velMaxima);
    }
    else if (cmd.startsWith("VMIN")) {
      velMinima = cmd.substring(4).toInt();
      velMinima = constrain(velMinima, 0, 255);
      bluetooth.print(F(">> OK! Vel Minima: ")); bluetooth.println(velMinima);
    }
    // --- COMANDOS DE AÇÃO BÁSICA ---
    else if (cmd == "F" || cmd == "K") {
      sentidoAtualMA = 'F';
      sentidoAtualMB = 'F';
      digitalWrite(pinLED, HIGH);
      bluetooth.println(F(">> MODO AUTOMATICO ATIVADO!"));
    } 
    else if (cmd == "P") {
      sentidoAtualMA = 'P';
      sentidoAtualMB = 'P';
      digitalWrite(pinLED, LOW);
      bluetooth.println(F(">> PARADA SUAVE"));
    } 
    else if (cmd == "B") {
      sentidoAtualMA = 'B';
      sentidoAtualMB = 'B';
      digitalWrite(pinLED, LOW);
      bluetooth.println(F(">> FREIO ATIVADO!"));
    }
  }
}

float sensoriamento_erro() {
  long duracaoE, duracaoD;
  float distE, distD;

  digitalWrite(pinoTriggerE, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinoTriggerE, LOW);
  duracaoE = pulseIn(pinoEchoE, HIGH);
  distE = (duracaoE * 0.0343f) / 2.0f;

  delay(30); 

  digitalWrite(pinoTriggerD, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinoTriggerD, LOW);
  duracaoD = pulseIn(pinoEchoD, HIGH);
  distD = (duracaoD * 0.0343f) / 2.0f;

  return (distD - distE);
}

float controlador(float Erro) {
  if (abs(Erro) < 1.5f) {
    Erro = 0.0f; // Zona Morta
  }

  float erroAtenuado = Erro / 10.0f; 
  
  float varTempo = (millis() - tempoAnteriorPID) / 1000.0f; 
  tempoAnteriorPID = millis();
  if (varTempo <= 0.0f) varTempo = 0.01f; 

  P = erroAtenuado * KP;
  I = I + (erroAtenuado * KI * varTempo);
  
  float limiteIntegral = 35.0f; 
  if (I > limiteIntegral) I = limiteIntegral;
  if (I < -limiteIntegral) I = -limiteIntegral;

  D = ((erroAtenuado - erroAnterior) / varTempo) * KD;
  PID = P + I + D;
  
  if (PID > velMaxima) PID = velMaxima;
  if (PID < -velMaxima) PID = -velMaxima;

  erroAnterior = erroAtenuado;
  return PID;
}

void atuador(float cPID) {
  if (sentidoAtualMA == 'F' && sentidoAtualMB == 'F') {
    
    int velEsqBruta = velMaxima + cPID; 
    int velDirBruta = velMaxima - cPID;

    // Fator de compensação da telemetria e limites
    int velEsq = constrain(velEsqBruta * FATOR_PWM_MB, velMinima * FATOR_PWM_MB, velMaxima * FATOR_PWM_MB);
    int velDir = constrain(velDirBruta * FATOR_PWM_MA, velMinima, velMaxima);

    analogWrite(pinENA, velDir); 
    digitalWrite(pinIN1, HIGH);
    digitalWrite(pinIN2, LOW);

    analogWrite(pinENB, velEsq); 
    digitalWrite(pinIN3, HIGH);
    digitalWrite(pinIN4, LOW);

  } else if (sentidoAtualMA == 'B') {
    analogWrite(pinENA, velMaxima);
    digitalWrite(pinIN1, HIGH);
    digitalWrite(pinIN2, HIGH);
    analogWrite(pinENB, velMaxima);
    digitalWrite(pinIN3, HIGH);
    digitalWrite(pinIN4, HIGH);
  } else {
    analogWrite(pinENA, 0);
    digitalWrite(pinIN1, LOW);
    digitalWrite(pinIN2, LOW);
    analogWrite(pinENB, 0);
    digitalWrite(pinIN3, LOW);
    digitalWrite(pinIN4, LOW);
  }
}

void loop() {
  contaSegundos();
  leitura_bluetooth();
  
  if (sentidoAtualMA == 'F') {
    erroDistancia = sensoriamento_erro();
    float velAjustar = controlador(erroDistancia);
    atuador(velAjustar);
    
    // bluetooth.print(F("Err:")); bluetooth.print(erroDistancia, 1);
    // bluetooth.print(F(" P:")); bluetooth.print(P, 1);
    // bluetooth.print(F(" I:")); bluetooth.print(I, 1);
    // bluetooth.print(F(" D:")); bluetooth.print(D, 1);
    // bluetooth.print(F(" PID:")); bluetooth.println(PID, 1);
  } else {
    atuador(0); 
  }
}