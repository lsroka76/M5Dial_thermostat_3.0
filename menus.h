#define MENU_1_ITEMS                       10
#define MENU_1_ANGLE                       36
#define MENU_1_OFFSET                      166

#define MENU_1_TMODES                       0
#define MENU_1_TEMPSET                      1
#define MENU_1_TIMER                        2
#define MENU_1_TSTS                         3
#define MENU_1_WSPROGS                      4
#define MENU_1_DSTS                         5
#define MENU_1_TSEL                         6
#define MENU_1_STATUS                       7
#define MENU_1_SENSORS                      8
#define MENU_1_MMENU                        9

#define MENU_11_ITEMS                       5
#define MENU_11_ANGLE                       72
#define MENU_11_OFFSET                      166
#define MENU_11_POS                         MENU_1_WSPROGS 


#define MENU_12_ITEMS                       4
#define MENU_12_ANGLE                       90
#define MENU_12_OFFSET                      166
#define MENU_12_POS                         MENU_1_TIMER

#define MENU_13_ITEMS                       12
#define MENU_13_ANGLE                       30
#define MENU_13_OFFSET                      166
#define MENU_13_POS                         MENU_1_TSTS

#define MENU_13_HIST                        0
#define MENU_13_TEMPMIN                     1
#define MENU_13_TEMPMAX                     2
#define MENU_13_HISTMIN                     3
#define MENU_13_HISTMAX                     4
#define MENU_13_TSSM                        5
#define MENU_13_AFRAOH                      6
#define MENU_13_TEMPAFR                     7
#define MENU_13_TEMPAOH                     8
#define MENU_13_MINON                       9
#define MENU_13_MINOFF                      10
#define MENU_13_MENUUP                      11

#define MENU_14_ITEMS                       2
#define MENU_14_ANGLE                       180
#define MENU_14_OFFSET                      166
#define MENU_14_POS                         MENU_1_DSTS

#define MENU_14_AMS                         0
#define MENU_14_MENUUP                      1

const static char menu_1_labels_1[MENU_1_ITEMS][12] PROGMEM = { "TRYB",       "NASTAWA",      "USTAWIENIA", "OPCJE",      "NASTAWA",  "OPCJE",    "WYBÓR",      "STATUS", "CZUJNIKI", "EKRAN"};
const static char menu_1_labels_2[MENU_1_ITEMS][12] PROGMEM = { "PRACY",      "TEMPERATURY",  "TIMERA",     "TERMOSTATU", "PROGRAMÓW","URZĄDZENIA",  "TERMOMETRU", "",       "",         "GŁÓWNY"};
const static char menu_1_labels_3[MENU_1_ITEMS][12] PROGMEM = { "TERMOSTATU", "(°C)",         "",           "",           "TYGODNIA",     "",   "",           "",       "",          ""};

const static byte menu_1_lines[MENU_1_ITEMS] PROGMEM = {  3,  3,  2,  2,  3,  2,  2,  1,  1,  2};


const static char menu_11_labels_1[MENU_11_ITEMS][12] PROGMEM = { "PROGRAM 1",    "PROGRAM 2",  "PROGRAM 3",  "PROGRAM 4",  "POPRZEDNIE"};
const static char menu_11_labels_2[MENU_11_ITEMS][12] PROGMEM = { "TEMPERATURA",  "TEMPERATURA","TEMPERATURA","TEMPERATURA","MENU"};
const static char menu_11_labels_3[MENU_11_ITEMS][12] PROGMEM = { "",             "",           "",           "",           ""};

const static byte menu_11_lines[MENU_11_ITEMS] PROGMEM = {  2,  2,  2,  2,  2};


const static char menu_12_labels_1[MENU_12_ITEMS][12] PROGMEM = { "TIMER",  "NASTAWA","NASTAWA",    "POPRZEDNIE"};
const static char menu_12_labels_2[MENU_12_ITEMS][12] PROGMEM = { "ON/OFF", "CZASU",  "TEMPERATURY","MENU"};
const static char menu_12_labels_3[MENU_12_ITEMS][12] PROGMEM = { "",       "TIMERA", "TIMERA",     ""};

const static byte menu_12_lines[MENU_12_ITEMS] PROGMEM = {  2,  3,  3,  2};

const static char menu_13_labels_1[MENU_13_ITEMS][12] PROGMEM = { 
  "NASTAWA", 
  "NASTAWA", 
  "NASTAWA", 
  "NASTAWA",
  "NASTAWA",
  "ZMIANA", 
  "OCHRONA",
  "NASTAWA",
  "NASTAWA",  
  "MIN.",  
  "MIN.", 
  "POPRZEDNIE"  
};

const static char menu_13_labels_2[MENU_13_ITEMS][12] PROGMEM = { 
  "HISTEREZY", 
  "TEMPERATURY", 
  "TEMPERATURY", 
  "HISTEREZY",
  "HISTEREZY",
  "TEMPERATURY", 
  "P-ZAMARZ.",
  "TEMPERATURY",
  "TEMPERATURY", 
  "CZAS", 
  "CZAS", 
  "MENU" 
};

const static char menu_13_labels_3[MENU_13_ITEMS][12] PROGMEM = { 
  "(°C)",
  "MIN.(°C)",
  "MAX.(°C)",
  "MIN.(°C)",
  "MAX.(°C)",
  "->T.RĘCZNY", 
  "P-PRZEGRZ.",
  "P-ZAMARZ.",
  "P-PRZEGRZ",
  "ON",
  "OFF",
  ""  
};

const static byte menu_13_lines[MENU_13_ITEMS] PROGMEM = {  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  2};

//-----------------------------------------

const static char menu_14_labels_1[MENU_14_ITEMS][12] PROGMEM = { "AKTYWNY",  "POPRZEDNIE"};
const static char menu_14_labels_2[MENU_14_ITEMS][12] PROGMEM = { "EKRAN",    "MENU"};
const static char menu_14_labels_3[MENU_14_ITEMS][12] PROGMEM = { "GŁÓWNY",   ""};

const static byte menu_14_lines[MENU_14_ITEMS] PROGMEM = {  3,  2 };
