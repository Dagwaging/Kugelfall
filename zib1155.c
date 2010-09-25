/***************************************************************************
*                                                                          *
*  Library  Z C o u n t e r                                                *
*                                                                          *
*  Routinen zum Ansteuern der Z�hler-Interface-Karte ZIB1155C              *
*  *************************************************************************
*                                                                          *
*  Eigenheiten:                                                            *
*  - 4 voneinander unabh�ngige 32-Bit-Z�hler:                              *
*    Z�hlernummern: 0, 1, 2 und 3                                          *
*  - alle 32 Bits werden gleichzeitig ausgelesen, beim Schreiben werden    *
*    zuertst die h�herwertigen Bytes geschrieben                           *
*  - voneinander unabh�ngige Z�hlermodi:                                   *
*      . DirektUp,                                                         *
*        DirektDown: die anliegenden Impulse inkrementieren bzw.           *
*                    dekrementieren den Z�hlerstand, unabh�ngig von        *
*                    ihrer "Richtung" (Phasenverschiebung)                 *
*      . Einfach:                                                          *
*        Zweifach:                                                         *
*        Vierfach:   die "Richtung" der anliegenden Impulsfolgen wird      *
*                    ausgewertet, der Z�hlerstand entsprechend erh�ht      *
*                    oder erniedrigt                                       *
*      . Dreifach:   diese Variante wird vom Hersteller der Z�hler-        *
*                    platine nicht dokumentiert, kann aber hier            *
*                    benutzt werden                                        *
*  - Hysterese: f�r jeden Z�hler kann bei der Initialisierung              *
*               angegeben werden, ob eine hardwaremaessige Jitter-         *
*               Erkennung vor dem Z�hler aktiv sein soll; dies ver-        *
*               hindert das Erkennen von falschen Impulsen, z.B. bei       *
*               Wellentorsion                                              *
*                                                                          *
*  - 2 8-Bit-Digitaleingabe-Ports (Knr 0 bzw. 1)          		   *
*  - 2 8-Bit-Digitalausgabe-Ports (Knr 0 bzw. 1)			   *
*  - der Portzugriff erfolgt zeitlich versetzt zuerst auf das              *
*    h�herwertige, dann auf das niederwertige Byte                         *
*                                                                          *
****************************************************************************
* history:                                                                 *
*                                                                          *
* mad      09.06.91 H.Baum, T.Schneele                                     *
* chang'd  16.01.94 H.J.Hoegerle, H.-J.Hafner                              *
* chang'd  xx.04.05 Bi.                                                    *
****************************************************************************/

//#include <stdio.h>
//#include <stdlib.h>
//#include </usr/include/sys/io.h>

//#include "zib1155.h"


// #define U_BYTE unsigned char
// #define BOOLEAN unsigned char
// #define TRUE 1
// #define FALSE 0



#define  ZIBBaseAdr           0x100  /* hier eventuell aendern */
  /* Basisadresse, die auf dem Zaehlermodul eingestellt ist */
#define  ZIBCounterOffset      0x08
#define  ZIBStatusIn           0x00
#define  ZIBStrobeOut          0x00
#define  ZIBCounterLSB         0x01
#define  ZIBCodeOut            0x05
#define  ZIBIOPort             0x06




  /* die implementierten Zaehlermodi */
typedef enum  {DirektUp, DirektDown, Einfach, Zweifach, Dreifach, Vierfach}
   ZIBCounterMode ;

  /* f�r ZIBWaitForCounter */
typedef enum {kleiner, gleich, groesser} ZIBRange;



/***************************************************************************/
/* function ZIBInitCounter                                                 */
/***************************************************************************/
/*                                                                         */
/* Z�hler 'Nr' initialisieren                                              */
/*                                                                         */
/* Aufruf:          ZIBInitCounter (Nr, Einfach, TRUE);                    */
/*                  ZIBInitCounter (3, Vierfach, FALSE);                   */
/*                                                                         */
/***************************************************************************/
void ZIBInitCounter (char Nr, ZIBCounterMode Mode, char Hysterese)
{
   char CodeByte;

   if (Nr > 3) return;				/* Nicht m�glich 	   */

   CodeByte = 0;				/* Befehlsbyte ermitteln   */
   switch (Mode)
   {
      case DirektUp:   CodeByte = 128 + 96; break;
      case DirektDown: CodeByte = 128;      break;
      case Einfach:    CodeByte =   5;      break;
      case Zweifach:   CodeByte =   1;      break;
      case Dreifach:   CodeByte =   4;      break;
      case Vierfach:   CodeByte =   0;      break;
   }

  if (Hysterese)				/* Jitter-Erkennung ?      */
     CodeByte |= 0x60;

						/* Ausgabe an Register     */
  outb (CodeByte, ZIBBaseAdr + Nr * ZIBCounterOffset + ZIBCodeOut);
}




/***************************************************************************/
/* function ZIBGetCounter                                                  */
/***************************************************************************/
/*                                                                         */
/* Der Z�hlerstand des Z�hlers 'Nr' wird ausgelesen und �bergeben.         */
/*                                                                         */
/* Das Auslesen der vier Bytes erfolgt gleichzeitig, es entstehen dabei    */
/* also keine Fehler durch weiter eintreffende Impulse.                    */
/*                                                                         */
/* Aufruf:          asdf = ZIBGetCounter (ctr);                            */
/*                  asdf = ZIBSetCounter (3);                              */
/*                                                                         */
/***************************************************************************/
unsigned long int ZIBGetCounter (char Nr)
{
   int i;
   unsigned long int Temp;

   if ( Nr > 3)  				/* Nicht m�glich           */
      return 0;

   						/* Zaehlerstand in �ber-   */
			 			/* gaberegister sicher     */
   outb (0, ZIBBaseAdr + Nr*ZIBCounterOffset + ZIBStrobeOut);

   Temp = 0;					/* Z�hlerst�nde aus �ber-  */
   for (i = 3; i >= 0; i--)			/* register auslese        */
   {
      Temp += inb (ZIBBaseAdr + Nr*ZIBCounterOffset + ZIBCounterLSB + i);
      if (i > 0)
         Temp <<= 8;  				/* Multiplikation mit 256  */
   }

  return Temp; 					/* R�ckgabewert 	   */
}




/***************************************************************************/
/* function ZIBSetCounter                                                  */
/***************************************************************************/
/*                                                                         */
/* Z�hler Nummer 'Nr' auf definierten vorzeichenbehafteten Wert 'SetTo'    */
/* setzen. Dabei werden zuerst die h�herwertigen, dann die niederwertigen  */
/* Bytes geschrieben. Dies ist zu beachten, wenn w�hrend der Ausf�hrung    */
/* des Befehls Z�hlimpulse eintreffen!                                     */
/*                                                                         */
/* Aufruf:          ZIBSetCounter (ctr, count);                            */
/*                  ZIBSetCounter (3, 100);                                */
/*                                                                         */
/***************************************************************************/
void ZIBSetCounter (char Nr, unsigned long int SetTo)
{
   int i;

   if (Nr > 3) 					/* Nicht m�glich           */
     return;

   for (i = 3; i >= 0; i--)			/* ausgeben der vier Bytes */
      outb ((char) (SetTo >> (8*i)),
            ZIBBaseAdr + Nr*ZIBCounterOffset + ZIBCounterLSB+i);
}




/***************************************************************************/
/* function ZIBGetPort8                                                    */
/***************************************************************************/
/*                                                                         */
/* 8-Bit Digitaleingabe                                                    */
/*                                                                         */
/* Liest ein 8-Bit-Wert von der Digitaleingabe mit der Kanalnummer 'Nr'    */
/*                                                                         */
/* Aufruf:          ewert = ZIBGetPort8 (KNR);                             */
/*                  ewert = ZIBGetPort8 (1);                               */
/*                                                                         */
/***************************************************************************/
char ZIBGetPort8 (char Nr)
{
   if (Nr > 1)  				/* Nur 0/1 m�glich         */
      return (0);

   return( inb (ZIBBaseAdr + ZIBIOPort + Nr));	/* Eingaberegister lesen   */  
}




/***************************************************************************/
/* function ZIBSetPort8                                                    */
/***************************************************************************/
/*                                                                         */
/* 8-Bit Digitalausgabe                                                    */
/*                                                                         */
/* Setzt ein 8-Bit-Wert der Digitalausgabe der Kanalnummer 'Nr'		   */
/* mit dem Byte-Wert 'SetTo'      					   */ 
/*                                                                         */
/* Aufruf:          ZIBSetPort8 (KNR, abcd);                               */
/*                  ZIBSetPort8 (1, 0xaa);                                 */
/*                                                                         */
/***************************************************************************/
void ZIBSetPort8 (char Nr, char SetTo)
{
   if( Nr > 1 )   				/* Nur 0/1 m�glich         */
      return;
   
   outb (SetTo, ZIBBaseAdr + ZIBIOPort + Nr);
}




/***************************************************************************/
/* function ZIBGetPort16                                                   */
/***************************************************************************/
/*                                                                         */
/* 16-Bit Digitaleingabe (als 'int' einlesen)                              */
/*                                                                         */
/* Liest die beiden 8-Bit-Ports der Digitaleingabe als Integerwert 'abcd'  */
/* Erst wird das h�terwertige, dann das niederwertige Byte eingelesen!     */
/*                                                                         */
/* Aufruf:          abcd = ZIBGetPort16 ();                                */
/*                                                                         */
/***************************************************************************/
int ZIBGetPort16 (void)
{
   return ((ZIBGetPort8 (1) << 8) + ZIBGetPort8 (0));
}




/***************************************************************************/
/* function ZIBSetPort16                                                   */
/***************************************************************************/
/*                                                                         */
/* 16-Bit Digitalausgabe (als 'int' ausgeben)                              */
/*                                                                         */
/* Setzt die beiden 8-Bit-Ports der Digitalausgabe auf den Wert 'SetTo'    */
/* Erst wird das niederwertige, dann das h�herwertige Byte ausgegeben!     */
/*                                                                         */
/* Aufruf:          ZIBSetPort16 (abcd);                                   */
/*                  ZIBSetPort16 (0x1234);                                 */
/*                                                                         */
/***************************************************************************/
void ZIBSetPort16 (int SetTo)
{
   ZIBSetPort8 (1, SetTo >> 8);  		/* h�herwertiges Byte 	   */
   ZIBSetPort8 (0, SetTo && 256);  		/* niederwertiges Byte     */
}




/***************************************************************************/
/* function ZIBInit                                                        */
/***************************************************************************/
/*                                                                         */
/* Initialisierung der Z�hler bzw. des Ausgaberegisters                    */
/*                                                                         */
/* Alle Z�hler auf: Vierfachz�hlung                                        */
/*                  Z�hlerst�nde auf 0                                     */
/*                  keine Jitter-Erkennung                                 */
/* Digital-Ausgabe: Kanal 0 auf 0x00                                       */
/*                  Kanal 1 auf 0x00                                       */
/*                                                                         */
/* Aufruf:          ZIBInit ();                                            */
/*                                                                         */
/***************************************************************************/
void ZIBInit (void)
{
  ZIBInitCounter (0, Vierfach, 0);		/* Z�hler initialisieren   */
  ZIBSetCounter  (0, 0);			/* Z�hler auf 0 setzen     */
  ZIBInitCounter (1, Vierfach, 0);
  ZIBSetCounter  (1, 0);
  ZIBInitCounter (2, Vierfach, 0);
  ZIBSetCounter  (2, 0);
  ZIBInitCounter (3, Vierfach, 0);
  ZIBSetCounter  (3, 0);

  ZIBSetPort16 (0);				/* Digitalausgabe setzen   */
}

/***************************************************************************/
/* Ende der ZCounter 							   */
/***************************************************************************/
