#include <LCD_I2C.h>

LCD_I2C lcd(0x3F, 16, 2);

const int trigPin = 2;
const int echoPin = A0;

long duration;
int distance;
int directionS = 1;

using byte = unsigned char;

int const DEFAULT_ROW_NUM = 2;
int const DEFAULT_COLUMN_NUM = 16;
int const DEFAULT_LINE_NUM = 8;

int gameState = 2; // 0 - przegrana, 1 - wygrana, 2 - gra idzie dalej

bool bombs[DEFAULT_ROW_NUM][DEFAULT_COLUMN_NUM + 2];

int nextByte;
int currentRow;
int currentColumn;
int currentLine;
int velocity = 0;
int points = 0;

//deklaracja początkowa tabel znaków
uint8_t clean[8] =
{
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};
uint8_t full[8] =
{
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111
};
uint8_t snake_top_first[8] =
{
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b10000
};
uint8_t snake_down_first[8] =
{
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};
uint8_t snake_top_second[8] =
{
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};
uint8_t snake_down_second[8] =
{
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};
uint8_t snake_top_last[8] =
{
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};
uint8_t snake_down_last[8] =
{
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};

void setup() {
  lcd.begin();
  lcd.backlight();

  pinMode(trigPin, OUTPUT); // Ustawia trigPin jako wyjście
  pinMode(echoPin, INPUT); // Ustawia echoPin jako wejście

  //urchomienie zdeklarowanych tabel znaków jako znaki
  lcd.createChar(1, clean);
  lcd.createChar(2, full);
  lcd.createChar(3, snake_top_first);
  lcd.createChar(4, snake_down_first);
  lcd.createChar(5, snake_top_second);
  lcd.createChar(6, snake_down_second);
  lcd.createChar(7, snake_top_last);
  lcd.createChar(8, snake_down_last);
}

//----------------------------------------
// funkcje generacyjne
//----------------------------------------

void reset() // tworzenie pierwszego i kolejnych ekranów(poziomów) gry
{
    generateBombs();

    // reset ścieżki
    for (int i = 0; i < DEFAULT_ROW_NUM; i++) {
        for (int j = 0; j < DEFAULT_COLUMN_NUM; j++) {
            if (bombs[i][j])
            {
                lcd.setCursor(j, i);
                lcd.write(2);
            }
            else
            {
                lcd.setCursor(j, i);
                lcd.write(1);
            }
        }
    }

    // reset wartości
    nextByte = 16;

    currentRow = 0;
    currentColumn = 0;
    currentLine = DEFAULT_LINE_NUM - 1;
    lcd.setCursor(currentRow, currentColumn);
    lcd.write(3);
}

void generateBombs() // generacja przeszkód(Bomb/ścian) jako tablica boolowska, która pełni role funkcji sprawdzającej
{
    int x;
    int y;
    int chance;
    bombs[0][0] = 0;
    bombs[0][1] = 0;
    bombs[1][0] = 0;
    bombs[1][1] = 0;

    for (x = 2; x < DEFAULT_COLUMN_NUM; x++)
    {
        chance = rand() % 3;
        if (chance == 0)
        {
            bombs[0][x] = 1;
            bombs[1][x] = 0;
            bombs[0][x + 1] = 0;
            bombs[1][x + 1] = 0;
            ++x;
        }
        else if (chance == 1)
        {
            bombs[0][x] = 0;
            bombs[1][x] = 1;
            bombs[0][x + 1] = 0;
            bombs[1][x + 1] = 0;
            ++x;
        }
        else
        {
            bombs[0][x] = 0;
            bombs[1][x] = 0;
        }
    }
}

//----------------------------------------
// funkcje sterujące
//----------------------------------------

void getDirection() //obsułga sensora ultradźwiękowego
{
  //zebranie pomiaru

  digitalWrite(trigPin, LOW);
  delayMicroseconds(20);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);

  //redefinicja pomiaru na centymetry

  distance = duration / 58;

  // zarządzanie podstawowądecyzją kierunku(kalibracja sektorów sterowania)

  if(distance <= 10) directionS = -1;
  else if(distance >= 15 && distance <= 25) directionS = 0;
  else if(distance >= 30) directionS = 1;
}

void goAnywhere(int directionS) // kontroler decyzyjny ruchu i dopasowania wartości/cyklu rysowania
{
    nextByte /= 2;

    if (nextByte == 0) {
        nextByte = 16;
        moveChainOfSnake();
        currentColumn++;

        if (currentColumn > 2) {
            hideColumn(currentColumn - 3);
        }

        if (isWin()) {
            gameState = 1;
            return;
        }
    }

    currentLine = currentLine - directionS;

    if (directionS == 1)
    {
        if (currentLine < 0) {
            currentLine = DEFAULT_LINE_NUM - 1;
            currentRow--;
        }
    }
    else if (directionS == -1)
    {
        if (currentLine > DEFAULT_LINE_NUM - 1) {
            currentLine = 0;
            currentRow++;
        }
    }

    if (isLose()) {
        gameState = 0;
        return;
    }
    if (currentRow==0)
    {
      snake_top_first[currentLine] += nextByte;
      printSnakeHead();
    }
    else
    {
     snake_down_first[currentLine] += nextByte;
     printSnakeHead();
    }
}

//----------------------------------------
// funkcje sprawdzające
//----------------------------------------

bool isWin() // sprawdzenie czy wąż ukończył poziom(prawastona ekranu)
{
     return (currentColumn > DEFAULT_COLUMN_NUM - 1);
}

bool isLose() // sprawdzenie czy nastąpiła kolizja węża z przeszkodą/ścianą
{
      return (currentRow < 0 || currentRow > DEFAULT_ROW_NUM - 1 || bombs[currentRow][currentColumn]);
}

//--------------------------------------------------------
// funkcje rysowania i zarządzania pamięcią znaków węża
//--------------------------------------------------------

void hideColumn(int columnNum)// chowanie kolumn za wężem
{
    lcd.setCursor(columnNum, 0);
    lcd.write(2);
    lcd.setCursor(columnNum, 1);
    lcd.write(2);
}

void moveChainOfSnake() //zarządzanie pełnego cyklu
{
  copySnake();
  clearSnake();
  updateSnake();
  printSnakeHead();
  printSnakeTail();
}

void copySnake() //przesunięcie pamięci w cyklu pełnym(zarządzanie znakami)
{
  memcpy(snake_top_last, snake_top_second, sizeof clean);
  memcpy(snake_down_last, snake_down_second, sizeof clean);
  memcpy(snake_top_second, snake_top_first, sizeof clean);
  memcpy(snake_down_second, snake_down_first, sizeof clean);
}

void clearSnake() //czszczenie aktywnej części węża na początku cyklu pełnego(zarządzanie znakami)
{
  memcpy(snake_top_first, clean, sizeof clean);
  memcpy(snake_down_first, clean, sizeof clean);
}

void updateSnake() //redefinicja znaków w pełnym cyklu na podstawie tabel znakowych
{
  lcd.createChar(3, snake_top_first);
  lcd.createChar(4, snake_down_first);
  lcd.createChar(5, snake_top_second);
  lcd.createChar(6, snake_down_second);
  lcd.createChar(7, snake_top_last);
  lcd.createChar(8, snake_down_last);
}

void printSnakeHead() //rysowanie głowy(aktywnej części)węża
{
  lcd.createChar(3, snake_top_first);
  lcd.createChar(4, snake_down_first);

  if (!bombs[0][currentColumn])
  {
    lcd.setCursor(currentColumn, 0);
    lcd.write(3);
  }

  if (!bombs[1][currentColumn])
  {
    lcd.setCursor(currentColumn, 1);
    lcd.write(4);
  }
}

void printSnakeTail() //rysowanie ogona(pasywnej części) węża
{
  if (!bombs[0][currentColumn])
  {
    lcd.setCursor(currentColumn, 0);
    lcd.write(5);
  }
  if(!bombs[1][currentColumn])
  {
    lcd.setCursor(currentColumn, 1);
    lcd.write(6);
  }
  if(!bombs[0][currentColumn - 1] && (currentColumn > 0))
  {
    lcd.setCursor(currentColumn - 1, 0);
    lcd.write(7);
  }
  if(!bombs[1][currentColumn - 1] && (currentColumn > 0))
  {
    lcd.setCursor(currentColumn - 1, 1);
    lcd.write(8);
  }
}

//--------------------------------------------------------
// funkcje końcowe gry/poziomu
//--------------------------------------------------------

void speedUP() //zarządzanie progresywnym(ograniczonym) poziomem trudności
{
  if(velocity < 200) velocity =+ 40;
}

void gameend() //ekran końca gry
{
  lcd.clear();
  epilepsy();
  lcd.setCursor(3, 0);
  lcd.print("YOU LOSE!");
  lcd.setCursor(3, 1);
  lcd.print("SCORE: ");
  lcd.print(points);
  delay(1000);
  points = 0;
  clearSnake();
  velocity = 0;
}

void epilepsy() //efekt ekranu końcowego
{
  for (int i = 0; i < 7; ++i)
    {
        lcd.backlight();
        delay(20);
        lcd.noBacklight();
        delay(20);
    }
    lcd.backlight();
}

//--------------------------------------------------------
//pętla programu
//--------------------------------------------------------

void loop()
{
  reset();  //początek poziomu
  gameState = 2;
  while (gameState == 2) //pętla gry
  {
    getDirection();
    delay(200 - velocity);
    goAnywhere(directionS);
  }
  if (gameState == 0) //koniec gry
  {
    gameend();
  }
  else  //przejście na następny poziom
  {
    speedUP();
    points++;
  }
}
