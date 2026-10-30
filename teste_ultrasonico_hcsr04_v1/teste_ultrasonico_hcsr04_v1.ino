//Código de teste usando dois sensores ultrassônicos um do lado oposto do outro.
//Mede a distância do sensor entre as paredes
//Mede a larguta total = Distância entre paredes + distância fixa entre os sensores.

const int pinoTriggerE = 7;
const int pinoEchoE = 6;
const int pinoTriggerD = 9;
const int pinoEchoD = 10;

float distanciaEntreSensores = 4.5; //centímetros
long duracaoPulsoE;
float distanciaCME;
long duracaoPulsoD;
float distanciaCMD;

void setup() {
  Serial.begin(9600);

  pinMode(pinoTriggerE, OUTPUT);
  pinMode(pinoEchoE, INPUT);
  digitalWrite(pinoTriggerE, LOW);

  pinMode(pinoTriggerD, OUTPUT);
  pinMode(pinoEchoD, INPUT);  
  digitalWrite(pinoTriggerD, LOW);
  //delayMicroseconds(2);
  delay(500);
}

void loop() {
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
  Serial.print("Distancia entre paredes: ");
  float totalDist = distanciaCMD + distanciaCME + distanciaEntreSensores;
    Serial.print(totalDist);
  Serial.print(" cm  | ");
  Serial.print("Distancia Entre: ");
  Serial.print(distanciaEntreSensores);
  //Serial.print(distanciaEntreSensores > 0);
  Serial.println(" cm");
  
  delay(400);
}
