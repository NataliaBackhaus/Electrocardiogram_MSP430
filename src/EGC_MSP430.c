//160015766 - Natália Backhaus Pereira

#include <msp430.h> 

#define FALSE 0
#define TRUE 1
#define THRESHOLD 2400
// Funções
void ser_str(char *vet);
void ser_char(char c);
void uart1_config(void);
void adc_config(void);
void tb0_config(void);
void get_bpm(int val);
void lcd_char(char x);
void lcd_esp(char x, char *vt);
void lcd_cursor(char x);
void lcd_cmdo(char x);
void lcd_inic(void);
void lcd_aux(char dado);
int pcf_read(void);
void pcf_write(char dado);
int pcf_teste(char adr);
void i2c_config(void);
void delay(long limite);
void buffer_func(int valor);
int calc_mean(void);

// Variáveis globais
volatile int media_x, media_y;
int flag_thr           = 0;
unsigned int count_diff = 0;
char bpm[4];
float val_bpm;

// Buffer Circular
#define BUFFER_SIZE 10
int buffer[BUFFER_SIZE]={};
int indexBuffer = 0;
int flag_buffer = 0;


// Definição do endereço do PCF_8574
#define PCF_ADR1 0x3F
#define PCF_ADR2 0x27
#define PCF_ADR  PCF_ADR2

#define BR_100K    11  //SMCLK/100K = 11
#define BR_50K     21  //SMCLK/50K  = 21
#define BR_10K    105  //SMCLK/10K  = 105

int main(void){
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    __delay_cycles(500000);     //Atraso de 0,5 seg para garantir estabilidade
    uart1_config();
    tb0_config();
    adc_config();
    i2c_config();
    lcd_inic();     //Inicializar LCD
    pcf_write(8);   //Acender Back Light
    __enable_interrupt();

    //printar comandos iniciais
    ser_str("Prova A5\n");
    volatile int medida;
    char mx[5] = {0};

    while(TRUE){
         ADC12CTL0 |= ADC12ENC; //Habilitar a cada conversão
         while( (ADC12IFG&ADC12IFG0) == 0); //IFG0 ?
         medida = ADC12MEM0;

         get_bpm(medida);

         mx[0] = '0' + ((medida/1000)%10);  // thousends digit
         mx[1] = '0' + ((medida/100)%10);   // hundreds digit
         mx[2] = '0' + ((medida/10)%10);    // tens digit
         mx[3] = '0' + (medida%10);         // ones digit

         ser_str(mx);
         ser_str("\n");
     }
    return 0;
}

//Enviar uma string pela serial
void ser_str(char *pt){
    int i=0;
    while (pt[i]!=0){
        ser_char(pt[i]);
        i++;
    }
}

//Enviar um caracter pela serial
void ser_char(char x){
    UCA1TXBUF=x;
    while ((UCA1IFG&UCTXIFG)==0);   //Esperar transmitir
}

//Configurar USCI_A1 em 57.600
void uart1_config(void){
    UCA1CTL1 = UCSSEL_2 | UCSWRST;              //RST = 1 e SMCLK
    UCA1CTL0 = 0;                               //sem paridade, 8 bits, 1 stop, modo UART
    UCA1BRW = 18;                               //Divisor
    UCA1MCTL = UCBRF_0 | UCBRS_1 & ~UCOS16;     //Moduladores 2, UCOS16=0
    P4SEL |= BIT5|BIT4;
    UCA1CTL1 &= ~UCSWRST;                       //RST=0
}

//Configurar ADC
void adc_config(void){
     volatile unsigned char *pt;
     unsigned char i;
     ADC12CTL0 &= ~ADC12ENC;                    //Desabilitar para configurar
     ADC12CTL0 = ADC12SHT0_3 | ADC12ON;         //Ligar ADC
     ADC12CTL1 = ADC12CONSEQ_0 |                //Modo single
                 ADC12SHS_3 |                   //Selecionar TB0.1
                 ADC12CSTARTADD_0 |             //Resultado a partir de ADC12MEM0
                 ADC12SSEL_3;                   //ADC12CLK = SMCLK
     ADC12CTL2 = ADC12RES_2;                    //ADC12RES=2, Modo 16 bits
     ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0;

     P6SEL |= BIT0;                             //Desligar digital de P6.0,1
     ADC12CTL0 |= ADC12ENC;                     //Habilitar ADC12
}

// Configurar o timer TB0.1
void tb0_config(void){
    TB0CTL = TBSSEL_1 | MC_1;
    TB0CCTL1 = OUTMOD_6;        //Out = modo 6
    TB0CCR0 = 32767/500;        //500 Hz
    TB0CCR1 = TB0CCR0/2;        //Carga 50%
    P4DIR |= BIT0;              //P4.0 como saída
    P4SEL |= BIT0;              //P4.0 saída alternativa
    PMAPKEYID = 0X02D52;        //Liberar mapeamento
    P4MAP0 = PM_TB0CCR1A;       //TB0.1 saí por P4.0
}

void get_bpm(int val){

    int bpm_mean = 0;
    count_diff++;
    if(flag_thr==0 && val > THRESHOLD){
        val_bpm = 60/(2*(count_diff/500.0));
        buffer_func((int)val_bpm);
        bpm_mean = calc_mean();
        bpm[0] = '0' + ((bpm_mean/100)%10);   // hundreds digit
        bpm[1] = '0' + ((bpm_mean/10)%10);    // tens digit
        bpm[2] = '0' + (bpm_mean%10);         // ones digit
        lcd_cursor(0x00);
        lcd_char('B');
        lcd_char('P');
        lcd_char('M');
        lcd_char(bpm[0]);
        lcd_char(bpm[1]);
        lcd_char(bpm[2]);
        ser_str("bpm: ");
        ser_str(bpm);
        ser_str("\n");

        flag_thr=1;
        count_diff = 0;
    }
    else if(flag_thr==1 && val <= THRESHOLD){
        flag_thr=0;
    }
}



// Imprimir uma letra no LCD (x = abcd efgh)
void lcd_char(char x){
    char lsn,msn;   //nibbles
    lsn=(x<<4)&0xF0;        //lsn efgh 0000
    msn=x&0xF0;             //msn abcd 0000
    pcf_write(msn | 0x9);
    pcf_write(msn | 0xD);
    pcf_write(msn | 0x9);
    ;
    pcf_write(lsn | 0x9);
    pcf_write(lsn | 0xD);
    pcf_write(lsn | 0x9);
}

// Posicionar cursor
void lcd_cursor(char x){
    lcd_cmdo(0x80 | x);
}

// Enviar um comando (RS=0) para o LCD (x = abcd efgh)
void lcd_cmdo(char x){
    char lsn,msn;   //nibbles
    lsn=(x<<4)&0xF0;        //lsn efgh 0000
    msn=x&0xF0;             //msn abcd 0000
    pcf_write(msn | 0x8);
    pcf_write(msn | 0xC);
    pcf_write(msn | 0x8);
    ;
    pcf_write(lsn | 0x8);
    pcf_write(lsn | 0xC);
    pcf_write(lsn | 0x8);
}

// Mapeia no caracter "x" o vetor vt[]
// Reposiciona o cursor em lin=0 e col=0
void lcd_esp(char x, char *vt){
    unsigned int adr,i;
    adr = x<<3;
    lcd_cmdo(0x40 | adr);
    for (i=0; i<8; i++)
        lcd_char(vt[i]);
    lcd_cursor(0);
}

// Incializar LCD modo 4 bits
void lcd_inic(void){

    // Preparar I2C para operar
    UCB0I2CSA = PCF_ADR;    //Endereço Escravo
    UCB0CTL1 |= UCTR    |   //Mestre TX
                UCTXSTT;    //Gerar START
    while ( (UCB0IFG & UCTXIFG) == 0);          //Esperar TXIFG=1
    UCB0TXBUF = 0;                              //Saída PCF = 0;
    while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT);   //Esperar STT=0
    if ( (UCB0IFG & UCNACKIFG) == UCNACKIFG)    //NACK?
                while(1);

    // Começar inicialização
    lcd_aux(0);     //RS=RW=0, BL=1
    delay(20000);
    lcd_aux(3);     //3
    delay(10000);
    lcd_aux(3);     //3
    delay(10000);
    lcd_aux(3);     //3
    delay(10000);
    lcd_aux(2);     //2

    // Entrou em modo 4 bits
    lcd_aux(2);     lcd_aux(8);     //0x28
    lcd_aux(0);     lcd_aux(8);     //0x08
    lcd_aux(0);     lcd_aux(1);     //0x01
    lcd_aux(0);     lcd_aux(6);     //0x06
    lcd_aux(0);     lcd_aux(0xF);   //0x0F

    while ( (UCB0IFG & UCTXIFG) == 0)   ;          //Esperar TXIFG=1
    UCB0CTL1 |= UCTXSTP;                           //Gerar STOP
    while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP)   ;   //Esperar STOP
    delay(50);
}

// Auxiliar inicialização do LCD (RS=RW=0)
// *** Só serve para a inicialização ***
void lcd_aux(char dado){
    while ( (UCB0IFG & UCTXIFG) == 0);              //Esperar TXIFG=1
    UCB0TXBUF = ((dado<<4)&0XF0) | BIT3;            //PCF7:4 = dado;
    delay(50);
    while ( (UCB0IFG & UCTXIFG) == 0);              //Esperar TXIFG=1
    UCB0TXBUF = ((dado<<4)&0XF0) | BIT3 | BIT2;     //E=1
    delay(50);
    while ( (UCB0IFG & UCTXIFG) == 0);              //Esperar TXIFG=1
    UCB0TXBUF = ((dado<<4)&0XF0) | BIT3;            //E=0;
}

// Ler a porta do PCF
int pcf_read(void){
    int dado;
    UCB0I2CSA = PCF_ADR;                //Endereço Escravo
    UCB0CTL1 &= ~UCTR;                  //Mestre RX
    UCB0CTL1 |= UCTXSTT;                //Gerar START
    while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT);
    UCB0CTL1 |= UCTXSTP;                //Gerar STOP + NACK
    while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP)   ;   //Esperar STOP
    while ( (UCB0IFG & UCRXIFG) == 0);  //Esperar RX
    dado=UCB0RXBUF;
    return dado;
}

// Escrever dado na porta
void pcf_write(char dado){
    UCB0I2CSA = PCF_ADR;        //Endereço Escravo
    UCB0CTL1 |= UCTR    |       //Mestre TX
                UCTXSTT;        //Gerar START
    while ( (UCB0IFG & UCTXIFG) == 0)   ;          //Esperar TXIFG=1
    UCB0TXBUF = dado;                              //Escrever dado
    while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT)   ;   //Esperar STT=0
    if ( (UCB0IFG & UCNACKIFG) == UCNACKIFG)       //NACK?
                while(1);                          //Escravo gerou NACK
    UCB0CTL1 |= UCTXSTP;                        //Gerar STOP
    while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP)   ;   //Esperar STOP
}

// Testar endereço I2C
// TRUE se recebeu ACK
int pcf_teste(char adr){
    UCB0I2CSA = adr;                            //Endereço do PCF
    UCB0CTL1 |= UCTR | UCTXSTT;                 //Gerar START, Mestre transmissor
    while ( (UCB0IFG & UCTXIFG) == 0);          //Esperar pelo START
    UCB0CTL1 |= UCTXSTP;                        //Gerar STOP
    while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP);   //Esperar pelo STOP
    if ((UCB0IFG & UCNACKIFG) == 0)     return TRUE;
    else                                return FALSE;
}

// Configurar UCSB0 e Pinos I2C
// P3.0 = SDA e P3.1=SCL
void i2c_config(void){
    UCB0CTL1 |= UCSWRST;    // UCSI B0 em ressete
    UCB0CTL0 = UCSYNC |     //Síncrono
               UCMODE_3 |   //Modo I2C
               UCMST;       //Mestre
    UCB0BRW = BR_100K;      //100 kbps
    P3SEL |=  BIT1 | BIT0;  // Use dedicated module
    UCB0CTL1 = UCSSEL_2;    //SMCLK e remove ressete
}

void delay(long limite){
    volatile long cont=0;
    while (cont++ < limite) ;
}

// Buffer circular para armazenas os 10 últimos valores
void buffer_func(int valor){

    if(indexBuffer == BUFFER_SIZE || flag_buffer == 1){
        int i = 0;
        for(i = 0; i < BUFFER_SIZE; i++){
            buffer[i] = buffer[i+1];
        }
        if(flag_buffer == 1) buffer[indexBuffer] = valor;
        indexBuffer = BUFFER_SIZE-1;
        flag_buffer = 1;
    }
    else{
        buffer[indexBuffer] = valor;

        indexBuffer++;
    }
}

int calc_mean(void){
    int i = 0;
    int soma = 0;
    int media = 0;
    for(i=0; i < BUFFER_SIZE; i++){
        soma = soma + buffer[i];
    }
    media = soma/BUFFER_SIZE;
    return media;
}

