/*
 *  Definiciones configuracion
 */

#define BAUD_RATE 115200    // Serial port baud rate

#define TIMEOUT         30           // Espera para finalizar ingreso de botellas. Timer interrumpe cada 5 seg. 30 equivale a 2.5 minutos de espera.
#define INTENTOS        3            // Cantidad de intentos de envio para reinicio del modulo SIM.
#define INTERVALCHECK   120000       // Intervalo para intentos de envio SIM808 en miliseg.
#define BLINKTIME       500          // Periodo parpadeo led estado.
#define LIMITE_LLENADO  95           // Porcentaje de llenado
//#define TPO_DISPLAY   1500         // Tiempo retardo display en ms.

#define UBICACION     "XC"         // Definir 2 caracteres fijos para la ubicacion de la USI.

//Touch screen:
#define MINPRESSURE 200
#define MAXPRESSURE 1000

// Limites sensor ultrasonido en cm:
#define XMIN  20        // Valor minimo equivale a 100% de llenado
#define XMAX  120       // Valor maximo equivale a 0% de llenado
