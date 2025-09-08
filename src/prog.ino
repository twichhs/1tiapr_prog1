void setup() {
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");
  pinMode(23, OUTPUT); // Configura o pino 23 como saída
  pinMode(22, OUTPUT); // Configura o pino 23 como saída
}

void loop() {
  digitalWrite(23, HIGH);
  digitalWrite(22, LOW);
  delay(1000);
  digitalWrite(23, LOW);
  digitalWrite(22, HIGH);
  delay(1000);
  Serial.println("Cuidado, carros.");
}

