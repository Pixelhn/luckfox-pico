/* SPDX-License-Identifier: MIT
 * BluFi 1.3 wire compatibility: ESP-IDF v5.4.2, Android b98ac52.
 * Independent implementation; security exchange follows the CC0 ESP-IDF example.
 */
#include "protocol.h"
#include <string.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>

uint16_t bf_crc(const uint8_t *p, size_t n)
{
    uint16_t c = 0xffff;
    while (n--) {
        c ^= (uint16_t)*p++ << 8;
        for (int i = 0; i < 8; ++i) c = (c << 1) ^ ((c & 0x8000) ? 0x1021 : 0);
    }
    return (uint16_t)~c;
}
void bf_init(struct bf *b, void *u, void (*e)(void *, const uint8_t *, size_t), void (*m)(void *, uint8_t, const uint8_t *, size_t))
{
    memset(b, 0, sizeof(*b)); b->user=u; b->emit=e; b->message=m;
}
void bf_clear(struct bf *b)
{
    void *u=b->user; void (*e)(void *, const uint8_t *, size_t)=b->emit;
    void (*m)(void *, uint8_t, const uint8_t *, size_t)=b->message;
    OPENSSL_cleanse(b,sizeof(*b)); bf_init(b,u,e,m);
}
static int crypt(struct bf *b, uint8_t seq, uint8_t *p, size_t n, int enc)
{
    uint8_t iv[16]={0}; int out=0, tail=0; iv[0]=seq;
    EVP_CIPHER_CTX *ctx=EVP_CIPHER_CTX_new();
    int ok=b->keyed && ctx && EVP_CipherInit_ex(ctx,EVP_aes_128_cfb128(),NULL,b->key,iv,enc)==1
        && EVP_CipherUpdate(ctx,p,&out,p,(int)n)==1 && EVP_CipherFinal_ex(ctx,p+out,&tail)==1;
    EVP_CIPHER_CTX_free(ctx); return ok ? 0 : -1;
}
int bf_send(struct bf *b, uint8_t type, const uint8_t *data, size_t n)
{
    if (n>BF_LIMIT || (!data && n)) return -1;
    size_t pos=0;
    do {
        uint8_t p[20]={type,4,b->tx++,0};
        /* Conservative MTU=23. 12 bytes content leaves room for fragment header and CRC. */
        size_t left=n-pos, take=left>12?12:left, off=4;
        if (left>12) {p[1]|=16; p[off++]=left&255; p[off++]=left>>8;}
        if(take) memcpy(p+off,data+pos,take);
        p[3]=off+take-4;
        size_t end=4+p[3];
        bool isdata=(type&3)==1 && type!=1;
        bool check=isdata?(b->mode&1):((type&3)==0 && (b->mode&16));
        if(check){p[1]|=2; uint16_t crc=bf_crc(p+2,p[3]+2);p[end++]=crc&255;p[end++]=crc>>8;}
        if(isdata && (b->mode&2)){p[1]|=1;if(crypt(b,p[2],p+4,p[3],1))return -1;}
        b->emit(b->user,p,end);pos+=take;
    }while(pos<n);
    return 0;
}
int bf_receive(struct bf *b, const uint8_t *in, size_t n)
{
    uint8_t p[261];
    if(n<4 || n>sizeof(p))return -1;
    memcpy(p,in,n);
    uint8_t type=p[0], flags=p[1], len=p[3];
    if((flags&0xe4) || (type&3)>1 || n!=(size_t)(4+len+((flags&2)?2:0)) || p[2]!=b->rx)return -1;
    if((flags&1) && crypt(b,p[2],p+4,len,0))return -1;
    if(flags&2){uint16_t c=bf_crc(p+2,len+2);if(c!=(uint16_t)(p[4+len]|p[5+len]<<8))return -1;}
    /* Credentials must use the negotiated encrypted+checksummed data mode. */
    if((type==9 || type==13) && (!b->keyed || (flags&3)!=3 || (b->mode&3)!=3))return -1;
    b->rx++;
    if(flags&8)bf_send(b,0,p+2,1);
    const uint8_t *v=p+4; size_t count=len;
    if(flags&16){
        if(len<3)return -1;
        size_t remaining=v[0]|v[1]<<8;
        if(!b->used){b->total=remaining;b->frag_type=type;b->frag_flags=flags&3;}
        if(b->total>BF_LIMIT || type!=b->frag_type || (flags&3)!=b->frag_flags || remaining!=b->total-b->used || count-2>=remaining)return -1;
        memcpy(b->buf+b->used,v+2,count-2);b->used+=count-2;return 0;
    }
    if(b->used){
        if(type!=b->frag_type || (flags&3)!=b->frag_flags || b->used+count!=b->total)return -1;
        memcpy(b->buf+b->used,v,count);v=b->buf;count=b->total;b->used=0;
    }
    b->message(b->user,type,v,count);
    OPENSSL_cleanse(b->buf,sizeof(b->buf));b->total=0;
    return 0;
}
static BIGNUM *read_bn(const uint8_t **p,size_t *n)
{
    if(*n<2)return NULL;
    size_t len=((*p)[0]<<8)|(*p)[1];*p+=2;*n-=2;
    if(!len || len>*n || len>128)return NULL;
    BIGNUM *b=BN_bin2bn(*p,len,NULL);*p+=len;*n-=len;return b;
}
int bf_negotiate(struct bf *b,const uint8_t *v,size_t n)
{
    if(n==3 && v[0]==0){b->dh_len=(v[1]<<8)|v[2];return b->dh_len<=390 && b->dh_len>=135?0:-1;}
    if(n<2 || v[0]!=1 || n!=b->dh_len+1 || b->keyed)return -1;
    v++;n--;
    BIGNUM *p=read_bn(&v,&n),*g=read_bn(&v,&n),*peer=read_bn(&v,&n);
    BIGNUM *expected=NULL,*priv=BN_new(),*pub=BN_new(),*shared=BN_new(),*limit=BN_new();BN_CTX *ctx=BN_CTX_new();
    uint8_t pubbytes[128],secret[128];int ret=-1;unsigned mdlen=0;
    /* Pin the stock Android v1 group, avoiding attacker-chosen weak DH groups. */
    BN_hex2bn(&expected,"cf5cf5c38419a724957ff5dd323b9c45c3cdd261eb740f69aa94b8bb1a5c96409153bd76b24222d03274e4725a5406092e9e82e9135c643cae98132b0d95f7d65347c68afc1e677da90e51bbab5f5cf429c291b4ba39c6b2dc5e8c7231e46aa7728e87664532cdf547be20c9a3fa8342be6e34371a27c06f7dc0edddd2f86373");
    if(!p||!g||!peer||!priv||!pub||!shared||!limit||!ctx||!expected||n||BN_cmp(p,expected)||!BN_is_word(g,2))goto done;
    if(!BN_copy(limit,p)||!BN_sub_word(limit,2)||BN_cmp(peer,BN_value_one())<=0||BN_cmp(peer,limit)>0)goto done;
    if(!BN_priv_rand(priv,256,BN_RAND_TOP_ONE,BN_RAND_BOTTOM_ANY))goto done;
    BN_set_flags(priv,BN_FLG_CONSTTIME);
    if(!BN_mod_exp_mont_consttime(pub,g,priv,p,ctx,NULL)||!BN_mod_exp_mont_consttime(shared,peer,priv,p,ctx,NULL))goto done;
    /* Java DH pads to group width; matches the stock client. */
    if(BN_bn2binpad(pub,pubbytes,128)!=128||BN_bn2binpad(shared,secret,128)!=128)goto done;
    if(!EVP_Digest(secret,sizeof(secret),b->key,&mdlen,EVP_md5(),NULL)||mdlen!=16)goto done;
    b->keyed=true;ret=bf_send(b,1,pubbytes,128);
 done:
    OPENSSL_cleanse(secret,sizeof(secret));BN_free(p);BN_free(g);BN_free(peer);BN_free(expected);BN_clear_free(priv);BN_free(pub);BN_clear_free(shared);BN_free(limit);BN_CTX_free(ctx);return ret;
}
