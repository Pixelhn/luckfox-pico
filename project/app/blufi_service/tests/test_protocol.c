/* SPDX-License-Identifier: MIT */
#include "../src/protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
struct test {struct bf b;int messages,frames;uint8_t data[4096],type;size_t n;};
static void emit(void *u,const uint8_t *p,size_t n){struct test *t=u;t->frames++;assert(n<=20);assert(p[1]&4);}
static void message(void *u,uint8_t type,const uint8_t *p,size_t n){struct test *t=u;t->messages++;t->type=type;t->n=n;memcpy(t->data,p,n);}
static void reset(struct test *t){memset(t,0,sizeof(*t));bf_init(&t->b,t,emit,message);}
int main(void){struct test t;reset(&t);
 assert(bf_crc((uint8_t *)"123456789",9)==0xd64e);
 uint8_t version[]={28,0,0,0};assert(!bf_receive(&t.b,version,4));assert(t.messages==1&&t.type==28);
 assert(bf_receive(&t.b,version,4)<0); /* sequence replay */
 reset(&t);uint8_t frag[]={1,16,0,5,5,0,10,11,12};uint8_t last[]={1,0,1,2,13,14};
 assert(!bf_receive(&t.b,frag,sizeof(frag)));assert(!t.messages);assert(!bf_receive(&t.b,last,sizeof(last)));assert(t.n==5&&t.data[4]==14);
 reset(&t);assert(!bf_receive(&t.b,frag,sizeof(frag)));last[0]=5;assert(bf_receive(&t.b,last,sizeof(last))<0);last[0]=1;
 reset(&t);uint8_t overflow[]={1,16,0,3,1,32,0};assert(bf_receive(&t.b,overflow,sizeof(overflow))<0);
 reset(&t);uint8_t plain[]={13,0,0,8,1,2,3,4,5,6,7,8};assert(bf_receive(&t.b,plain,sizeof(plain))<0);
 reset(&t);uint8_t bad[]={28,2,0,0,0,0};assert(bf_receive(&t.b,bad,sizeof(bad))<0);
 reset(&t);uint8_t ack[]={28,8,0,0};assert(!bf_receive(&t.b,ack,sizeof(ack)));assert(t.frames==1);
 reset(&t);uint8_t payload[128]={0};assert(!bf_send(&t.b,1,payload,128));assert(t.frames==11);
 /* Encrypted credentials: AES-CFB with sequence in the first IV byte. */
 reset(&t);t.b.keyed=true;t.b.mode=3;memset(t.b.key,0x55,16);uint8_t packet[14]={13,3,0,8,1,2,3,4,5,6,7,8};
 uint16_t crc=bf_crc(packet+2,10);packet[12]=crc;packet[13]=crc>>8;uint8_t iv[16]={0};int out;
 EVP_CIPHER_CTX *ctx=EVP_CIPHER_CTX_new();assert(EVP_EncryptInit_ex(ctx,EVP_aes_128_cfb128(),NULL,t.b.key,iv));assert(EVP_EncryptUpdate(ctx,packet+4,&out,packet+4,8));EVP_CIPHER_CTX_free(ctx);
 assert(!bf_receive(&t.b,packet,sizeof(packet)));assert(t.messages==1&&t.data[0]==1&&t.data[7]==8);
 bf_clear(&t.b);assert(!t.b.keyed&&t.b.mode==0);
 puts("protocol tests passed: CRC, replay, fragmentation, bounds, ACK, AES, plaintext rejection");return 0;}
