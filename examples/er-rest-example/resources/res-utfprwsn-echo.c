/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      Example resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include "contiki.h"

#include <string.h>
#include "contiki.h"
#include "rest-engine.h"
#include <er-coap.h>

int32_t msg_size = 0;
char msg_buffer[64];
char msg_buffer2[64];

static void res_post_put_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t offset);
static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t offset);

/* A simple actuator example. Toggles the red led */
RESOURCE(res_utfprwsn_echo,
         "title=\"UTFPRWSN ECHO\";rt=\"Control\"",
         res_get_handler,
         res_post_put_handler,
         res_post_put_handler,
         NULL);

static void res_post_put_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t offset){
    //converte o payload recebido por PUT em um pacote CoAP
    coap_packet_t *const coap_req = (coap_packet_t *)request;
    uint8_t buffer_ptr = 0;

    //verifica se o payload enviado nao eh muito grande para a requisicao
    if (coap_req->payload_len > REST_MAX_CHUNK_SIZE){
        //caso for muito grande, simplesmente configura a resposta como BAD_REQUEST e retorna:
        REST.set_response_status(response, REST.status.BAD_REQUEST);
        return;
    }

    const char *buf = NULL;
    int32_t bufnumber = 1;
    if (REST.get_query_variable(request, "buf", &buf)){
        bufnumber = atoi(buf);
    }

    printf("Buffer number %d\n", bufnumber);

    switch (bufnumber){
    case 2:
        memcpy((void*)msg_buffer2, (void*)coap_req->payload, coap_req->payload_len);
        break;
    default:
        memcpy((void*)msg_buffer, (void*)coap_req->payload, coap_req->payload_len);
    }

    //salva também o tamanho da mensagem recebida (para uso futuro)
    msg_size = coap_req->payload_len;
}

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t offset){
    uint32_t i;
    uint8_t etag = 0;

    //configura o tipo de conteúdo da mensagem:
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);

    char selectedBuffer[64];

    const char *buf = NULL;
    int32_t bufnumber = 1;
    if (REST.get_query_variable(request, "buf", &buf)){
        bufnumber = atoi(buf);
    }

    printf("Buffer number %d\n", bufnumber);

    switch (bufnumber){
    case 2:
        memcpy((void*)selectedBuffer, (void*)msg_buffer2, 64);
        break;
    default:
        memcpy((void*)selectedBuffer, (void*)msg_buffer, 64);
    }

    //etag é uma propriedade que é utilizada pelos servidores de cache para saber se a msg mudou
    //duas mensagens com o mesmo valor devem ter o mesmo etag
    for (i = 0; i < msg_size; i++){
        //neste caso, utilizamos um checksum simples como etag, mas o usuário pode usar o que quiser.
        etag += selectedBuffer[i];
    }
    REST.set_header_etag(response, (uint8_t *)&etag, 1);

    //configura o payload para ser retornado
    REST.set_response_payload(response, selectedBuffer, msg_size);
}
