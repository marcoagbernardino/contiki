/**/
#include "contiki.h"

#include <string.h>
#include "contiki.h"
#include "ti-lib.h"
#include "rest-engine.h"
#include <er-coap.h>
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "random.h"
#include "board-peripherals.h"
#include "cpu/cc26xx-cc13xx/lpm.h"


//Frequência do PWM
#define PWM_FREQ    (5000)


static void res_post_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/* A simple actuator example. Toggles the red led */
RESOURCE(res_temp_control_cooler,
         "title=\"Cooler\";rt=\"Control\"",
         NULL,
         res_post_handler,
         res_post_handler,
         NULL);

//Configuração do LPM e PWM:
uint8_t pwm_request_max_pm(void){
    return LPM_MODE_DEEP_SLEEP;
}
void sleep_enter(void){
    leds_on(LEDS_RED);
}
void sleep_leave(void){
    leds_off(LEDS_RED);
}
LPM_MODULE(pwmdrive_module, pwm_request_max_pm, sleep_enter, sleep_leave, LPM_DOMAIN_PERIPH);

uint32_t pwminit(int32_t freq){
    /* Register with LPM. This will keep the PERIPH PD powered on
    * during deep sleep, allowing the pwm to keep working while the chip is
    * being power-cycled */

    lpm_register_module(&pwmdrive_module);
    uint32_t load = 0;
    ti_lib_ioc_pin_type_gpio_output(IOID_21);
    leds_off(LEDS_RED);

    /* Enable GPT0 clocks under active mode */
    ti_lib_prcm_peripheral_run_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_peripheral_sleep_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_peripheral_deep_sleep_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_load_set();
    while(!ti_lib_prcm_load_get());

    /* Enable GPT0 clocks under active, sleep, deep sleep */
    ti_lib_prcm_peripheral_run_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_peripheral_sleep_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_peripheral_deep_sleep_enable(PRCM_PERIPH_TIMER0);
    ti_lib_prcm_load_set();
    while(!ti_lib_prcm_load_get());

    /* Drive the I/O ID with GPT0 / Timer A */
    ti_lib_ioc_port_configure_set(IOID_21, IOC_PORT_MCU_PORT_EVENT0, IOC_STD_OUTPUT);

    /* GPT0 / Timer A: PWM, Interrupt Enable */
    ti_lib_timer_configure(GPT0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PWM);

    /* Stop the timers */
    ti_lib_timer_disable(GPT0_BASE, TIMER_A);
    ti_lib_timer_disable(GPT0_BASE, TIMER_B);

    if(freq > 0) {
        load = (GET_MCU_CLOCK / freq);
        ti_lib_timer_load_set(GPT0_BASE, TIMER_A, load);
        ti_lib_timer_match_set(GPT0_BASE, TIMER_A, load - 1);
        /* Start */
        ti_lib_timer_enable(GPT0_BASE, TIMER_A);
    }
    return load;
}


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
    int32_t velocity = 1;
    if (REST.get_query_variable(request, "v", &buf)){
        velocity = atoi(buf);
    }
    printf("Velocity %d\n", velocity);

    static uint32_t loadvalue;
    static uint32_t ticks;

    //inicializa o PWM com carga mínima (desligado?)
    loadvalue = pwminit(PWM_FREQ);

    ticks = (velocity * loadvalue) / 100;
    printf("COOLER PWM currenty_duty = %lu, ticks = %lu\n", velocity, ticks);
    ti_lib_timer_match_set(GPT0_BASE, TIMER_A, loadvalue - ticks);

}
