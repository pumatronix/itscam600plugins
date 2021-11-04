#include <string.h>
#include "camera/itscam/recebedados.h"

static unsigned int diferencaTempo(unsigned int t1, unsigned int t0)
{
    if(t1>=t0) return t1-t0;
    return (0xFFFFFFFF-t0)+t1;
}

RecebeDados::RecebeDados(int config)
{
    configuracao = config;
    bufferRecebimento = NULL;
    semaforo = CreateSemaphore(NULL,1,1,NULL);
    semaforoUnlock = CreateSemaphore(NULL,1,1,NULL);
    WaitForSingleObject(semaforo,INFINITE);
    total = 0;
    indice = 0;
    counter = 0;
    timestamp = GetTickCount();
}

RecebeDados::RecebeDados(int config, int tamanho)
{
    configuracao = config;
    bufferRecebimento = new unsigned char[tamanho];
    semaforo = CreateSemaphore(NULL,1,1,NULL);
    semaforoUnlock = CreateSemaphore(NULL,1,1,NULL);
    WaitForSingleObject(semaforo,INFINITE);
    total = tamanho;
    indice = 0;
    counter = 0;
    timestamp = GetTickCount();
}

void RecebeDados::redimensione(int tamanho)
{
    if(bufferRecebimento!=NULL) delete[] bufferRecebimento;
    bufferRecebimento = new unsigned char[tamanho];
    total = tamanho;
    indice = 0;
}

int RecebeDados::espere(int tout)
{
    return WaitForSingleObject(semaforo,tout);
}

unsigned char *RecebeDados::buffer()
{
    return bufferRecebimento;
}

int RecebeDados::capacidade()
{
    return total;
}

RecebeDados::~RecebeDados()
{
    if(bufferRecebimento!=NULL) delete[] bufferRecebimento;
    ReleaseSemaphore(semaforo,1,NULL);
    CloseHandle(semaforo);
    CloseHandle(semaforoUnlock);
}

bool RecebeDados::verifica(int config)
{
    return config==configuracao;
}

int RecebeDados::preenche(unsigned char *b, int tamanho)
{
    int y = tamanho;
    if(y>total-indice) y = total - indice;
    memcpy(bufferRecebimento+indice,b,y);
    indice += y;
    if(indice>=total) ReleaseSemaphore(semaforo,1,NULL);
    return y;
}

bool RecebeDados::preenchido()
{
    return indice>=total;
}

DWORD RecebeDados::tempoVida()
{
    return diferencaTempo(GetTickCount(),timestamp);
}

void RecebeDados::lock()
{
    WaitForSingleObject(semaforoUnlock,INFINITE);
    counter++;
    ReleaseSemaphore(semaforoUnlock,1,NULL);
}

void RecebeDados::unlock()
{
    WaitForSingleObject(semaforoUnlock,INFINITE);
    counter--;
    if(counter<=0){
        ReleaseSemaphore(semaforoUnlock,1,NULL);
        delete this;
    }
    else ReleaseSemaphore(semaforoUnlock,1,NULL);
}

FilaRecebeDados::FilaRecebeDados()
{
    semaforo = CreateSemaphore(NULL,1,1,NULL);
    semaforoEnvelopes = CreateSemaphore(NULL,1,1,NULL);
}

FilaRecebeDados::~FilaRecebeDados()
{
    ReleaseSemaphore(semaforoEnvelopes,1,NULL);
    Sleep(100);
    CloseHandle(semaforoEnvelopes);
    for(unsigned int i=0;i<vec2.size();i++){
        remova(vec2[i]);
        delete vec2[i];
    }

    ReleaseSemaphore(semaforo,1,NULL);
    Sleep(100);
    CloseHandle(semaforo);

    for(unsigned int i=0;i<vec.size();i++)
        delete vec[i];
}

RecebeDados *FilaRecebeDados::encontre(int config)
{
    RecebeDados *rd = NULL;
    WaitForSingleObject(semaforo,INFINITE);
    for(unsigned int i=0;i<vec.size();i++){
        if(vec[i]->verifica(config)){
            rd = vec[i];
            rd->lock();
            if(config>1000){
                //Segura o envelope e suas fotos
                for(unsigned int j=0;j<rd->capacidade()/(2*sizeof(RecebeDados *));j++){
                    RecebeDados *foto;
                    memcpy(&foto,rd->buffer()+(sizeof(RecebeDados *)*j),sizeof(RecebeDados *));
                    foto->lock();
                }
                vec.erase(vec.begin()+i);
            }
            break;
        }
    }
    ReleaseSemaphore(semaforo,1,NULL);
    return rd;
}

void FilaRecebeDados::unlockEnvelopes()
{
    RecebeDados *rd;
    WaitForSingleObject(semaforoEnvelopes,INFINITE);
    for(unsigned int i=0;i<vec2.size();i++){
        rd = vec2[i];
        if(rd->tempoVida()>10000){
            //Faz unlock em todos já preenchidos e com tempo de vida maior que 10 segundos
            remova(rd);
            for(unsigned int j=0;j<rd->capacidade()/(2*sizeof(RecebeDados *));j++){
                RecebeDados *foto;
                memcpy(&foto,rd->buffer()+(sizeof(RecebeDados *)*j),sizeof(RecebeDados *));
                foto->unlock();
            }
            rd->unlock();
            vec2.erase(vec2.begin()+i);
            i--;
        }
    }
    ReleaseSemaphore(semaforoEnvelopes,1,NULL);
}

void FilaRecebeDados::adicioneEnvelope(RecebeDados *dados)
{
    WaitForSingleObject(semaforoEnvelopes,INFINITE);
    vec2.push_back(dados);
    ReleaseSemaphore(semaforoEnvelopes,1,NULL);
}

void FilaRecebeDados::adicione(RecebeDados *dados)
{
    WaitForSingleObject(semaforo,INFINITE);
    vec.push_back(dados);
    ReleaseSemaphore(semaforo,1,NULL);
}

void FilaRecebeDados::remova(RecebeDados *dados)
{
    WaitForSingleObject(semaforo,INFINITE);
    for(unsigned int i=0;i<vec.size();i++){
        if(vec[i]==dados){
            vec.erase(vec.begin()+i);
            break;
        }
    }
    ReleaseSemaphore(semaforo,1,NULL);
}
