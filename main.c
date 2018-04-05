#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cassert>
#include <cstring>
#include <cinttypes>
#include <unordered_map>
#include <time.h>
#include <unistd.h>
#include <endian.h>

/********************************
*           GLOBAL VAR          *
*********************************/

#define COMPRESION_ROUNDS 2
#define FINALIZATION_ROUNDS 4
#define RAINBOW_PATH "./rainbow"
#define NB_COLL_TEST 10

/********************************
*           PROTOTYPES          *
*********************************/

void print_internal_state(uint64_t* v);
uint64_t coll_search(uint32_t k, uint32_t (*fun)(uint32_t, uint32_t));
uint64_t rotation_shift(const uint64_t value, int shift);
uint32_t sip_hash_fix32(uint32_t k, uint32_t m);
void sipround(uint64_t v[4]);
uint64_t siphash_2_4(uint64_t k[2], uint8_t *m, unsigned mlen);
uint64_t twine_perm_z(uint64_t input);
uint32_t twine_fun1(uint32_t k, uint32_t m);

void question1();
void question3();
void question4();
void question5();
void question6();
void question7();

/********************************
*           FUNCTIONS           *
*********************************/

/*
 * Procedure print_internal_state :
 * Affiche proprement chaque uint64_t d'un tableau de 4 uint64_t
 * @ARG
 *       - un tableau avec 4 uint64_t 'v'
 */
void print_internal_state(uint64_t* v){
    printf("0x%" PRIx64 "   ", v[0]);
    printf("0x%" PRIx64 "   ", v[1]);
    printf("0x%" PRIx64 "   ", v[2]);
    printf("0x%" PRIx64 "\n", v[3]);
}

/*
 * Fonction rotation_shift :
 * Effectue une rotation d'un chiffre binaire en réinjectant le
 *  bit de poid fort en bit de poid faible (rotation left)
 *  @ARG
 *      - un uint64_t 'value' sur le quel effectuer la rotation
 *      - un int 'shift' pour le nombre de rotation a effectuer
 *  @RETURN
 *      -uint64_t rotate on left
 */
uint64_t rotation_shift(const uint64_t value, int shift){
    if ((shift &= sizeof(value)*8 - 1) == 0)
        return value;
    return (value << shift) | (value >> (sizeof(value)*8 - shift));
}

/*
 * Procedure sipround :
 *  Applique le réseau ARX de SipRound (detail sur doc SipHash avec cours)
 *  @ARG
 *      - un tableau de 4 uint64_t 'v' sur les quelles effectuer le
 *          sipround
 */
void sipround(uint64_t v[4]){
    v[0] += v[1];
    v[2] += v[3];

    v[1] = rotation_shift(v[1], 13);
    v[3] = rotation_shift(v[3], 16);

    v[1] ^= v[0];
    v[3] ^= v[2];

    v[0] = rotation_shift(v[0], 32);

    v[2] += v[1];
    v[0] += v[3];

    v[1] = rotation_shift(v[1], 17);
    v[3] = rotation_shift(v[3], 21);

    v[1] ^= v[2];
    v[3] ^= v[0];

    v[2] = rotation_shift(v[2], 32);
}

/*
 * Procedure siphash_2_4 :
 * Applique la fonction SipHash sur un message
 *  @ARG
 *      - un tableau de 2 uint64_t 'k' la cle de 128 bits (2 fois 64)
 *      - un pointeur uint8_t 'm' pour le message
 *      - un unsigned 'mlen' pour la taille du message m
 *  @RETURN
 *      - un uint64_t correspondant au hash
 */
uint64_t siphash_2_4(uint64_t k[2], uint8_t *m, unsigned mlen){
    uint64_t i, j;
    k[0] = htole64(k[0]);
    k[1] = htole64(k[1]);

    uint64_t v[4];
    // Init
    v[0] = k[0] ^ 0x736f6d6570736575;
    v[1] = k[1] ^ 0x646f72616e646f6d;
    v[2] = k[0] ^ 0x6c7967656e657261;
    v[3] = k[1] ^ 0x7465646279746573;

    ////////////// PARSING WORD ////////////
    size_t w = ceil((mlen + 1) / 8);
    unsigned padding = 0;
    if (mlen % 8 == 0) padding++;
    uint64_t mi[w+padding];
    memset(mi, 0, (w + 1) * sizeof(uint64_t));

    for (i = 0; i < w; i++){
        memcpy(mi+i, m+(i*8), 8);
    }

    // Fin du mot
    if (padding){
        mi[w] ^= (uint64_t)(mlen % 256) << 56;
    } else {
        mi[w-1] ^= (uint64_t)(mlen % 256) << 56;
    }

    /////////////////////////////////////////////////
    // Compresion rounds
    for (i = 0; i < w + padding; i++){
        v[3] ^= mi[i];
        for (j = 0; j < COMPRESION_ROUNDS; j++)
            sipround(v);
        v[0] ^= mi[i];
    }

    // Finalize
    v[2] ^= 0xff;
    for (i = 0; i < FINALIZATION_ROUNDS; i++)
        sipround(v);
    uint64_t result = v[0] ^ v[1] ^ v[2] ^ v[3];

    return result;
}

/*
 * Fonction sip_hash_fix32 :
 * Effectue un siphash_2_4 avec une cle de 32 bits sur un message,
 * et renvoie les 32 bits de poids fort
 *  @ARG
 *      - un uint32_t 'k' cle de 32 bits
 *      - un uint32_t 'm' de 32 bits
 *  @RETURN
 *      -uint32_t
 */
uint32_t sip_hash_fix32(uint32_t k, uint32_t m){
    uint64_t k64[2];
    memset(k64, 0, 2 * sizeof(uint64_t));
    uint8_t m8[4];
    memset(m8, 0, 2 * sizeof(uint8_t));

    // Putting k 4 times into k64
    int i;
    for (i = 0; i < 4; i++){
        k64[i/2] ^= (uint64_t)k << 32 *(i % 2);
    }

    for (i = 0; i < 4; i++){
        m8[i] ^= (uint8_t) (m >> 8 * i);
    }

    return siphash_2_4(k64, m8, 32) >> 32;
}

/*
 * Fonction coll_search :
 * Effectue une recherche de colission (recherche 2 résultats égaux
 *	pour 2 indices de la function fun avec une clé 'k')
 *  @ARG
 *      - un uint32_t 'k' cle de 32 bits
 *      - un pointeur uint32_t 'fun' pour une fonction prenant en
 *          parametre 2 uint32_t
 *  @RETURN
 *      -uint64_t 'i' 2ème indice pour le quel la fonction fun à
 *			le même résultat.
 */
uint64_t coll_search(uint32_t k, uint32_t (*fun)(uint32_t, uint32_t)){
    uint32_t max;

    max = 2<<19;

    std::unordered_map<uint32_t, uint32_t> hashmap;

    //puts("Computing rainbow table & checking for collisions...");
    for (uint32_t i = 0; i < max; i++){
        std::pair<std::unordered_map<uint32_t, uint32_t>::iterator, bool > result;
        result = hashmap.insert({fun(i,k), i});
        if (result.second == false)
            return i;
    }
    //puts("Done. No collision found.");

    return 0;
}

/*
 * Procedure print_q4_result :
 * Affiche proprement un hexadecimal (pour la question 4)
 *  @ARG
 *      - un int 'i' indice de l'hexadecimal à afficher
 *      - un uint32_t 'result' hexadecimal à afficher
 */
void print_q4_result(int i, uint32_t result){
    printf("Results for %02d - 0x%" PRIx32 "\n", i, result);
}


/*
 * Procedure twine_perm_z :
 * Chiffre un message hexadecimal via twine
 *  @ARG
 *      - un uint64_t 'input' message à chiffrer
 *  @RETURN
 *      -uint64_t 'output' message chiffré
 */
uint64_t twine_perm_z(uint64_t input){
    uint8_t X[36][16];
    uint64_t output = 0;
    uint8_t SBox[16], pi[16];

    for (int i = 0; i < 16; i++)
    {
        X[0][i] = (input >> (4*i)) & 0xF;
    }

    // Tables de permutation
    SBox[0] = 0xc;    SBox[1] = 0x0;    SBox[2] = 0xf;
    SBox[3] = 0xa;    SBox[4] = 0x2;    SBox[5] = 0xb;
    SBox[6] = 0x9;    SBox[7] = 0x5;    SBox[8] = 0x8;
    SBox[9] = 0x3;    SBox[10] = 0xd;   SBox[11] = 0x7;
    SBox[12] = 0x1;   SBox[13] = 0xe;   SBox[14] = 0x6;
    SBox[15] = 0x4;

    // Tables de permutation
    pi[0] = 5;    pi[1] = 0;    pi[2] = 1;
    pi[3] = 4;    pi[4] = 7;    pi[5] = 12;
    pi[6] = 3;    pi[7] = 8;    pi[8] = 13;
    pi[9] = 6;    pi[10] = 9;   pi[11] = 2;
    pi[12] = 15;  pi[13] = 10;  pi[14] = 11;
    pi[15] = 14;

    for (int i = 0; i < 35; i++){
        for (int j = 0; j < 8; j++){
            X[i][2*j + 1] = SBox[X[i][2 * j]] ^ X[i][2*j+1];
        }
        for (int h = 0; h < 16; h++){
            X[i+1][pi[h]] = X[i][h];
        }
    }
    for (int j = 0; j < 8; j++){
        X[35][2*j+1] = SBox[X[35][2*j]] ^ X[35][2*j+1];
    }

    for (int i = 0; i < 16; i++)
    {
        output ^= (uint64_t)X[35][i] << (4*i);
    }

    return output;
}

uint32_t twine_fun1(uint32_t k, uint32_t m){
    uint64_t input = 0;
    input ^= ((uint64_t)k << 32) ^ m;

    return (uint32_t) twine_perm_z(input);
}

/********************************
 *            QUESTIONS          *
 *********************************/

void question1(){
    // TODO: peut-etre un probleme avec little/big endian
    printf("\n=====================\n");
    printf("===== Question 1 ====\n");

    uint64_t k[2] = {0x0706050403020100, 0x0f0e0d0c0b0a0908};
    uint64_t k2[2] = {0, 0};
    uint8_t m[15] = {
        0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
        0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe
    };
    uint8_t m2[8] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};
    uint64_t result;

    printf("===== Test 1 ====\n");
    result = siphash_2_4(k, m, 15);
    assert(result == 0xa129ca6149be45e5);
    printf("OK - 0x%" PRIx64 "\n", result);

    printf("===== Test 2 ====\n");
    result = siphash_2_4(k, m2, 8);
    assert(result == 0x93f5f5799a932462);
    printf("OK - 0x%" PRIx64 "\n", result);

    printf("===== Test 3 ====\n");
    result = siphash_2_4(k, NULL, 0);
    assert(result == 0x726fdb47dd0e0e31);
    printf("OK - 0x%" PRIx64 "\n", result);

    printf("===== Test 4 ====\n");
    result = siphash_2_4(k2, NULL, 0);
    assert(result == 0x1e924b9d737700d7);
    printf("OK - 0x%" PRIx64 "\n", result);

    printf("===== More Tests ====\n");
    result = siphash_2_4(k2, m, 15);
    printf("0x%" PRIx64 "\n", result);
    result = siphash_2_4(k2, m2, 15);
    printf("0x%" PRIx64 "\n", result);
}

void question3(){
    printf("\n=====================\n");
    printf("===== Question 3 ====\n");

    uint32_t k, m, result;
    uint32_t i;
    uint32_t max = 1000000;

    printf("SipHash32\n");
    for (i = 1; i < max; i*=10){
        printf("===== Test %d ====\n", i);
        k = 0;
        m = i;
        result = sip_hash_fix32(k, m);
        printf("OK - 0x%" PRIx32 "\n", result);
    }

    m = 0;

    printf("===== Test %d ====\n", i + 1);
    k = 0x03020100;
    result = sip_hash_fix32(k, m);
    printf("OK - 0x%" PRIx32 "\n", result);
    printf("===== Test %d ====\n", i + 2);
    k = 0x07060504;
    result = sip_hash_fix32(k, m);
    printf("OK - 0x%" PRIx32 "\n", result);
    printf("===== Test %d ====\n", i + 3);
    k = 0x0b0a0908;
    result = sip_hash_fix32(k, m);
    printf("OK - 0x%" PRIx32 "\n", result);
    printf("===== Test %d ====\n", i + 4);
    k = 0x0f0e0d0c;
    result = sip_hash_fix32(k, m);
    printf("OK - 0x%" PRIx32 "\n", result);

    /*
       printf("===== Test 4 ====\n");
       k = 0x03020100;
       m = 0x03020100;
       result = sip_hash_fix32(k, m);
       printf("OK - 0x%" PRIx32 "\n", result);
       */
}

void question4(){
    printf("\n=====================\n");
    printf("===== Question 4 ====\n");

    int i;

    for (i = 0; i < NB_COLL_TEST; i++){
        print_q4_result(i, coll_search(i, &sip_hash_fix32));
    }
}

void question5(){
    printf("\n=====================\n");
    printf("===== Question 5 ====\n");

    float min, max, moyenne;
    clock_t t1,t2;
    float tabTemps[1000];
    int i, k;
    srandom(time(0));
    long int rand = random();

    for (i=1; i < 3; i++){
        printf("===== Test %d ====\n", i);
        for (k = 0; k < 1000; k++){
            t1 = clock();
            coll_search(rand, &sip_hash_fix32);
            t2 = clock();
            tabTemps[k]= (float)(t2-t1)/CLOCKS_PER_SEC;
            rand++;
        }

        min = max = moyenne = tabTemps[0];

        for (k = 1; k < 1000; k++){
	#ifdef DEBUG
            printf("t%d : %f   ",k, tabTemps[k]);
	#endif
            if (min > tabTemps[k])
                min = tabTemps[k];
            if (max < tabTemps[k])
                max = tabTemps[k];
            moyenne += tabTemps[k];
        }

        printf("temps min = %f s\n", min);
        printf("temps max = %f s\n", max);
        printf("temps moyen = %f s\n", moyenne/1000);
    }
}

void question6(){
    printf("\n=====================\n");
    printf("===== Question 6 ====\n");

    uint64_t input, result;

    printf("===== Test 1 ====\n");
    input = 0x0000000000000000ULL;
    result = twine_perm_z(input);
    assert(result == 0xc0c0c0c0c0c0c0c0);
    printf("OK - 0x%" PRIx64 "\n", result);

    printf("===== Test 2 ====\n");
    input = 0x123456789abcdef1ULL;
    result = twine_perm_z(input);
    assert(result == 0xb4e946d9ad8f7b29);
    printf("OK - 0x%" PRIx64 "\n", result);

    printf("===== Test 3 ====\n");
    input = 0xb4329ed38453aac8ULL;
    result = twine_perm_z(input);
    assert(result == 0x784f5613309457d8);
    printf("OK - 0x%" PRIx64 "\n", result);
}

void question7(){
    printf("\n=====================\n");
    printf("===== Question 7 ====\n");

    uint32_t result;

    result = twine_fun1(0x00000000, 0x00000000);
    assert(result == 0xc0c0c0c0);
    printf("OK - 0x%" PRIx32 "\n", result);

    result = twine_fun1(0xcdef1234, 0xab123478);
    assert(result == 0x6465886c);
    printf("OK - 0x%" PRIx32 "\n", result);
}

/********************************
 *             MAIN              *
 *********************************/

int main(int argc, char** argv){
    //question1();
    //question3();
    //question4();
    //question5(); // Long ...
    question6();
    question7();

    return 1;
}
