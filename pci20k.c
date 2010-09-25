/*******************************************************************/
/* Routinen zur Ansteuerung der PCI20428-I/O-Karte                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*******************************************************************/


/*******************************************************************/
/* Definitionen fuer die Routinen                                  */
/*                                                                 */
/*******************************************************************/

#define FAKTOR  0.0048828125

int board_id;

int base_adr;


/*******************************************************************/
/* Initialisierung der E/A-Karte PCI20428 (ISA-Slot)               */
/*                                                                 */
/* Standard-Basis-Adresse: 0x320                                   */
/*                                                                 */
/* Port 0       : Digital-Eingabe (8 Bit)                          */
/*      1       : Digital-Ausgabe (8 Bit)                          */
/*              : 16 Kanal-Analogeingabe (12 Bit Aufl.)            */
/*              : 2 Kanal-Analogausgabe (12 Bit Aufl.)             */
/*              : Zaehler/Zeitgeber (16 Bit)                       */
/*                                                                 */
/* Aufruf       : status = init_pci();                             */
/*                                                                 */
/* Returnwert:  0  - Karte vorhanden                               */
/*              1  - keine PCI20428-Karte vorhanden                */
/*                                                                 */
/*******************************************************************/

int init_pci(void)
{
 board_id = inb (0x320);

 switch (board_id)
 {
  case 0xff:
   board_id = 0;
   return (0);
  case 0x00:
   board_id = 0;
   return (0);
  case 0x30:
   base_adr = 0x320;
   outb (0, base_adr);
   outb (0, base_adr + 1);
   outb (0, base_adr + 8);
   return (1);
  default:
   board_id = 0;
   return (0);
 }
}


/*******************************************************************/
/* Digital-Eingabe                                                 */
/*                                                                 */
/* Aufruf       : iwert = digital_eingabe (knr);                   */
/*                                                                 */
/* Returnwert:  x  - eingelesener Digitalwert                      */
/*              -1 - keine Karte vorhanden oder falsche knr        */
/*                                                                 */
/*******************************************************************/

int digital_eingabe (int knr)
{
 int dummy;

 switch (board_id)
 {
  case 0x30 :
   if (knr == 0)
   {
    dummy = inb (base_adr + 2);
    return (dummy);
   }
  default   :
    dummy = -1;
 }
 return (-1);
}


/*******************************************************************/
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

int digital_ausgabe (int knr, int wert)
{
 int dummy;

 switch (board_id)
 {
 case 0x30 :
  if (knr == 0)
  {
   outb (wert, base_adr + 2);
   return (1);
  }
 default   :
   dummy = 0;
 }
 return (0);
}


/*******************************************************************/
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

int analog_ausgabe (char knr, double wert)
{
 char high_byte, low_byte;
 int  anau_wert;

 anau_wert = (int) ((wert + 10.0) / FAKTOR);
 low_byte  = (char) (anau_wert & 0x00ff);
 high_byte = (char) ((anau_wert >> 8) & 0x000f);

 switch (board_id)
 {
 case 0x30 :
  if ((knr == 0) || (knr == 1 ))
  {
   outb (high_byte, base_adr + 0x0d + 2*knr);
   outb (low_byte,  base_adr + 0x0c + 2*knr);
   outb (0, base_adr + 0x0b);
   return (1);
  }
  else
  {
   return (0);
  }
 default   :
   return (0);
 }
}



/*******************************************************************/
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

double analog_eingabe (char knr)
{
 char   high_byte, low_byte;
 int    anei;
 double aneix;
 int    ianei;

 switch (board_id)
 {
 case 0x30 :
  if (knr <= 15)
  {
   outb (knr, base_adr + 9);
   rt_sleep (nano2count(10));
   outb (0, base_adr + 0x0a);
   do
   {
    rt_sleep (nano2count(10));
   }
   while ((inb(base_adr + 1) & 0x01) != 0x01);
   high_byte = inb (base_adr + 0x0b) & 0x0f;
   low_byte  = inb (base_adr + 0x0a);
   anei      = (((int) (high_byte)) << 8) | (((int) (low_byte)) & 0x00ff);
 
   aneix     = ((double) (anei)) * FAKTOR - 10.0;
   ianei     = (int) (aneix * 1000.0);
   return (aneix);
  }
  else return (0);
 default : return (0); 
 }
 return (0); 
}




/*******************************************************************/
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

int bit1 (int nr)
{
 return (1 << nr);
}

int bit0 (int nr)
{
 return (0xfe << nr);
}

/*******************************************************************/
/* Ende E/A-Ruoutinen                                              */
/*                                                                 */
/*******************************************************************/

