#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cassert>
#include <cstring>
#include <cinttypes>
#include <unordered_map>

#include <unistd.h>
#include <endian.h>

/********************************
*           GLOBAL MACROS       *
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
uint32_t twine_fun2(uint32_t k, uint16_t *m, unsigned mlen);
uint32_t twine_fun2_fix32(uint32_t k, uint32_t m);
uint32_t twine_fun2_fix16(uint32_t k, uint32_t m);

void question1();
void question3();
void question4();
void question5();
void question6();
void question7();
void question9();
void question10();
void question11();

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
 * Effectue une recherche de colision (recherche 2 résultats égaux
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
    uint32_t max = 2<<19;
    std::unordered_map<uint32_t, uint32_t> hashmap;
    register uint32_t i;

    // C++'s unordered_map return a std::pair, which contains the
    // index of the element, and a boolean indicating wether the
    // insertion succeeded or not. This is this boolean that we
    // are checking here.
    for (i = 0; i < max; i++){
        if (hashmap.insert({fun(k,i), i}).second == false)
            return i;
    }

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
 * Fonction twine_perm_z :
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

    // Découpages en nibbles
    for (int i = 0; i < 16; i++)
    {
        X[0][i] = (input >> (4*i)) & 0xF;
    }

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

    // Mise en forme du résultat
    for (int i = 0; i < 16; i++)
    {
        output ^= (uint64_t)X[35][i] << (4*i);
    }

    return output;
}

/*
 * Fonction twine_fun1 :
 * Chiffre un message de taille fixe via twine_perm_z
 *  @ARG
 *      - un uint32_t 'k' clé de chiffrement
 *      - un uint32_t 'm' message à chiffrer
 *  @RETURN
 *      -uint32_t 'output' message chiffré
 */
uint32_t twine_fun1(uint32_t k, uint32_t m){
    return (uint32_t) twine_perm_z(((uint64_t)k << 32) ^ m);
}

/*
 * Fonction twine_fun2 :
 * Chiffre un message de taille variable via twine_fun1
 *  @ARG
 *      - un uint32_t 'k' clé de chiffrement
 *      - un uint16_t* 'm' message à chiffrer
 *		- un unsigned 'mlen' taille du message
 *  @RETURN
 *      -uint32_t 'result' message chiffré
 */
uint32_t twine_fun2(uint32_t k, uint16_t *m, unsigned mlen){
    unsigned int i;
    uint32_t result = twine_fun1(k, (0xFFFF << 16) ^ m[0]);
    for (i = 1; i < mlen; i++){
        result = twine_fun1(k, (result << 16) ^ m[i]);
    }

    return result;
}

/*
 * Fonction twine_fun2_fix32 :
 * Chiffre un message via twine_fun2 ( en 2 partie)
 *  @ARG
 *      - un uint32_t 'k' clé de chiffrement
 *      - un uint32_t 'm' message à chiffrer
 *  @RETURN
 *      -uint32_t  message chiffré
 */
uint32_t twine_fun2_fix32(uint32_t k, uint32_t m){
    uint16_t message[2];
    message[0] = m >> 16;
    message[1] = m;
    return twine_fun2(k, message, 2);
}

/*
 * Fonction twine_fun2_fix16 :
 * Chiffre un message via twine_fun2
 *  @ARG
 *      - un uint32_t 'k' clé de chiffrement
 *      - un uint32_t 'm' message à chiffrer
 *  @RETURN
 *      -uint32_t 'output' message chiffré
 */
uint32_t twine_fun2_fix16(uint32_t k, uint32_t m){
    uint16_t message[1];
    message[0] = m;
    return twine_fun2(k, message, 1);
}

/********************************
 *            QUESTIONS          *
 *********************************/

void question1(){
    printf("\n====================\n");
    printf("==== Question 1 ====\n");

    uint64_t k[2] = {0x0706050403020100, 0x0f0e0d0c0b0a0908};
    uint64_t k2[2] = {0, 0};
    uint8_t m[15] = {
        0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
        0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe
    };
    uint8_t m2[8] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};
    uint64_t result;

    printf("==== Test 1 ====\n");
    result = siphash_2_4(k, m, 15);
    assert(result == 0xa129ca6149be45e5);
    printf("Assertion passed for siphash_2_4 - 0x%" PRIx64 "\n", result);

    printf("==== Test 2 ====\n");
    result = siphash_2_4(k, m2, 8);
    assert(result == 0x93f5f5799a932462);
    printf("Assertion passed for siphash_2_4 - 0x%" PRIx64 "\n", result);

    printf("==== Test 3 ====\n");
    result = siphash_2_4(k, NULL, 0);
    assert(result == 0x726fdb47dd0e0e31);
    printf("Assertion passed for siphash_2_4 - 0x%" PRIx64 "\n", result);

    printf("==== Test 4 ====\n");
    result = siphash_2_4(k2, NULL, 0);
    assert(result == 0x1e924b9d737700d7);
    printf("Assertion passed for siphash_2_4 - 0x%" PRIx64 "\n", result);

    printf("==== More Tests ====\n");
    result = siphash_2_4(k2, m, 15);
    printf("0x%" PRIx64 "\n", result);
    result = siphash_2_4(k2, m2, 15);
    printf("0x%" PRIx64 "\n", result);
}

void question3(){
    printf("\n====================\n");
    printf("==== Question 3 ====\n");

    uint32_t k, m, result;
    uint32_t max = 10;
    register uint32_t i;

    for (i = 1; i <= max; i++){
        printf("==== Test %d ====\n", i);
        k = 0;
        m = i;
        result = sip_hash_fix32(k, m);
        printf("sip_hash_fix32 - 0x%" PRIx32 "\n", result);
    }

    m = 0;
}

void question4(){
    printf("\n====================\n");
    printf("==== Question 4 ====\n");

    int i;

    for (i = 0; i < NB_COLL_TEST; i++){
        print_q4_result(i, coll_search(i, &sip_hash_fix32));
    }
}

void question5(){
    printf("\n====================\n");
    printf("==== Question 5 ====\n");
    puts("Please wait a few seconds, we are doing a thousand collisions...");

    float min, max, average;
    clock_t t1,t2;
    float tabTemps[1000];
    register int i, j;
    srandom(time(NULL));
    uint32_t k = (uint32_t) random(); // distinct key k

    for (i=1; i < 2; i++){
        printf("==== Test %d ====\n", i);
        for (j = 0; j < 1000; j++){
            t1 = clock();
            coll_search(k, &sip_hash_fix32);
            t2 = clock();
            tabTemps[j]= (float)(t2-t1)/CLOCKS_PER_SEC;
            k++;
        }

        min = max = average = tabTemps[0];

        for (j = 1; j < 1000; j++){
#ifdef DEBUG
            printf("t%d : %f   ",j, tabTemps[j]);
#endif

            if (min > tabTemps[j])
                min = tabTemps[j];
            if (max < tabTemps[j])
                max = tabTemps[j];
            average += tabTemps[j];
        }

        printf("temps min = %f s\n", min);
        printf("temps max = %f s\n", max);
        printf("temps moyen = %f s\n", average/1000);
    }
}

void question6(){
    printf("\n====================\n");
    printf("==== Question 6 ====\n");

    uint64_t input, result;

    printf("==== Test 1 ====\n");
    input = 0x0000000000000000ULL;
    result = twine_perm_z(input);
    assert(result == 0xc0c0c0c0c0c0c0c0);
    printf("Assertion passed for twine_perm_z - 0x%" PRIx64 "\n", result);

    printf("==== Test 2 ====\n");
    input = 0x123456789abcdef1ULL;
    result = twine_perm_z(input);
    assert(result == 0xb4e946d9ad8f7b29);
    printf("Assertion passed for twine_perm_z - 0x%" PRIx64 "\n", result);

    printf("==== Test 3 ====\n");
    input = 0xb4329ed38453aac8ULL;
    result = twine_perm_z(input);
    assert(result == 0x784f5613309457d8);
    printf("Assertion passed for twine_perm_z - 0x%" PRIx64 "\n", result);
}

void question7(){
    printf("\n====================\n");
    printf("==== Question 7 ====\n");

    uint32_t result;

    result = twine_fun1(0x00000000, 0x00000000);
    assert(result == 0xc0c0c0c0);
    printf("Assertion passed for twine_fun1 - 0x%" PRIx32 "\n", result);

    result = twine_fun1(0xcdef1234, 0xab123478);
    assert(result == 0x6465886c);
    printf("Assertion passed for twine_fun1 - 0x%" PRIx32 "\n", result);
}

void question9(){
    printf("\n====================\n");
    printf("==== Question 9 ====\n");
    puts("Please wait a few seconds, we are doing a thousand collisions...");

    float min, max, average;
    clock_t t1,t2;
    float tabTemps[1000];
    register int i, j;
    uint32_t k = 0xabcd; // fixed key

    for (i=1; i < 2; i++){
        printf("==== Test %d ====\n", i);
        for (j = 0; j < 1000; j++){
            t1 = clock();
            coll_search(k, &twine_fun1);
            t2 = clock();
            tabTemps[j]= (float)(t2-t1)/CLOCKS_PER_SEC;
        }

        min = max = average = tabTemps[0];

        for (j = 1; j < 1000; j++){
#ifdef DEBUG
            printf("t%d : %f   ",j, tabTemps[j]);
#endif

            if (min > tabTemps[j])
                min = tabTemps[j];
            if (max < tabTemps[j])
                max = tabTemps[j];
            average += tabTemps[j];
        }

        printf("temps min = %f s\n", min);
        printf("temps max = %f s\n", max);
        printf("temps moyen = %f s\n", average/1000);

        k++;
    }
}

void question10(){
    printf("\n=====================\n");
    printf("==== Question 10 ====\n");

    uint32_t result;
    uint16_t m1[1] = {0x67FC};
    uint32_t m1_16 = 0x67FC;
    uint16_t m2[2] = {0xEF12, 0x5678};
    uint32_t m2_32 = 0xEF125678;
    uint16_t m3[4] = {0xEF12, 0x5678, 0x31AA, 0x7123};

    result = twine_fun2(0x00000000, m1, 1);
    assert(result == 0xc57c8cbc);
    printf("Assertion passed for twine_fun2 - 0x%" PRIx32 "\n", result);

    result = twine_fun2(0x23AE90FF, m2, 2);
    assert(result == 0xab8e124f);
    printf("Assertion passed for twine_fun2 - 0x%" PRIx32 "\n", result);

    result = twine_fun2(0xEEEEEEEE, m3, 4);
    assert(result == 0x9941a493);
    printf("Assertion passed for twine_fun2 - 0x%" PRIx32 "\n", result);

    result = twine_fun2_fix16(0x00000000, m1_16);
    assert(result == 0xc57c8cbc);
    printf("Assertion passed for twine_fun2_fix16 - 0x%" PRIx32 "\n", result);

    result = twine_fun2_fix32(0x23AE90FF, m2_32);
    assert(result == 0xab8e124f);
    printf("Assertion passed for twine_fun2_fix32 - 0x%" PRIx32 "\n", result);
}

void question11(){
    printf("\n=====================\n");
    printf("==== Question 11 ====\n");
    puts("Please wait a few seconds, we are doing a thousand collisions...");

    puts("Gathering stats for twine_fun2_fix32...");
    float min, max, average;
    clock_t t1,t2;
    float tabTemps[1000];
    register int i, j;
    uint32_t k = 0xabcd; // fixed key

    for (i=1; i < 3; i++){
        printf("==== Test %d ====\n", i);
        for (j = 0; j < 1000; j++){
            t1 = clock();
            coll_search(k, &twine_fun2_fix32);
            t2 = clock();
            tabTemps[j]= (float)(t2-t1)/CLOCKS_PER_SEC;
        }

        min = max = average = tabTemps[0];

        for (j = 1; j < 1000; j++){
#ifdef DEBUG
            printf("t%d : %f   ",j, tabTemps[j]);
#endif

            if (min > tabTemps[j])
                min = tabTemps[j];
            if (max < tabTemps[j])
                max = tabTemps[j];
            average += tabTemps[j];
        }

        printf("temps min = %f s\n", min);
        printf("temps max = %f s\n", max);
        printf("temps moyen = %f s\n", average/1000);

        k++;
    }

    puts("Gathering stats for twine_fun2_fix16...");

    for (i=1; i < 3; i++){
        printf("==== Test %d ====\n", i);
        for (j = 0; j < 1000; j++){
            t1 = clock();
            coll_search(k, &twine_fun2_fix16);
            t2 = clock();
            tabTemps[j]= (float)(t2-t1)/CLOCKS_PER_SEC;
        }

        min = max = average = tabTemps[0];

        for (j = 1; j < 1000; j++){
#ifdef DEBUG
            printf("t%d : %f   ",j, tabTemps[j]);
#endif

            if (min > tabTemps[j])
                min = tabTemps[j];
            if (max < tabTemps[j])
                max = tabTemps[j];
            average += tabTemps[j];
        }

        printf("temps min = %f s\n", min);
        printf("temps max = %f s\n", max);
        printf("temps moyen = %f s\n", average/1000);

        k++;
    }
}

/*
 * Procedure usage :
 * Affiche l'usage des fonctions
 */
void usage(char* name){
    printf("Usage: %s <args>\n", name);
    puts("\t--all\t\tPrint all the questions, including the long one.");
    puts("\t--part1\t\tPrint first part questions (excluding question 5).");
    puts("\t--part2\t\tPrint the second part questions.");
    puts("\t--question5\tPrint question 5, which take a bit of time.");
    puts("\t--question9\tPrint question 9, which take a bit of time.");
    puts("\t--question11\tPrint question 11, which take a bit of time.");
    puts("\t--questionN\tPrint question N (N in {1,3,4,5,6,7,9,10,11}).");
    puts("\t--qN\tPrint question N (N in {1,3,4,5,6,7,9,10,11}).");
}

/********************************
 *             MAIN             *
 ********************************/

int main(int argc, char** argv){
    if (argc < 2){
        usage(argv[0]);
    } else {
        if (strcmp(argv[1], "--all") == 0){
            question1();
            question3();
            question4();
            question5();
            question6();
            question7();
            question9();
            question10();
            question11();
        } else if (strcmp(argv[1], "--part1") == 0 || strcmp(argv[1], "--p1") == 0 ){
            puts("Outputting first questions of part 1...");
            question1();
            question3();
            question4();
        } else if (strcmp(argv[1], "--part2") == 0  || strcmp(argv[1], "--p2") == 0 ){
            puts("Outputting first questions of part 2...");
            question6();
            question7();
            question10();
        } else if (strcmp(argv[1], "--question1") == 0 || strcmp(argv[1], "--q1") == 0 ){
            question1();
        } else if (strcmp(argv[1], "--question3") == 0 || strcmp(argv[1], "--q3") == 0 ){
            question3();
        } else if (strcmp(argv[1], "--question4") == 0 || strcmp(argv[1], "--q4") == 0 ){
            question4();
        } else if (strcmp(argv[1], "--question5") == 0 || strcmp(argv[1], "--q5") == 0 ){
            question5();
        } else if (strcmp(argv[1], "--question6") == 0 || strcmp(argv[1], "--q6") == 0 ){
            question6();
        } else if (strcmp(argv[1], "--question7") == 0 || strcmp(argv[1], "--q7") == 0 ){
            question7();
        } else if (strcmp(argv[1], "--question9") == 0 || strcmp(argv[1], "--q9") == 0 ){
            question9();
        } else if (strcmp(argv[1], "--question10") == 0 || strcmp(argv[1], "--q10") == 0 ){
            question10();
        } else if (strcmp(argv[1], "--question11") == 0 || strcmp(argv[1], "--q11") == 0 ){
            question11();
        } else {
            usage(argv[0]);
        }
    }

    return 0;
}
