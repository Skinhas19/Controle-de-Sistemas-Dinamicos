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

float voltasPorSegundoMA = (float)0.0;
float voltasPorSegundoMB = (float)0.0;

volatile unsigned long pulsosNoSegundoMA = 0;
volatile unsigned long pulsosNoSegundoMB = 0;
unsigned long tempoAnterior = 0;
unsigned int segundos = 0; // conta total de segundos passados desde que ligou o AGV
char sentidoTesteCurva = 'X'; // D: direita, E:esquerda ou X:parado

// Led que informa interações como início da curva de 
// reação e reage a cada interação no modo automatico
const int pinLED = 4;

//float distanciaEntreSensores = 4.5; //centímetros
long duracaoPulsoE; // Tempo de viagem da onda sonora do lado esquerdo
float distanciaCME; // Distância em centímetros do lado esquerdo
long duracaoPulsoD; // Tempo de viagem da onda sonora do lado direito
float distanciaCMD; // Distância em centímetros do lado direito
float rpmMA;
float rpmMB;
float perimetroMetros;
float velKmPorHoraMA;
float velKmPorHoraMB;


//Motor A (Direita)
const int pinENA = 6;  // PWM
const int pinIN1 = 8;
const int pinIN2 = 9;
const int pinEncoderDOA = 2;

//Motor B (Esquerda)
const int pinENB = 7;  // PWM
const int pinIN3 = 12;
const int pinIN4 = 13;
const int pinEncoderDOB = 3;

const int FUROS_DISCO = 20;
const float DIAMETRO_RODA_MM = 50.5;

//mínimaB é 127 (parado), 128 (anda)
//minimoA é 52 (parado), 53 (anda)
unsigned int velMinima = 80;//0;//140; //Velocidade mínima
unsigned int velMinimaA = 52;//Direita//0;//140; //Velocidade mínima
unsigned int velMinimaB = 127;//Esquerda//0;//140; //Velocidade mínima
unsigned int velMaxima = 255; //Velocidade máxima

unsigned int velocidadeAtual = 255; //Velocidade máxima
// 'F' (para frente), 'T' (para trás), 
// 'P' (Parada Suave), 'B' (Freio), 
// 'D' (para direita), 'E' (para esquerda)
char sentidoAtualMA = 'P'; //Roda solta
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
  int velNovaA = velMaxima; //255
  int velNovaB = velMaxima; //255
  int velNova = velMaxima; //255
  char sentidoAtual;

  //Corrige linha reta
  if (sentidoAtualMA=='F' && sentidoAtualMB=='F'){
    if (voltasPorSegundoMA > voltasPorSegundoMB){
      //Motor A está mais rápido que B: Reduzir A
      velNovaA = (int) velNovaA-(velNovaA * ((voltasPorSegundoMA-voltasPorSegundoMB)/voltasPorSegundoMA));
    }else if (voltasPorSegundoMA < voltasPorSegundoMB){
      velNovaB = (int) velNovaB-(velNovaB * ((voltasPorSegundoMB-voltasPorSegundoMA)/voltasPorSegundoMB));
    }
  }

  if(Motor == 'B' && sentidoAtualMB=='V'){
    velNovaB = velMinimaB;//127;
    //127 não anda
    //128 anda
  } else if(Motor == 'A' && sentidoAtualMA =='V'){
    velNovaA = velMinimaA;//52;//velMinima;
  }
  
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

    // if (Motor =='A' && sentidoAtualMB == 'V'){
    //   Serial.print(F(">> SentA:"));
    //   Serial.print(sentidoAtualMA);
    //   Serial.print(F(" VelNovaA:"));
    //   Serial.print(velNova);   

    //   bluetooth.print(F(">> SentA:"));
    //   bluetooth.print(sentidoAtualMA);
    //   bluetooth.print(F(" VelNovaA:"));
    //   bluetooth.print(velNova);    
    // }

    // if (Motor =='B' && sentidoAtualMA == 'V'){
    //   Serial.print(F(">> SentB:"));
    //   Serial.print(sentidoAtualMB);
    //   Serial.print(F(" VelNovaB:"));
    //   Serial.println(velNova);   

    //   bluetooth.print(F(">> SentB:"));
    //   bluetooth.print(sentidoAtualMB);
    //   bluetooth.print(F(" VelNovaB:"));
    //   bluetooth.println(velNova);    
    // }
  } else if (sentidoAtual == 'B') {
    // FREIO RAPIDO: ENA/ENB no maximo e ambas as entradas em HIGH
    analogWrite(pinEN, velMaxima);
    digitalWrite(pinINX, HIGH);
    digitalWrite(pinINY, HIGH);
  
  } else if (sentidoAtual == 'P') {
    // PARADA SUAVE ('P'): entradas em LOW
    analogWrite(pinEN, 0);
    digitalWrite(pinINX, LOW);
    digitalWrite(pinINY, LOW);

  } else if (sentidoAtual == 'V') {
    // Anda na velocidade mínima
    analogWrite(pinEN, velNova);
    digitalWrite(pinINX, HIGH);
    digitalWrite(pinINY, LOW);
  }


}

void contaSegundos(){
  //conta segundos
  float tempoAtual = millis(); 
  float varTempo = (tempoAtual - tempoAnterior) / 1000.0; 
  
  if (millis() - tempoAnterior >= 1000) {
    tempoAnterior = millis();
    segundos += 1;

    //TELEMETRIA A CADA 1 SEGUNDO
    noInterrupts();
    unsigned long pulsosMA = pulsosNoSegundoMA;
    unsigned long pulsosMB = pulsosNoSegundoMB;
    pulsosNoSegundoMA = 0;
    pulsosNoSegundoMB = 0;
    interrupts();

    voltasPorSegundoMA = (float)pulsosMA / FUROS_DISCO;
    voltasPorSegundoMB = (float)pulsosMB / FUROS_DISCO;
    // float rpmMA = voltasPorSegundoMA * 60.0;
    // float rpmMB = voltasPorSegundoMB * 60.0;
    // float perimetroMetros = (3.14159265 * DIAMETRO_RODA_MM) / 1000.0;    
    // float velKmPorHoraMA = (voltasPorSegundoMA * perimetroMetros) * 3.6;
    // float velKmPorHoraMB = (voltasPorSegundoMB * perimetroMetros) * 3.6;    

    rpmMA = voltasPorSegundoMA * 60.0;
    rpmMB = voltasPorSegundoMB * 60.0;
    perimetroMetros = (3.14159265 * DIAMETRO_RODA_MM) / 1000.0;    
    velKmPorHoraMA = (voltasPorSegundoMA * perimetroMetros) * 3.6;
    velKmPorHoraMB = (voltasPorSegundoMB * perimetroMetros) * 3.6;    
  }
}

void leitura_bluetooth(){
  // Leitura Bluetooth
  if (bluetooth.available() > 0) {
    char comando = bluetooth.read();

    if (comando == 'D' || comando == 'E') {
      segundos = 0;
      sentidoTesteCurva = comando; //char sentidoTesteCurva = 'X'; // D: direita, E:esquerda ou X:parado

      digitalWrite(pinLED, LOW);
        
      sentidoAtualMA = 'B';
      sentidoAtualMB = 'B';
        
      atualizarMotores('A');
      atualizarMotores('B');
    }
  } // final da verificacao de caractere no buffer do bluetooth

}

void modo_normal_curva(){
  // Escuta bluetooth
  // Se "D" faz curva para a direita, se E para a esquerda
  // Espera 3 segundos e anda pra frente (equilibrado) por 1 segundo e gira.
  // Se D, MotorA=0 solto e Motor B=255
  // Se E, MotorA=255 e Motor B=0 solto
  // Se R(right), MotorA=80 solto e Motor B=255
  // Se L(left), MotorA=255 e Motor B=(int)80*0.8

  leitura_bluetooth();

  // inicia a andar para frente em linha reta
  if (segundos == 4 && (sentidoTesteCurva == 'D' || sentidoTesteCurva == 'E')) {
    digitalWrite(pinLED, LOW);

    sentidoAtualMA = 'F';
    sentidoAtualMB = 'F';
    atualizarMotores('A'); //dir
    atualizarMotores('B'); //esq
  
    bluetooth.println(F(">> CORRIDA RETA!"));
    Serial.println(F(">> CORRIDA RETA!"));
    
  } else if (segundos == 6 && sentidoTesteCurva == 'E') {
    digitalWrite(pinLED, HIGH);

    sentidoAtualMA = 'F';
    sentidoAtualMB = 'V';//'P';
    atualizarMotores('A');//dir
    atualizarMotores('B');//esq
  
    bluetooth.println(F(">> CURVA ATIVADA - ESQ!"));
    Serial.println(F(">> CURVA ATIVADA - ESQ!"));

  } else if (segundos == 6 && sentidoTesteCurva == 'D') {
    digitalWrite(pinLED, HIGH);

    sentidoAtualMA = 'V';//'P';
    sentidoAtualMB = 'F';
    atualizarMotores('A');//dir
    atualizarMotores('B');//esq
  
    bluetooth.println(F(">> CURVA ATIVADA - DIR"));
    Serial.println(F(">> CURVA ATIVADA - DIR"));

  } else if (segundos == 8){//} && (sentidoTesteCurva == 'D' || sentidoTesteCurva == 'E')) {
    // para de girar
    digitalWrite(pinLED, LOW);

    sentidoAtualMA = 'P';
    sentidoAtualMB = 'P';
    atualizarMotores('A');//dir
    atualizarMotores('B');//esq
  
    bluetooth.println(F(">> CURVA FINALIZADA!"));
    Serial.println(F(">> CURVA FINALIZADA!"));

    sentidoTesteCurva = 'X';
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

  Serial.println(F("Sistema pronto com Freio Dinamico!"));
  bluetooth.println(F("Sistema pronto com Freio Dinamico!"));
}

void loop() {
  contaSegundos();
  //MODO CURVA DE REACAO
  modo_normal_curva();

  // Serial.print(F("RPM-MA: "));
  // Serial.print(rpmMA, 1);
  // Serial.print(F(" - RPM-MB: "));
  // Serial.print(rpmMB, 1);
  // Serial.print(F(" | VelMA: "));
  // Serial.print(velKmPorHoraMA, 2);
  // Serial.print(F(" km/h"));
  // Serial.print(F(" | VelMB: "));
  // Serial.print(velKmPorHoraMB, 2);
  // Serial.println(F(" km/h"));

  // bluetooth.print(F("RPM-MA: "));
  // bluetooth.print(rpmMA, 1);
  // bluetooth.print(F(" - RPM-MB: "));
  // bluetooth.print(rpmMB, 1);
  // bluetooth.print(F(" | VelMA: "));
  // bluetooth.print(velKmPorHoraMA, 2);
  // bluetooth.print(F(" km/h"));
  // bluetooth.print(F(" | VelMB: "));
  // bluetooth.print(velKmPorHoraMB, 2);
  // bluetooth.println(F(" km/h"));
  
  Serial.print(F(">> Sentido: "));
  Serial.print(sentidoTesteCurva);
  Serial.print(F(" - "));
  Serial.println(segundos);

  bluetooth.print(F("sentido: "));
  bluetooth.print(sentidoTesteCurva);
  bluetooth.print(F(" - "));
  bluetooth.println(segundos);
}