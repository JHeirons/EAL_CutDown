
int const DI = 0; // USI USART
int const data = 2;

// USI UART **********************************************

unsigned char ReverseByte (unsigned char x) {
  x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
  x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
  x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
  return x;    
}

// Initialise USI for UART reception.
void InitialiseUSI (void) {  
  DDRB &= ~(1<<DI);               // Define DI as input
  USICR = 0;                      // Disable USI.
  GIFR = 1<<PCIF;                 // Clear pin change interrupt flag.
  GIMSK |= 1<<PCIE;               // Enable pin change interrupts
  PCMSK |= 1<<PCINT0;             // Enable pin change on pin 0
}

// Pin change interrupt detects start of UART reseption.
ISR (PCINT0_vect) {
  if (PINB & 1<<DI) return;       // Ignore if DI is high
  GIMSK &= ~(1<<PCIE);            // Disable pin change interrupts
  TCCR0A = 2<<WGM00;              // Timer in CTC mode
  TCCR0B = 2<<CS00;               // Set prescaler to /8
  OCR0A = 103;                    // Shift every (103+1)*8 cycles
  TCNT0 = 206;                    // Start counting from (256-52+2)
  // Enable USI OVF interrupt, and select Timer0 compare match as USI Clock source:
  USICR = 1<<USIOIE | 0<<USIWM0 | 1<<USICS0;
  USISR = 1<<USIOIF | 8;          // Clear USI OVF flag, and set counter
}

// USI overflow interrupt indicates we've received a serial byte
ISR (USI_OVF_vect) {
  USICR  =  0;                    // Disable USI         
  GIFR = 1<<PCIF;                 // Clear pin change interrupt flag.
  GIMSK |= 1<<PCIE;               // Enable pin change interrupts again
  char c = USIDR;
  sei();                          // Allow interrupts
  ParseGPS(ReverseByte(c));
}

char c = 0;   // for incoming serial data

// Example: $GPRMC,092741.00,A,5213.13757,N,00008.23605,E,0.272,,180617,,,A*7F
//char fmt[]="$GPRMC,dddtdd.ds,A,eeae.eeee,l,eeeae.eeee,o,jdk,c,dddy";
char fmt[]="$GPGGA,dddtdd.ds,eeae.eeee,l,eeeae.eeee,o,f,u,jdh,jdi,M,jdg";

int state;
unsigned int temp;
long ltmp;

// GPS variables
//volatile unsigned int Time, Csecs, Knots, Course, Date;
volatile unsigned int Time, Csecs, Fixed, Sats, HDOP, Altitude, AltRef;
volatile long Lat, Long;


void setup() {
  pinMode(data, OUTPUT);
  digitalWrite(data, LOW);
  InitialiseUSI();
}

void loop() {
  unsigned int Altitude0;
  unsigned int Fixed0;
  //while (!Fix);
  cli(); Altitude0 = Altitude; sei();
  
  Altitude0 = Altitude0 / 100;
  if (Altitude0 > 85) {
    digitalWrite(data, HIGH);
  }
  else { digitalWrite(data, LOW);}
  
        
}

void ParseGPS(char c) {
       if (c == '$') { state = 0; temp = 0; ltmp = 0; Fixed = 0;}
        char mode = fmt[state++];
        
        // If received character matches format string, return
        if (mode == c) return;
        char d = c - '0';
        
        // Ignore extra digits of precision
        if (mode == ',') state--; 
        
        // d=decimal digit; j=decimal digits before decimal point
        else if (mode == 'd') temp = temp*10 + d;
        else if (mode == 'j') { if (c != '.') { temp = temp*10 + d; state--; } }
        
        // e=long decimal digit
        else if (mode == 'e') ltmp = (ltmp<<3) + (ltmp<<1) + d; // ltmp = ltmp*10 + d;
        
        // a=angular measure
        else if (mode == 'a') ltmp = (ltmp<<2) + (ltmp<<1) + d; // ltmp = ltmp*6 + d;
        
        // t=Time - hhmm
        else if (mode == 't') { Time = temp*10 + d; temp = 0; }
        
        // s=Centisecs
        else if (mode == 's') { Csecs = temp*10 + d; temp = 0; }
        
        // l=Latitude - in minutes*1000
        else if (mode == 'l') { if (c == 'N') Lat = ltmp; else Lat = -ltmp; ltmp = 0; }
        
        // o=Longitude - in minutes*1000
        else if (mode == 'o') { if (c == 'E') Long = ltmp; else Long = -ltmp; ltmp = 0; }
         
        // f=Fixed
        else if (mode == 'f') { Fixed = temp*10 + d; temp = 0; }

        // u=Sats
        else if (mode == 'u') { Sats = temp*10 + d; temp = 0; }

        // h=HDOP
        else if (mode == 'h') { HDOP = temp*10 + d; temp = 0; }

        // i=Altitude
        else if (mode == 'i') { Altitude = temp*10 + d; temp = 0; }
        
        // g=AltRef
        else if (mode == 'g') { AltRef = temp*10 + d ; state = 0; }
        else state = 0;
}



