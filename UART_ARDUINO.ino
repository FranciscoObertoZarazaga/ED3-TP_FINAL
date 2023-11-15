#include <LiquidCrystal.h>

//Declarar LCD y pines

LiquidCrystal lcd(7,6,5,4,3,2);

void setup() {

  // Inicia la comunicación serial con 8 bits de datos, sin paridad y 1 bit de parada.
  Serial.begin(600, SERIAL_8N2);
  
  lcd.begin(16, 2);  // Inicializa el LCD: 16 columnas, 2 filas
  
  //Seleccionamos en que columna y en que linea empieza a mostrar el texto
  lcd.setCursor(0,0);
  //Mostramos el texto deseado
}

void loop() {

  if (Serial.available()) {
    // Lee el byte entrante.
    int receivedByte = Serial.read();

    // Procesa o muestra el byte recibido.
    Serial.println(receivedByte);
  

    // Limpia la pantalla del LCD y muestra el nuevo valor
    lcd.setCursor(0, 0);
    lcd.print("                ");  // Borra la línea anterior
    lcd.setCursor(0, 0);
    lcd.print(receivedByte);
  }
}