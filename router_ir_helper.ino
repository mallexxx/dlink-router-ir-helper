// Use SSH to send commands link:
// echo -e "\nhi\n" > /dev/ttyS1

#include <IRremote.h>
#include <SoftwareSerial.h>
#include "password.h"

#define PARSER_LINELEN 128
#define PARSER_NOMATCH -1
#define PARSER_EOL -2
#define CTRL(x) (x-64)
#define TOUPPER(a) (a & ~('a'-'A'))

IRsend irsend;
IRrecv irrecv(11);
SoftwareSerial mySerial(12, 8); //RX, TX

char linebuffer[PARSER_LINELEN];
uint8_t inptr, parseptr;
uint8_t enableIR = 1; 

void reset() {
  inptr=0;
  parseptr=0;
}

void serialPrint(const __FlashStringHelper* txt) {
  mySerial.write('>');
  mySerial.print(txt);
  Serial.print(txt);
}
void serialPrintln(const __FlashStringHelper* txt) {
  mySerial.write('>');
  mySerial.println(txt);
  Serial.println(txt);
}
void serialPrintln(const char* txt) {
  mySerial.write('>');
  mySerial.println(txt);
  Serial.println(txt);
}
void serialPrint(int val, int sys) {
  mySerial.write('>');
  mySerial.print(val, sys);
  Serial.print(val, sys);
}

void setup() {
  mySerial.begin(9600);
  Serial.begin(57600);

  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  digitalWrite(4, 0);
  digitalWrite(5, 0);
  digitalWrite(6, 0);
  reset();

  delay(500);
  mySerial.println(F("#My, there!"));
  Serial.println(F("#Hi, there!"));
}

void startEcho(const __FlashStringHelper* txt) {
  serialPrint(F("echo \""));
  serialPrint(txt);
}
void endEcho(const __FlashStringHelper* txt) {
  serialPrint(txt);
  serialPrintln(F("\" > /dev/pts/0"));
}
void echo(const __FlashStringHelper *txt) {
  startEcho(txt);
  endEcho(F(""));
}

void sendCmd(uint8_t model, unsigned long arg, unsigned int bits) {
  if (!arg) {
    goto cmd;
  }
  startEcho(F("echo \"Sending 0x"));
  serialPrint(arg, HEX);
  if (bits > 0) {
    serialPrint(F(" "));
    serialPrint(bits, DEC);
    endEcho(F(" bits."));
  } else {
    endEcho(F(""));
  }
    
  switch (model) {
    case NEC:
        irsend.sendNEC(arg, bits ?: 32);
        break ;
    case SONY:
        irsend.sendSony(arg, bits ?: 12);
        break ;
    case RC5:
        irsend.sendRC5(arg, bits ?: 12);
        break ;
    case RC6:
        irsend.sendRC6(arg, bits ?: 20);
        break ;
    case DISH:
        irsend.sendDISH(arg, bits ?: 12);
        break ;
    case SHARP:
        irsend.sendSharp(arg, bits ?: 16);
        break ;
    case JVC:
        irsend.sendJVC(arg, bits ?: 16, false);
        break ;
    case SANYO:
        echo(F("not supported: SANYO "));
//        irsend.sendSanyo(arg, bits);
        break ;
    case MITSUBISHI:
        echo(F("not supported: MITSUBISHI"));
//        irsend.sendMitsubishi(arg, bits);
        break ;
    case SAMSUNG:
        irsend.sendSAMSUNG(arg, bits ?: 32);
        break ;
    case LG:
        irsend.sendLG(arg, bits ?: 32);
        break ;
    case WHYNTER:
        irsend.sendWhynter(arg, bits ?: 32);
        break ;
    case AIWA_RC_T501:
        irsend.sendAiwaRCT501(arg);
        break ;
    case PANASONIC:
        irsend.sendPanasonic(arg, bits ?: 16);
        break ;
    case DENON:
        irsend.sendDenon(arg, bits ?: 14);
        break ;
        
    default:
        startEcho(F("Invalid model."));
        goto fmt;
cmd:
        startEcho(F("Invalid command."));
fmt:
        endEcho(F(" send MODEL 0xHEX [N_bits]"));
        break;
  }
}

void blink() {
  for (uint8_t i = 0; i < 3; i++) {
      digitalWrite(6, 1);
      delay(200);
      digitalWrite(6, 0);
      delay(200);
  }
}

unsigned long parseNumber() {
    while (parseptr < inptr && linebuffer[parseptr] == ' ') {
        parseptr++;
    }
    unsigned long longval = 0;
    uint8_t j = 10;
    uint8_t c;
    
    if (linebuffer[parseptr] == '0') {
        parseptr++;
    }
    switch (linebuffer[parseptr]) {
        case 'x': case 'X':
            parseptr++;
            j = 16;
            break;
        default:
            break;
    }
    
    while (parseptr < inptr) {
        switch (linebuffer[parseptr]) {
            case ' ': case ',': case ';': case ':': case '=': case '\t': case '\n': case '\r':
                parseptr++;
                goto exit_loop;
            default:
                if (linebuffer[parseptr] >= '0' && linebuffer[parseptr] <= '9') {
                    c = linebuffer[parseptr] - '0';
                } else if (linebuffer[parseptr] >= 'a' && linebuffer[parseptr] <= 'f') {
                    c = linebuffer[parseptr] - 'a' + 10;
                } else if (linebuffer[parseptr] >= 'A' && linebuffer[parseptr] <= 'F') {
                    c = linebuffer[parseptr] - 'A' + 10;
                } else {
                    return 0;
                }
                longval = longval * j + c;
                break;
        }
        parseptr++;
    }
exit_loop:
    return longval;
}

uint8_t searchForward(__FlashStringHelper * arg, uint8_t final) {
    uint8_t i = parseptr;
    uint8_t j = 0;
    uint8_t c;
    PGM_P pgm = (PGM_P)arg;

    i=parseptr;
    j = 0;
    do {
        if (j == strlen_P(pgm)) {
            if (!final || i == inptr - 1) {
                parseptr = i;
                return true;
            }
            return false;
        }
        c = pgm_read_byte(&pgm[j]);

        if (c != linebuffer[i]) {
            return false;
        }
        
        if (i == inptr - 1) {
            return false;
        }
        i++;
        j++;
    } while (true);
}

uint8_t searchBackward(__FlashStringHelper * arg) {
    uint8_t i = inptr - 1;
    uint8_t j = 0;
    uint8_t c;
    PGM_P pgm = (PGM_P)arg;

    j = strlen_P(pgm) - 1;

    do {
        c = pgm_read_byte(&pgm[j]);
        if (c != linebuffer[i]) {
            return false;
        }
        if (j == 0) {
            return true;
        }
        if (i == 0) {
            return false;
        }
        i--;
        j--;
    } while (true);
}

void  encoding (decode_results *results)
{
  switch (results->decode_type) {
    default:
    case UNKNOWN:      serialPrint(F("UNKNOWN"));       break ;
    case NEC:          serialPrint(F("NEC"));           break ;
    case SONY:         serialPrint(F("SONY"));          break ;
    case RC5:          serialPrint(F("RC5"));           break ;
    case RC6:          serialPrint(F("RC6"));           break ;
    case DISH:         serialPrint(F("DISH"));          break ;
    case SHARP:        serialPrint(F("SHARP"));         break ;
    case JVC:          serialPrint(F("JVC"));           break ;
    case SANYO:        serialPrint(F("SANYO"));         break ;
    case MITSUBISHI:   serialPrint(F("MITSUBISHI"));    break ;
    case SAMSUNG:      serialPrint(F("SAMSUNG"));       break ;
    case LG:           serialPrint(F("LG"));            break ;
    case WHYNTER:      serialPrint(F("WHYNTER"));       break ;
    case AIWA_RC_T501: serialPrint(F("AIWA_RC_T501"));  break ;
    case PANASONIC:    serialPrint(F("PANASONIC"));     break ;
    case DENON:        serialPrint(F("Denon"));         break ;
  }
}

void loop() {  
  unsigned long cmd;
  uint8_t bits;
  uint8_t i;

  uint8_t  c;
read:
  if (Serial.available()) {
    c = Serial.read();
    goto handle;
  } else if (mySerial.available()) {
    c = mySerial.read();
    goto handle;
  }
  goto endwhile;
handle:

    if (c >= 0x20)
      mySerial.write(c);
    if (inptr < PARSER_LINELEN) {
        linebuffer[inptr++] = c;
    } else {
        mySerial.write('!');
    }
    
    switch (c) {
        case '\n':
        case '\r':
            mySerial.write(13);
            mySerial.write(10);

            if (searchForward(F("blink"), true)) {
                blink();
            } else if (searchForward(F("sh "), false)) {
                serialPrintln(linebuffer + parseptr);
                
            } else if (searchForward(F("hi"), true)) {
                echo(F("Hi!:)"));
            } else if (searchForward(F("logged in as root"), true)) {
                enableIR = true;
            } else if (searchForward(F("send "), false)) {
                if (searchForward(F("rc5"), false)) {
                    i = RC5;
                }
                else if (searchForward(F("rc6"), false)) {
                    i = RC6;
                }
                else if (searchForward(F("nec"), false)) {
                    i = NEC;
                }
                else if (searchForward(F("sony"), false)) {
                    i = SONY;
                }
                else if (searchForward(F("panasonic"), false)) {
                    i = PANASONIC;
                }
                else if (searchForward(F("jvc"), false)) {
                    i = JVC;
                }
                else if (searchForward(F("samsung"), false)) {
                    i = SAMSUNG;
                }
                else if (searchForward(F("whynter"), false)) {
                    i = WHYNTER;
                }
                else if (searchForward(F("aiwa"), false)) {
                    i = AIWA_RC_T501;
                }
                else if (searchForward(F("lg"), false)) {
                    i = LG;
                }
                else if (searchForward(F("sanyo"), false)) {
                    i = SANYO;
                }
                else if (searchForward(F("mitsubishi"), false)) {
                    i = MITSUBISHI;
                }
                else if (searchForward(F("dish"), false)) {
                    i = DISH;
                }
                else if (searchForward(F("sharp"), false)) {
                    i = SHARP;
                }
                else if (searchForward(F("denon"), false)) {
                    i = DENON;
                }
                else if (searchForward(F("pronto"), false)) {
                    i = PRONTO;
                }
                else {
                  echo(F("Wrong model"));
                  goto rst;
                }

                cmd = parseNumber();
                bits = parseNumber();
                sendCmd(i, cmd, bits);
            }
rst:
            reset();
            break;
            
        case '.':
            if (searchBackward(F("/jffs/etc/init."))) {
              serialPrintln(F(""));
              reset();
            }
            break;
        case ':':
            if (searchBackward(F("Release:"))) {
              serialPrintln(F("root"));
              reset();
            }
            break;
            
        case '!': 
            if (searchBackward(F("hi!"))) {
              echo(F("Hi!:)"));
              blink();
            }
            
            break;
        
        case ' ':
            if (searchBackward(F("sword: "))) {
              serialPrintln(PASSWORD);
              reset();
            }
            
            break;
            
        default:
            break;
      }
  goto read;//  };
endwhile:

  decode_results results;
  if (enableIR && irrecv.decode(&results)) {
    startEcho(F("RECV_IR "));
    serialPrint(results.value, HEX);
    serialPrint(F(" "));
    encoding(&results);
    serialPrint(F(" "));
    serialPrint(results.bits, DEC);
    endEcho(F(""));

    irrecv.resume(); // Receive the next value
  }

  delay(100);
}
