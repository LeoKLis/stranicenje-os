// Logicka adresa ima 16 bitova, prvih 6 bitova je za pomak u okviru, sljedeca 4
// bita su za indeks okvira, ostalih 6 bitova se ne koriste Okvir je velicine 64
// okteta i ima ih max 16

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_RESET "\x1b[0m"

class Mem_okvir {
       public:
        u_int8_t polje[64];
        bool seKoristi;
        bool naDisku;

        Mem_okvir()
        {
                for (int i = 0; i < 64; i++) {
                        polje[i] = 0;
                }
                seKoristi = false;
                naDisku = false;
        }

        void kopiraj(Mem_okvir &original)
        {
                for (int i = 0; i < 64; i++) {
                        polje[i] = original.polje[i];
                }
                seKoristi = original.seKoristi;
                naDisku = original.naDisku;
        }
};

int vrijeme = 0;
int broj_okvira = 6;
Mem_okvir *memorija;
Mem_okvir *disk;

class Tablica_stranicenja {
       public:
        Mem_okvir **fizicka_adresa;
        bool *bit_prisutnosti;
        int *metapodatak;

        Tablica_stranicenja()
        {
                fizicka_adresa = (Mem_okvir **)malloc(16 * sizeof(Mem_okvir *));
                bit_prisutnosti = (bool *)malloc(16 * sizeof(bool));
                metapodatak = (int *)malloc(16 * sizeof(int));
                for (int i = 0; i < 16; i++) {
                        fizicka_adresa[i] = nullptr;
                        bit_prisutnosti[i] = 0;
                        metapodatak[i] = 0;
                }
        }

        Mem_okvir *dohvati_fizicku_adresu(int indeks)
        {
                if (bit_prisutnosti[indeks] == 0) {
                        printf("\tPromasaj!\n");
                        // Ako se nade slobodno mjesto u memoriji, postavi
                        // pokazivac iz tablice tamo
                        for (int i = 0; i < broj_okvira; i++) {
                                if (!memorija[i].seKoristi) {
                                        printf(
                                            "\tU tablicu stranicenja se kopira "
                                            "memorija iz bloka: %p\n",
                                            fizicka_adresa[indeks]);
                                        memorija[i].kopiraj(
                                            *fizicka_adresa[indeks]);
                                        fizicka_adresa[indeks] = &memorija[i];
                                        fizicka_adresa[indeks]->seKoristi =
                                            true;
                                        bit_prisutnosti[indeks] = 1;
                                        break;
                                }
                        }
                        // Ako u prethodnom koraku nije nadjeno mjesto...
                        if (bit_prisutnosti[indeks] == 0) {
                                int min_index;
                                int min_metapodatak = 32;
                                // Nadji podatak sa najmanjim LRU-om
                                for (int i = 0; i < broj_okvira; i++) {
                                        if (metapodatak[i] < min_metapodatak &&
                                            bit_prisutnosti[i] == 1) {
                                                min_index = i;
                                                min_metapodatak =
                                                    metapodatak[i];
                                        }
                                }
                                // Nadji slobodno mjesto u memoriji, kopiraj
                                // podatke iz memorije (na koje je pokazivao
                                // najranije koristen element tabl.) na disk i
                                // postavi pokazivac najranijeg elementa na to
                                // mjesto na disku
                                for (int i = 0; i < 1000; i++) {
                                        if (!disk[i].naDisku) {
                                                disk[i].kopiraj(
                                                    *fizicka_adresa[min_index]);
                                                printf(
                                                    "\tU tablicu stranicenja "
                                                    "se kopira memorija iz "
                                                    "bloka: %p\n",
                                                    fizicka_adresa[indeks]);
                                                printf(
                                                    "\tNa disk se kopira "
                                                    "memorija iz bloka tablice "
                                                    "stranicenja: %p\n",
                                                    fizicka_adresa[min_index]);
                                                fizicka_adresa[min_index]
                                                    ->kopiraj(*fizicka_adresa
                                                                  [indeks]);
                                                fizicka_adresa[indeks] =
                                                    fizicka_adresa[min_index];
                                                fizicka_adresa[min_index] =
                                                    &disk[i];
                                                break;
                                        }
                                }
                                bit_prisutnosti[indeks] = 1;
                                bit_prisutnosti[min_index] = 0;
                        }
                }
                metapodatak[indeks] = vrijeme % 30;
                return fizicka_adresa[indeks];
        }
};

Tablica_stranicenja tablica[2];

u_int8_t dohvati_sadrzaj(int pid, u_int16_t logicka_adresa)
{
        int log_indeks =
            (logicka_adresa >> 6) & 0b1111;  // Dohvati sadrzaj bitova [6 - 10]
        int log_pomak = logicka_adresa & 0b111111;
        Mem_okvir *fizicka_adresa_okvira =
            tablica[pid].dohvati_fizicku_adresu(log_indeks);
        printf("Broj okvira u tablici stranicenja: %d\n", log_indeks);
        printf("Fizicka adresa memorijskog bloka: %p\n",
               fizicka_adresa_okvira->polje);
        printf("Pomak u memorijskom bloku: %d (0x%x)\n", log_pomak, log_pomak);
        u_int8_t sadrzaj = fizicka_adresa_okvira->polje[log_pomak];
        return sadrzaj;
}

void zapisi_sadrzaj(int pid, u_int16_t logicka_adresa, u_int8_t sadrzaj)
{
        int log_indeks = (logicka_adresa >> 6) & 0b1111;
        int log_pomak = logicka_adresa & 0b111111;
        Mem_okvir *fizicka_adresa_okvira =
            tablica[pid].dohvati_fizicku_adresu(log_indeks);
        fizicka_adresa_okvira->polje[log_pomak] = sadrzaj;
}

void log_memorije(Tablica_stranicenja *tablica)
{
        for (int i = 0; i < 16; i++) {
                if (tablica[0].fizicka_adresa[i]->seKoristi) {
                        printf(ANSI_COLOR_BLUE "%d: Koristi se" ANSI_COLOR_RESET
                                               "\n",
                               i);
                }
                else {
                        printf(ANSI_COLOR_BLUE "%d: Prazno" ANSI_COLOR_RESET
                                               "\n",
                               i);
                }
        }
        for (int i = 0; i < 16; i++) {
                if (tablica[1].fizicka_adresa[i]->seKoristi) {
                        printf(ANSI_COLOR_GREEN
                               "%d: Koristi se" ANSI_COLOR_RESET "\n",
                               i);
                }
                else {
                        printf(ANSI_COLOR_GREEN "%d: Prazno" ANSI_COLOR_RESET
                                                "\n",
                               i);
                }
        }
}

int main()
{
        memorija = (Mem_okvir *)malloc(sizeof(Mem_okvir) * broj_okvira);
        disk = (Mem_okvir *)malloc(sizeof(Mem_okvir) * 1000);

        for (int i = 0; i < 16; i++) {
                tablica[0].fizicka_adresa[i] = &disk[i];
                disk[i].naDisku = true;
        }
        for (int i = 0; i < 16; i++) {
                tablica[1].fizicka_adresa[i] = &disk[16 + i];
                disk[16 + i].naDisku = true;
        }

        __uint16_t logicka_adresa = 0x1fe;
        u_int8_t sadrzaj;
        while (1) {
                // Proces 1
                printf(ANSI_COLOR_BLUE "Proces 1 (t: %d)" ANSI_COLOR_RESET "\n",
                       vrijeme);
                // logicka_adresa = (rand() % 1024) & 0x3fe; // 2^10 adresa (jer
                // se koristi samo prvih 10 bitova)
                printf("Logicka adresa: 0x%x\n", logicka_adresa);
                sadrzaj = dohvati_sadrzaj(0, logicka_adresa);
                printf("Sadrzaj na memorijskoj lokaciji: %d\n", sadrzaj);
                sadrzaj += 1;
                zapisi_sadrzaj(0, logicka_adresa, sadrzaj);
                vrijeme = (vrijeme + 1) % 31;
                sleep(1);

                printf("\n======================\n\n");

                // Proces 2
                printf(ANSI_COLOR_GREEN "Proces 2 (t: %d)" ANSI_COLOR_RESET
                                        "\n",
                       vrijeme);
                // logicka_adresa = (rand() % 1024) & 0x3fe;
                printf("Logicka adresa: 0x%x\n", logicka_adresa);
                sadrzaj = dohvati_sadrzaj(1, logicka_adresa);
                printf("Sadrzaj na memorijskoj lokaciji: %d\n", sadrzaj);
                sadrzaj += 1;
                zapisi_sadrzaj(1, logicka_adresa, sadrzaj);
                vrijeme = (vrijeme + 1) % 31;
                sleep(1);

                // log_memorije(tablica);

                printf("\n======================\n\n");
        }

        return 0;
}