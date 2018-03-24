/**/
#include "contiki.h"

#include <string.h>
#include "contiki.h"
#include "ti-lib.h"
#include "rest-engine.h"
#include <er-coap.h>
#include "board-peripherals.h"

//Enderaçamento do indicador de LEDs
#define LED_AZUL            (IOID_30)
#define LED_VERDE           (IOID_29)
#define LED_VERMELHO        (IOID_28)

static void res_post_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/* A simple actuator example. Toggles the red led */
RESOURCE(res_temp_control_leds,
         "title=\"Escala de LEDs\";rt=\"Control\"",
         NULL,
         res_post_handler,
         res_post_handler,
         NULL);

static void
res_post_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
    //converte o payload recebido por POST em um pacote CoAP
    coap_packet_t *const coap_req = (coap_packet_t *)request;

    uint8_t buffer_ptr = 0;
    //verifica se o payload enviado nao eh muito grande para a requisicao
    if (coap_req->payload_len > REST_MAX_CHUNK_SIZE){
        //caso for muito grande, simplesmente configura a resposta como BAD_REQUEST e retorna:
        REST.set_response_status(response, REST.status.BAD_REQUEST);
        return;
    }

    const char *buf = NULL;
    uint32_t cor = 1;
    if (REST.get_query_variable(request, "cor", &buf)){
        cor = atoi(buf);
    }
    printf("Buffer number %d\n", cor);

    int azul = cor == 1 ? 1:0;
    int verde = cor == 2 ? 1:0;
    int vermelho = cor ==3 ? 1:0;

    printf("led azul: %d, led verde: %d, led vermelho: %d", azul, verde, vermelho);
    //Configura os pinos como saída:
    IOCPinTypeGpioOutput(LED_AZUL);
    IOCPinTypeGpioOutput(LED_VERDE);
    IOCPinTypeGpioOutput(LED_VERMELHO);

    GPIO_clearDio(LED_AZUL);
    GPIO_clearDio(LED_VERDE);
    GPIO_clearDio(LED_VERMELHO);

    if (azul)
        GPIO_setDio(LED_AZUL);
    else if (verde)
        GPIO_setDio(LED_VERDE);
    else if (vermelho)
        GPIO_setDio(LED_VERMELHO);
}
