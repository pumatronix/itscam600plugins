#ifndef RECEBEDADOS_H
#define RECEBEDADOS_H

#include "camera/itscam/compat_pumatronix.h"
#include <vector>

using namespace std;

class RecebeDados
{
private:
    int indice;
    int total;
    volatile int counter;
    HANDLE semaforo;
    HANDLE semaforoUnlock;
    unsigned char *bufferRecebimento;
    int configuracao;
    DWORD timestamp;
public:
    RecebeDados(int config);
    RecebeDados(int config, int tamanho);
    void redimensione(int tamanho);
    ~RecebeDados();
    bool verifica(int config);
    int preenche(unsigned char *b, int tamanho);
    bool preenchido();
    int espere(int tout);
    int capacidade();
    unsigned char *buffer();
    void lock();
    void unlock();
    DWORD tempoVida();
};

class FilaRecebeDados
{
private:
    vector<RecebeDados*> vec;
    vector<RecebeDados*> vec2;
    HANDLE semaforo;
    HANDLE semaforoEnvelopes;
public:
    FilaRecebeDados();
    ~FilaRecebeDados();
    RecebeDados *encontre(int config);
    void adicione(RecebeDados *dados);
    void adicioneEnvelope(RecebeDados *dados);
    void remova(RecebeDados *dados);
    void unlockEnvelopes();
};

#endif
