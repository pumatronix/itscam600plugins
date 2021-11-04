#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "camera/jlib/jdebug/jdebug.h"

#include "camera/itscam/itscam300.h"
#include "camera/itscam/protocolo.h"

#define DEBUG(lv,fmt,...)   DEBUG_PRINT_TAG("LOG",lv,fmt,##__VA_ARGS__)
#define DBDEBUG             DEBUG_LV_INFO

void Itscam300::log(const char *s)
{
    if(strlen(arqLog)>5){
        SYSTEMTIME lt;
        GetLocalTime(&lt);
        FILE *st = fopen(arqLog,"at");
        if(st!=NULL){
            fprintf(st,"%d/%d/%d %02d:%02d:%02d %s\n",lt.wDay,lt.wMonth,lt.wYear,lt.wHour,lt.wMinute,lt.wSecond,s);
            fclose(st);
        }
    }
}

static unsigned int diferencaTempo(unsigned int t2, unsigned int t1)
{
    if(t2>=t1) return t2 - t1;
    else return (0xFFFFFFFF-t1)+1+t2;
}

void *mainThread(void* pParams)
{
    Itscam300 *a = (Itscam300 *) pParams;
    a->itscamThread();
    pthread_exit(NULL);
}

void *keepAliveThread(void* pParams)
{
    Itscam300 *a = (Itscam300 *) pParams;
    a->keepAlive();
    pthread_exit(NULL);
}

void Itscam300::keepAlive()
{
    unsigned char comando[] = {
        CABECALHO, REQ_CONTEXTO, 0, 0
    };
    int crc, count=1200;

    crc = updateCrc(comando,2);
    comando[2] = crc&0xFF;
    comando[3] = (crc>>8)&0xFF;

    DEBUG(DBDEBUG, "Espera camera OK");
    while(status!=CAMERA_OK){
        Sleep(10);
    }

    DEBUG(DBDEBUG, "Camera OK");
    while(status == CAMERA_OK){
        DEBUG(DBDEBUG, "inicio do while");
        if(count>=1200){
            reqConfig(REQ_CONTEXTO,NULL,0,0,1);
            if(status==OBJECT_DESTROYED) return;
            reqConfig(SETA_MAC,macItscam,6,0,1);
            if(status==OBJECT_DESTROYED) return;
            count = 0;
        }
        if(count%20==19){
            rd->unlockEnvelopes();
        }
        Sleep(50);
        count++;
        DEBUG(DBDEBUG, "count: %d", count);
    }
    DEBUG(DBDEBUG, "exit");
}

//Metodos para calculo do CRC
int Itscam300::updateCrc(unsigned char *c, int numBytes)
{
    int tmp, short_c;
    int i,crc=0;
    for (i = 0; i < numBytes; i++){
        short_c  = 0x00ff & c[i];
        tmp = ((crc >> 8) ^ short_c) & 0x00FF;
        crc = (crc << 8) ^ crcTab[tmp];
    }
    return crc & 0xFFFF;
}

void Itscam300::initCrcTab()
{
    int i,c,crc,j;
    for(i=0;i<256;i++){
        c = 256*i;
        crc = 0;
        for(j=0;j<8;j++){
            if( ((crc ^ c) & 0x8000) != 0 ){
                crc <<= 1;
                crc ^= 0x1021;
            }
            else{
                crc <<= 1;
            }
            crc &= 0xFFFF;
            c <<= 1;
        }
        crcTab[i] = crc;
    }
}

/* Le o status da camera */
int Itscam300::leStatus()
{
    return -status;
}

Itscam300 *Itscam300::ids[]={
    0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
};

int Itscam300::criarItscam300(const char *ip, int timeout)
{
    static HANDLE semaf = NULL;
    if(semaf==NULL){
        semaf = CreateSemaphore(NULL,1,1,NULL);
    }
    WaitForSingleObject(semaf,INFINITE);
    for(int i=1;i<=64;i++){
        if(ids[i]==NULL){
            ids[i] = new Itscam300(ip, timeout);
            ReleaseSemaphore(semaf,1,NULL);
            return i;
        }
    }
    ReleaseSemaphore(semaf,1,NULL);
    return 0;
}

Itscam300::~Itscam300()
{
    status = OBJECT_DESTROYED;
    for(size_t i=0;i<sizeof(ids)/sizeof(Itscam300*);i++)
        if(ids[i]==this) ids[i] = NULL;
    //estado = 0;
    DEBUG(DBDEBUG, "libera semaforos");
    ReleaseSemaphore(semaforoFotos,1,NULL);
    ReleaseSemaphore(semaforo,1,NULL);
    ReleaseSemaphore(semSlots,1,NULL);

    DEBUG(DBDEBUG, "Dorme 100ms");
    Sleep(100);

    DEBUG(DBDEBUG, "Fecha os handles");
    CloseHandle(semaforoFotos);
    CloseHandle(semaforo);
    CloseHandle(semSlots);

    delete rd;

    DEBUG(DBDEBUG, "fecha o socket, fd %x", sock);
    if(sock!=INVALID_SOCKET) closesocket(sock);
    DEBUG(DBDEBUG, "Fechou o socket");

    Sleep(100);
    DEBUG(DBDEBUG, "espera a thread1");
    if(thread1!=NULL){
        pthread_join(*thread1,NULL);
        free(thread1);
    }
    DEBUG(DBDEBUG, "espera a thread2");
    if(thread2!=NULL){
        pthread_join(*thread2,NULL);
        free(thread2);
    }
    DEBUG(DBDEBUG, "Exit");
}

Itscam300::Itscam300(const char *ip, int timeout, int port)
{
    char porta[10];
    int i, j, k;
    bool alpha = false;
    struct sockaddr_in host_addr;
    fd_set FdSet;

    macItscam[0] = 0x00;
    macItscam[1] = 0x50;
    macItscam[2] = 0xC2;
    macItscam[3] = 0x8C;
    macItscam[4] = 0x8F;
    macItscam[5] = 0xFE;

    arqLog[0] = '\0';
    estado = 0;
    tempoLaco = 0;
    tamanhoPacote = 0;
    thread1 = NULL;
    thread2 = NULL;
    memset(slots,0,sizeof(slots));

    DEBUG(DBDEBUG,  "Vai criar fila para recepcao de dados");
    rd = new FilaRecebeDados();
    timeoutIO = INFINITE;
    initCrcTab();
    config = NULL;

    DEBUG(DBDEBUG, "Cria um socket TCP");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    DEBUG(DBDEBUG, "valor do socket: %x", sock);
    if(sock==INVALID_SOCKET){
        DEBUG(DBDEBUG, "erro na criacao do socket");
        status = SOCKET_CREATION_ERROR;
        return;
    }

    DEBUG(DBDEBUG, "Cria semaforos");
    semaforo = CreateSemaphore(NULL,1,1,NULL);
    semaforoFotos = CreateSemaphore(NULL,1,1,NULL);
    semSlots = CreateSemaphore(NULL,1,1,NULL);

    memset(&host_addr, 0, sizeof (host_addr));
    host_addr.sin_family = AF_INET;
    for(i=0,j=-1;ip[i];i++){
        if(ip[i]==':') j = 0;
        else if(j>=0 && j<5 && (ip[i]>='0' && ip[i]<='9')) porta[j++] = ip[i];
    }
    if(j>0){
        porta[j] = '\0';
        host_addr.sin_port = htons(atoi(porta));
        char *ip2 = new char[strlen(ip)];
        memcpy(ip2,ip,strlen(ip)-j-1);
        ip2[strlen(ip)-j-1] = '\0';
        for(i=0;ip2[i];i++)
            if(!(ip2[i]=='.' || (ip2[i]>='0' && ip2[i]<='9'))) alpha = true;
        if(alpha){
            struct hostent *remoteHost;
            remoteHost = gethostbyname(ip2);
            if(remoteHost==NULL || remoteHost->h_addr_list[0]==0){
                status = UNABLE_TO_CONNECT;
                delete[] ip2;
                return;
            }
            host_addr.sin_addr.s_addr = *(unsigned long *) remoteHost->h_addr_list[0];
        }
        else host_addr.sin_addr.s_addr = inet_addr(ip2);
        delete[] ip2;
    }
    else{
        for(i=0;ip[i];i++)
            if(!(ip[i]=='.' || (ip[i]>='0' && ip[i]<='9'))) alpha = true;
        if(alpha){
            struct hostent *remoteHost;
            remoteHost = gethostbyname(ip);
            if(remoteHost==NULL || remoteHost->h_addr_list[0]==0){
                status = UNABLE_TO_CONNECT;
                return;
            }
            host_addr.sin_addr.s_addr = *(unsigned long *) remoteHost->h_addr_list[0];
        }
        else host_addr.sin_addr.s_addr = inet_addr(ip);
        host_addr.sin_port = htons (port);
    }
    if(timeout>0)
    {
        //Modo non-blocking
        if (-1 == (k = fcntl(sock, F_GETFL, 0))) k = 0;
        fcntl(sock, F_SETFL, k | O_NONBLOCK);
    }

    DEBUG(DBDEBUG, "Conecta a camera...");
    if(connect(sock,(struct sockaddr *)&host_addr,sizeof(struct sockaddr))==-1){
        DEBUG(DBDEBUG, "Erro de conexao");
        if(timeout>0){
            DEBUG(DBDEBUG, "Com timeout...");
            if(true)
            {
                struct timeval Time;
                DEBUG(DBDEBUG, "Espera o timeout especificado");
                FD_ZERO(&FdSet);
                FD_SET(sock, &FdSet);

                memset(&Time,0,sizeof(Time));
                Time.tv_sec  = timeout;
                unsigned char dm = 0;
                if(write(sock, &dm, 1) < 0)
                    DEBUG(DBDEBUG, "socket write error");
                if(select(sock+1, NULL, &FdSet, NULL, &Time)<=0){
                    DEBUG(DBDEBUG, "configura status to UNABLE_TO_CONNECT");
                    status = UNABLE_TO_CONNECT;
                    return;
                }
            }
            else{
                status = UNABLE_TO_CONNECT;
                return;
            }
        }
        else{
            DEBUG(DBDEBUG, "Sem timeout");
            status = UNABLE_TO_CONNECT;
            return;
        }
    }

    if(timeout>0){
        DEBUG(DBDEBUG, "Modo blocking");
        if (-1 == (k = fcntl(sock, F_GETFL, 0))) k = 0;
        fcntl(sock, F_SETFL, k & ~O_NONBLOCK);
    }
    DEBUG(DBDEBUG, "Timeout do socket");

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

    DEBUG(DBDEBUG, "Cria a thread");
    status = CREATING_THREAD;
    DWORD dummy;
    if((thread1=CreateThread(NULL,4096,keepAliveThread,this,0,&dummy))==0) return;
    if((thread2=CreateThread(NULL,4096,mainThread,this,0,&dummy))==0) return;
    do{
        DEBUG(DBDEBUG, "Camera Status: %d", status);
        Sleep(100);
    } while(status!=CAMERA_OK);

    numeroFotos     = leNumeroFotos();
    numeroFotosIO   = leNumeroFotosIO();
    qualidadeFotoIO = leQualidadeFotoIO();
    formatoFotoIO   = leFormatoFotoIO();

    DEBUG(DBDEBUG, "numeroFotos %d", numeroFotos);
    DEBUG(DBDEBUG, "numeroFotosIO %d", numeroFotosIO);
    DEBUG(DBDEBUG, "qualidadeFotoIO %d", qualidadeFotoIO);
    DEBUG(DBDEBUG, "formatoFotoIO %d", formatoFotoIO);
    DEBUG(DBDEBUG, "Exit");
}
void Itscam300::itscamThread()
{
    unsigned char buf[4096];
    int len,count=0;

    status = CAMERA_OK;

    DEBUG(DBDEBUG, "Inicio while da thread");
    while(status == CAMERA_OK) {
        if(sock==INVALID_SOCKET) {
            DEBUG(DBDEBUG, "Sock invalido");
            len = 0;
        }
        else {
            DEBUG(DBDEBUG, "chama RECV");
            len = recv(sock,(char *)buf,4096,0);
        }

        if(status==OBJECT_DESTROYED) {
            DEBUG(DBDEBUG, "Destroi o objeto");
            len = 0;
        }

        if (len<=0) {
            count++; //Erro ... nao recebeu nenhum dado!
        }
        else {
            DEBUG(DBDEBUG, "Chama processa dados");
            processaDados(buf,len);
            count = 0;
        }

        DEBUG(DBDEBUG, "len: %d, count: %d", len, count);

        if(count>=18) break; //90s sem receber dado!
    }

    DEBUG(DBDEBUG, "Finalizando o thread");
    if(status!=OBJECT_DESTROYED) status = LOST_CONNECTION;
    DEBUG(DBDEBUG, "Exit");
}

void Itscam300::enviaDados(SOCKET s, char *data, int size)
{
    int pacote;
    for(int i=0;i<size;){
        pacote = size - i;
        if(pacote>4096) pacote = 4096;
        i += send(s,data+i,pacote,0);
    }
}

int Itscam300::leFocoInfraVermelho()
{
    return reqConfig(0x87,NULL,0,0,1);
}

int Itscam300::leModelo()
{
    return reqConfig(REQ_MODELO,NULL,0,0,3);
}

int Itscam300::leRealceBorda()
{
    return reqConfig(REQ_BORDAS,NULL,0,0,1);
}

int Itscam300::leSombra()
{
    return reqConfig(REQ_ECLIPSE,NULL,0,0,1);
}

int Itscam300::leNumeroFotos()
{
    return reqConfig(REQ_NUMERO_FOTOS,NULL,0,0,1);
}

int Itscam300::lePosicaoFoco()
{
    return reqConfig(REQ_POSICAO_FOCO,NULL,0,0,4);
}

int Itscam300::leAutoFoco()
{
    return reqConfig(REQ_ZOOM_FOCO,NULL,0,0,1);
}


int Itscam300::lePosicaoZoom()
{
    return reqConfig(REQ_POSICAO_ZOOM,NULL,0,0,4);
}

int Itscam300::leNumeroFotosIO()
{
    return reqConfig(REQ_NUMERO_FOTOS,NULL,0,1,1);
}

int Itscam300::leVersaoFirmware()
{
    return reqConfig(REQ_CONF_GER,NULL,0,VERSAO,1);
}

int Itscam300::leRevisaoFirmware()
{
    return reqConfig(REQ_CONF_GER,NULL,0,REVISAO,1);
}

int Itscam300::leHdr()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,HDR,1);
}

int Itscam300::leTipoShutter()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,TIPO_SHUTTER,1);
}

int Itscam300::leShutterFixo()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,SHUTTER_FIXO,2);
}

int Itscam300::leShutterMaximo()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,SHUTTER_MAX,2);
}

int Itscam300::leTipoGanho()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,TIPO_GANHO,1);
}

int Itscam300::leGanhoFixo()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,GANHO_FIXO,1);
}

int Itscam300::leGanhoSegundaFotoLuzInfravermelha()
{
    return reqConfig(REQ_GANHO_IR,NULL,0,0,1);
}

int Itscam300::leGanhoSegundaFotoLuzVisivel()
{
    return reqConfig(REQ_GANHO_VISIVEL,NULL,0,0,1);
}

int Itscam300::leHoraAtual()
{
    return reqConfig(REQ_HORA,NULL,0,0,4);
}

int Itscam300::leTempoEntreTriggers()
{
    return reqConfig(REQ_TEMPO_TRIGGER,NULL,0,0,2);
}

int Itscam300::leDataAtual()
{
    return reqConfig(REQ_DATA,NULL,0,0,4);
}

int Itscam300::leGanhoMaximo()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,GANHO_MAX,1);
}

int Itscam300::leModoTeste()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,MODO_TESTE,1);
}

int Itscam300::leNivelDesejado()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,NIVEL_DESEJADO,1);
}

int Itscam300::leValorNivel()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,NIVEL_ATUAL,1);
}

int Itscam300::leValorGanho()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,VALOR_GANHO,1);
}

int Itscam300::leValorShutter()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,VALOR_SHUTTER,2);
}

int Itscam300::leFormatoFotoIO()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,FORMATO_FOTO_IO,1);
}

int Itscam300::leQualidadeFotoIO()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,QUALIDADE_FOTO_IO,1);
}

int Itscam300::leSituacaoDayNight()
{
    return reqConfig(REQ_SITUACAO_DAY_NIGHT,NULL,0,0,1);
}

int Itscam300::leRotacao()
{
    return reqConfig(REQ_ROTACAO,NULL,0,0,1);
}

int Itscam300::leValorEntrada()
{
    return reqConfig(REQ_CONF_GER,NULL,0,ENTRADA,1);
}

int Itscam300::leAutoIris()
{
    return reqConfig(REQ_LENTE_AUTO_IRIS,NULL,0,0,1);
}

int Itscam300::leModoDayNight()
{
    return reqConfig(REQ_MODO_DAY_NIGHT,NULL,0,0,1);
}

int Itscam300::leModoOCR()
{
    return reqConfig(REQ_MODO_OCR,NULL,0,0,1);
}

int Itscam300::leTipoFlash()
{
    return reqConfig(REQ_CONF_GER,NULL,0,FLASH,1);
}

int Itscam300::leTrigger()
{
    return reqConfig(REQ_CONF_GER,NULL,0,TRIGGER,1);
}

int Itscam300::leTipoSaida()
{
    return reqConfig(REQ_CONF_GER,NULL,0,TIPO_SAIDA,1);
}

int Itscam300::leValorSaida()
{
    return reqConfig(REQ_CONF_GER,NULL,0,SAIDA,1);
}

int Itscam300::leDelay()
{
    return reqConfig(REQ_CONF_GER,NULL,0,DELAY,2);
}

int Itscam300::lePorcentagemSegundoDisparo()
{
    return reqConfig(REQ_SEGUNDO_DISPARO,NULL,0,0,1);
}

int Itscam300::leSaturacao()
{
    return reqConfig(REQ_SATURACAO,NULL,0,0,1);
}

int Itscam300::leBrilho()
{
    return reqConfig(REQ_SATURACAO,NULL,0,1,1);
}

int Itscam300::leContraste()
{
    return reqConfig(REQ_SATURACAO,NULL,0,2,1);
}

int Itscam300::leGamma()
{
    return reqConfig(REQ_GAMMA,NULL,0,0,1);
}

int Itscam300::leFotoColorida()
{
    return reqConfig(REQ_FOTO_COLORIDA,NULL,0,0,1);
}

int Itscam300::lePesos(int *pesos)
{
    for(int i=0;i<16;i++){
        pesos[i] = reqConfig(REQ_PESOS,NULL,0,i,1);
        if(pesos[i]<0) return pesos[i];
    }
    return 1;
}

int Itscam300::leIp(char *ip)
{
    int i = reqConfig(REQ_CONF_REDE,NULL,0,REDE_IP,4);
    sprintf(ip,"%d.%d.%d.%d",i&0xFF,(i>>8)&0xFF,(i>>16)&0xFF,(i>>24)&0xFF);
    return 1;
}

int Itscam300::leMascaraRede(char *mascara)
{
    int i = reqConfig(REQ_CONF_REDE,NULL,0,REDE_MASCARA,4);
    sprintf(mascara,"%d.%d.%d.%d",i&0xFF,(i>>8)&0xFF,(i>>16)&0xFF,(i>>24)&0xFF);
    return 1;
}

int Itscam300::leGateway(char *gateway)
{
    int i = reqConfig(REQ_CONF_REDE,NULL,0,REDE_GATEWAY,4);
    sprintf(gateway,"%d.%d.%d.%d",i&0xFF,(i>>8)&0xFF,(i>>16)&0xFF,(i>>24)&0xFF);
    return 1;
}

int Itscam300::leMac(char *mac)
{
    int i = reqConfig(REQ_CONF_REDE,NULL,0,REDE_MAC,3);
    int j = reqConfig(REQ_CONF_REDE,NULL,0,QUARTO_BYTE_MAC,3);
    sprintf(mac,"%02X-%02X-%02X-%02X-%02X-%02X",i&0xFF,(i>>8)&0xFF,(i>>16)&0xFF,j&0xFF,(j>>8)&0xFF,(j>>16)&0xFF);
    return 1;
}

int Itscam300::leBalancoBranco(int *bb)
{
    int i = reqConfig(REQ_WHITE_BALANCE,NULL,0,0,3);
    bb[0] = i&0xFF;
    bb[1] = (i>>8) & 0xFF;
    bb[2] = (i>>16) & 0xFF;
    return 1;
}

int Itscam300::leGanhoAlternativo()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,TIPO_GANHO_DIF,1);
}

int Itscam300::leValorGanhoAlternativo()
{
    return reqConfig(REQ_CONF_IMAGEM,NULL,0,VALOR_GANHO_DIF,1);
}

int Itscam300::leGammaAlternativo()
{
    return reqConfig(REQ_GAMMA_DIF,NULL,0,0,1);
}

int Itscam300::leValorGammaAlternativo()
{
    return reqConfig(REQ_GAMMA_DIF,NULL,0,1,1);
}

int Itscam300::leBalancoBrancoAlternativo()
{
    return reqConfig(REQ_WB_DIF,NULL,0,0,1);
}

int Itscam300::leValorBalancoBrancoAlternativo(int *bb)
{
    int i = reqConfig(REQ_WB_DIF,NULL,0,1,3);
    if(i<0) return i;
    bb[0] = i&0xFF;
    bb[1] = (i>>8)&0xFF;
    bb[2] = (i>>16)&0xFF;
    return 1;
}

int Itscam300::leBalancoBrancoAtual(int *bb)
{
    int i = reqConfig(REQ_WB_ATUAL,NULL,0,0,3);
    if(i<0) return i;
    bb[0] = i&0xFF;
    bb[1] = (i>>8)&0xFF;
    bb[2] = (i>>16)&0xFF;
    return 1;
}

int Itscam300::leDelayCapturaDay()
{
    return reqConfig(REQ_DELAY_DAY,NULL,0,0,1);
}

int Itscam300::leDelayCapturaNight()
{
    return reqConfig(REQ_DELAY_NIGHT,NULL,0,0,1);
}

int Itscam300::setaConfig(unsigned char conf, int valor,int minimo, int maximo)
{
    unsigned char buf[4];
    int t;
    if(maximo>0 && (valor<minimo || valor>maximo)) return -INVALID_VALUE;
    if(maximo<0) t = 4;
    else if(maximo<0x100) t = 1;
    else if(maximo<0x10000) t = 2;
    else if(maximo<0x1000000) t = 3;
    else t = 4;
    buf[0] = valor & 0xFF;
    buf[1] = (valor>>8) & 0xFF;
    buf[2] = (valor>>16) & 0xFF;
    buf[3] = (valor>>24) & 0xFF;
    t = reqConfig(conf,buf,t,0,1);
    if(t==0) return -INVALID_VALUE;
    else return t;
}

int Itscam300::setaNivelDesejado(int nivel)
{
    return setaConfig(SETA_NIVEL_IMG,nivel,7,62);
}

int Itscam300::setaRotacao(int rotacao180)
{
    return setaConfig(SETA_ROTACAO,rotacao180,0,1);
}

int Itscam300::setaDataAtual(int data)
{
    return setaConfig(SETA_DATA,data,0,-1);
}

int Itscam300::setaHoraAtual(int hora)
{
    return setaConfig(SETA_HORA,hora,0,-1);
}

int Itscam300::setaHdr(int hdr)
{
    return setaConfig(SETA_HDR,hdr,0,1);
}

int Itscam300::setaDelay(int delay)
{
    return setaConfig(SETA_DELAY,delay,100,25000);
}

int Itscam300::setaShutterFixo(int shutter)
{
    return setaConfig(SETA_SHUT,shutter,1,2047);
}

int Itscam300::setaShutterMaximo(int shutter)
{
    return setaConfig(SETA_SHUT_MAX,shutter,1,2047);
}

int Itscam300::setaGanhoFixo(int ganho)
{
    return setaConfig(SETA_GANHO,ganho,0,72);
}

int Itscam300::setaGanhoSegundaFotoLuzVisivel(int ganho)
{
    return setaConfig(SETA_GANHO_VISIVEL,ganho,0,72);
}

int Itscam300::setaGanhoSegundaFotoLuzInfravermelha(int ganho)
{
    return setaConfig(SETA_GANHO_IR,ganho,0,72);
}

int Itscam300::setaValorSaida(int saida)
{
    return setaConfig(SETA_SAIDA,saida,0,3);
}

int Itscam300::setaTempoEntreTriggers(int tempo)
{
    return setaConfig(SETA_TEMPO_TRIGGER,tempo,0,60000);
}

int Itscam300::setaQualidadeFotoIO(int qualidade)
{
    return setaConfig(SETA_QLIDADE_FOTO_IO,qualidade,0,100);
}

int Itscam300::setaGanhoMaximo(int ganho)
{
    return setaConfig(SETA_GANHO_MAX,ganho,0,72);
}

int Itscam300::setaFlash(int flash)
{
    return setaConfig(SETA_FLASH,flash,FLASH_DESAT,FLASH_AUTO_DELAY);
}

int Itscam300::setaTrigger(int trigger)
{
    return setaConfig(SETA_TRIGGER,trigger,TRIG_REDE,TRIG_CONTINUO);
}

int Itscam300::setaTipoSaida(int saida)
{
    return setaConfig(SETA_TIPO_SAIDA,saida,SAIDA_FLASH,SAIDA_IO);
}

int Itscam300::setaTipoShutter(int tipo)
{
    return setaConfig(SETA_TIPO_SHUT,tipo,0,2);
}

int Itscam300::setaTipoGanho(int automatico)
{
    return setaConfig(SETA_TIPO_GANHO,automatico,0,1);
}

int Itscam300::setaModoTeste(int teste)
{
    return setaConfig(SETA_MODO_TESTE,teste,TESTE_NORMAL,TESTE_DIAG);
}

int Itscam300::setaFormatoFotoIO(int formato)
{
    return setaConfig(SETA_FORMATO_FOTO_IO,formato,0,255);
}

int Itscam300::setaLenteAutoIris(int autoiris)
{
    return setaConfig(SETA_LENTE_AUTO_IRIS,autoiris,0,1);
}

int Itscam300::setaModoOCR(int ocr)
{
    return setaConfig(SETA_MODO_OCR,ocr,JIDOSHA_DISABLE,JIDOSHA_MODO_ULTRA_SLOW);
}

int Itscam300::setaModoDayNight(int daynight)
{
    return setaConfig(SETA_MODO_DAY_NIGHT,daynight,DAYNIGHT_AUTO,DAYNIGHT_NIGHT);
}

int Itscam300::setaPadrao()
{
    return reqConfig(SETA_DEFAULT,NULL,0,0,1);
}

int Itscam300::reinicializaItscam()
{
    return reqConfig(REINICIALIZA_ITSCAM,NULL,0,0,0);
}

int Itscam300::setaPorcentagemSegundoDisparo(int porcentagem)
{
    return setaConfig(SETA_SEGUNDO_DISPARO,porcentagem,0,100);
}

int Itscam300::setaSaturacao(int saturacao)
{
    if(saturacao<0 || saturacao>255) return -INVALID_VALUE;
    int sat = reqConfig(REQ_SATURACAO,NULL,0,0,3);
    if(sat<0) return sat;
    sat = (saturacao&0xFF) | (sat&0xFFFF00);
    return setaConfig(SETA_SATURACAO,sat,0,0xFFFFFF);
}

int Itscam300::setaBrilho(int brilho)
{
    if(brilho<0 || brilho>255) return -INVALID_VALUE;
    int sat = reqConfig(REQ_SATURACAO,NULL,0,0,3);
    if(sat<0) return sat;
    sat = (sat&0xFF) | (sat&0xFF0000) | (brilho<<8);
    return setaConfig(SETA_SATURACAO,sat,0,0xFFFFFF);
}

int Itscam300::setaContraste(int contraste)
{
    if(contraste<0 || contraste>255) return -INVALID_VALUE;
    int sat = reqConfig(REQ_SATURACAO,NULL,0,0,3);
    if(sat<0) return sat;
    sat = (sat&0xFFFF) | (contraste<<16);
    return setaConfig(SETA_SATURACAO,sat,0,0xFFFFFF);
}

int Itscam300::setaGamma(int gamma)
{
    return setaConfig(SETA_GAMMA,gamma,0,255);
}

int Itscam300::setaSombra(int sombra)
{
    return setaConfig(SETA_ECLIPSE,sombra,0,8);
}

int Itscam300::setaBalancoBranco(int *bb)
{
    int b = (bb[0]&0xFF) | ((bb[1]&0xFF)<<8) | ((bb[2]&0xFF)<<16);
    return setaConfig(SETA_WHITE_BALANCE,b,0,0xFFFFFF);
}

int Itscam300::setaRealceBorda(int borda)
{
    return setaConfig(SETA_BORDAS,borda,0,3);
}

int Itscam300::setaPesos(int *pesos)
{
    unsigned char p[16];
    for(int i=0;i<16;i++)
        p[i] = pesos[i];
    return reqConfig(SETA_PESOS,p,16,0,1);
}

int Itscam300::setaFotoColorida(int cor)
{
    return setaConfig(SETA_FOTO_COLORIDA,cor,0,1);
}

int Itscam300::setaGanhoAlternativo(int tipo)
{
    return setaConfig(SETA_TIPO_GANHO_DIF,tipo,0,1);
}

int Itscam300::setaValorGanhoAlternativo(int ganho)
{
    return setaConfig(SETA_GANHO_DIF,ganho,0,72);
}

int Itscam300::setaGammaAlternativo(int tipo)
{
    if(tipo<0 || tipo>2) return -INVALID_VALUE;
    int g = reqConfig(REQ_GAMMA_DIF,NULL,0,0,2);
    if(g<0) return g;
    g = (g&0xFF00) | (tipo&0xFF);
    return setaConfig(SETA_GAMMA_DIF,g,0,0xFFFF);
}

int Itscam300::setaValorGammaAlternativo(int gamma)
{
    if(gamma<0 || gamma>255) return -INVALID_VALUE;
    int g = reqConfig(REQ_GAMMA_DIF,NULL,0,0,2);
    if(g<0) return g;
    g = ((gamma&0xFF)<<8) | (g&0xFF);
    return setaConfig(SETA_GAMMA_DIF,g,0,0xFFFF);
}

int Itscam300::setaBalancoBrancoAlternativo(int tipo)
{
    if(tipo<0 || tipo>2) return -INVALID_VALUE;
    int g = reqConfig(REQ_WB_DIF,NULL,0,1,3);
    if(g<0) return g;
    g = (g<<8) | (tipo&0xFF);
    return setaConfig(SETA_WB_DIF,g,0,-1);
}

int Itscam300::setaValorBalancoBrancoAlternativo(int *bb)
{
    int tipo, g = 0;
    for(int i=0;i<3;i++)
        g += bb[i]<<(8*i);
    tipo = reqConfig(REQ_WB_DIF,NULL,0,0,1);
    if(tipo<0) return tipo;
    g = (g<<8) | (tipo&0xFF);
    return setaConfig(SETA_WB_DIF,g,0,-1);
}

int Itscam300::setaZoom(int zoom)
{
    return setaConfig(SETA_ZOOM,zoom,1,1999);
}

int Itscam300::setaFoco(int foco)
{
    return setaConfig(SETA_FOCO,foco,1,1999);
}

int Itscam300::setaAutoFoco(int foco)
{
    return setaConfig(SETA_ZOOM_FOCO,foco,0,2);
}

int Itscam300::setaPosicaoZoom(int zoom)
{
    return setaConfig(SETA_POSICAO_ZOOM,zoom,0,-1);
}

int Itscam300::setaFocoDayNight(int daynight)
{
    return setaConfig(SETA_FOCO_DAY_NIGHT,daynight,1,102);
}

int Itscam300::setaPosicaoFoco(int foco)
{
    return setaConfig(SETA_POSICAO_FOCO,foco,0,-1);
}

int Itscam300::setaDelayCapturaDay(int delay)
{
    return setaConfig(SETA_DELAY_DAY,delay,0,100);
}

int Itscam300::setaDelayCapturaNight(int delay)
{
    return setaConfig(SETA_DELAY_NIGHT,delay,0,100);
}

bool verificaEndereco(char *ip, unsigned char *v)
{
    int k = 0, c = 0;
    bool n = false;
    for(unsigned int i=0;i<=strlen(ip);i++){
        if(ip[i]=='.' || ip[i]=='\0'){
            if(!n) return false;
            v[c++] = k & 0xFF;
            if(c>4) return false;
            k = 0;
            n = false;
        }
        else if((ip[i]>='0' && ip[i]<='9')){
            n = true;
            k = k*10 + ip[i] - '0';
            if(k>255) return false;
        }
        else return false;
    }
    return true;
}

int Itscam300::setaArquivoLog(const char *arquivo)
{
    if(strlen(arquivo)<5 || strlen(arquivo)>2000){
        arqLog[0] = '\0';
        return -INVALID_VALUE;
    }
    strcpy(arqLog,arquivo);
    return 1;
}

int Itscam300::setaIp(char *ip)
{
    unsigned char v[4];
    if(!verificaEndereco(ip,v)) return -INVALID_VALUE;
    return reqConfig(SETA_IP,v,4,0,1);
}

int Itscam300::atualizarFirmware(unsigned char *firmware, int tamanho)
{
    unsigned char *buf = NULL;
    int ret;
    if(tamanho==1024*1024){
        buf = new unsigned char[tamanho+2];
        int crc = updateCrc(firmware,tamanho);
        buf[0] = crc & 0xFF;
        buf[1] = (crc>>8) & 0xFF;
        memcpy(buf+2,firmware,tamanho);
        ret = reqConfig(FIRMWARE,buf,2+tamanho,0,1);
    }
    else{
        buf = new unsigned char[tamanho+5];
        int crc = updateCrc(firmware,tamanho);
        buf[0] = tamanho & 0xFF;
        buf[1] = (tamanho>>8) & 0xFF;
        buf[2] = (tamanho>>16) & 0xFF;
        buf[3] = crc & 0xFF;
        buf[4] = (crc>>8) & 0xFF;
        memcpy(buf+5,firmware,tamanho);
        ret = reqConfig(FIRMWARE_TAMANHO,buf,5+tamanho,0,1);
    }
    if(buf!=NULL) delete[] buf;
    return ret;
}
int Itscam300::atualizarFirmwareSenha(unsigned char *firmware, int tamanho, unsigned short senha)
{
    unsigned char *buf = NULL;
    int ret;
    if(tamanho==1024*1024){
        buf = new unsigned char[tamanho+2];
        buf[0] = senha & 0xFF;
        buf[1] = (senha>>8) & 0xFF;
        memcpy(buf+2,firmware,tamanho);
        ret = reqConfig(FIRMWARE,buf,2+tamanho,0,1);
    }
    else{
        buf = new unsigned char[tamanho+5];
        buf[0] = tamanho & 0xFF;
        buf[1] = (tamanho>>8) & 0xFF;
        buf[2] = (tamanho>>16) & 0xFF;
        buf[3] = senha & 0xFF;
        buf[4] = (senha>>8) & 0xFF;
        memcpy(buf+5,firmware,tamanho);
        ret = reqConfig(FIRMWARE_TAMANHO,buf,5+tamanho,0,1);
    }
    if(buf!=NULL) delete[] buf;
    return ret;
}

int Itscam300::setaMascaraRede(char *mascara)
{
    unsigned char v[4];
    if(!verificaEndereco(mascara,v)) return -INVALID_VALUE;
    return reqConfig(SETA_MASCARA,v,4,0,1);
}

int Itscam300::setaMac(unsigned char *mac)
{
    memcpy(macItscam,mac,6);
    return reqConfig(SETA_MAC,mac,6,0,1);
}

int Itscam300::setaGateway(char *gateway)
{
    unsigned char v[4];
    if(!verificaEndereco(gateway,v)) return -INVALID_VALUE;
    return reqConfig(SETA_GATEWAY,v,4,0,1);
}

int Itscam300::reqConfig(unsigned char conf,unsigned char *parametros, int tparametros, int offset,int len)
{
    int size, timeout;
    unsigned char *buf;
    DEBUG(DBDEBUG, "status %d", status);
    if(status!=CAMERA_OK) return -status;

    switch(conf){
        case REQ_CONF_REDE:
            size = 18;
            break;
        case REQ_CONF_IMAGEM:
            size = 19;
            break;
        case REQ_CONF_GER:
            size = 9;
            break;
        case REQ_SATURACAO:
        case REQ_WHITE_BALANCE:
        case REQ_WB_ATUAL:
        case REQ_MODELO:
            size = 3;
            break;
        case REQ_GAMMA_DIF:
        case REQ_NUMERO_FOTOS:
        case REQ_TEMPO_TRIGGER:
            size = 2;
            break;
        case REQ_WB_DIF:
        case REQ_POSICAO_ZOOM:
        case REQ_POSICAO_FOCO:
        case REQ_DATA:
        case REQ_HORA:
            size = 4;
            break;
        case REQ_PESOS:
            size = 16;
            break;
        default:
            size = 1;
            break;
    }
    switch(conf){
        case REINICIALIZA_ITSCAM:
            timeout = 10;
            break;
        case FIRMWARE:
        case FIRMWARE_TAMANHO:
            timeout = 2*60*1000;
            break;
        case SETA_FOCO:
        case SETA_ZOOM:
        case SETA_FOCO_DAY_NIGHT:
        case SETA_POSICAO_ZOOM:
        case REQ_POSICAO_ZOOM:
        case SETA_POSICAO_FOCO:
        case REQ_POSICAO_FOCO:
            timeout = 30*1000;
            break;
        case SETA_ZOOM_FOCO:
            timeout = 300000;
            break;
        default:
            timeout = 2000;
            break;
    }
    RecebeDados *cfg;
    cfg = new RecebeDados(conf,size);
    cfg->lock();
    WaitForSingleObject(semaforo,INFINITE);
    rd->adicione(cfg);
    buf = new unsigned char[4+tparametros];
    buf[0] = CABECALHO;
    buf[1] = conf;
    memcpy(buf+2,parametros,tparametros);
    if(conf!=FIRMWARE && conf!=FIRMWARE_TAMANHO){
        int crc = updateCrc(buf,2+tparametros);
        buf[2+tparametros] = crc&0xFF;
        buf[2+tparametros+1] = (crc>>8)&0xFF;
        if(sock!=INVALID_SOCKET) enviaDados(sock,(char*)buf,4+tparametros);
    }
    else if(sock!=INVALID_SOCKET) enviaDados(sock,(char*)buf,2+tparametros);
    delete[] buf;
    ReleaseSemaphore(semaforo,1,NULL);
    if(cfg->espere(timeout)==WAIT_TIMEOUT){
        rd->remova(cfg);
        cfg->unlock();
        DEBUG(DBDEBUG, "TIMEOUT_EXCEEDED");
        return -TIMEOUT_EXCEEDED;
    }
    if(status==OBJECT_DESTROYED) return -OBJECT_DESTROYED;
    int ret = 0;
    for(int i=0;i<len;i++){
        ret += ((int)cfg->buffer()[i+offset])<<(8*i);
    }
    rd->remova(cfg);
    cfg->unlock();
    return ret;
}

int Itscam300::requisitaMultiplasFotosSemEspera(int nfotos, int formato, int qualidade)
{
    unsigned char request[6];
    int i, crc;
    if(status!=CAMERA_OK) return -status;
    RecebeDados **cfg;
    WaitForSingleObject(semaforoFotos,INFINITE);
    if(nfotos!=numeroFotos){
        int snf = setaNumeroFotos(nfotos);
        if(snf!=1){
            ReleaseSemaphore(semaforoFotos,1,NULL);
            return -INVALID_VALUE;
        }
        numeroFotos = nfotos;
    }
    WaitForSingleObject(semaforo,INFINITE);
    //Procura um slot
    RecebeDados *sl[16];
    memset(sl,0,sizeof(sl));
    WaitForSingleObject(semSlots,INFINITE);
    for(i=0;i<(int)(sizeof(slots)/sizeof(sl));i++)
        if(!memcmp(slots[i],sl,sizeof(sl))) break;
    if(i>=(int)(sizeof(slots)/sizeof(sl))){
        ReleaseSemaphore(semaforo,1,NULL);
        ReleaseSemaphore(semSlots,1,NULL);
        ReleaseSemaphore(semaforoFotos,1,NULL);
        return -SLOT_FULL;
    }
    //Slot encontrado, requisite as fotos
    cfg = new RecebeDados*[nfotos];
    for(int j=0;j<nfotos;j++){
        cfg[j] = new RecebeDados(FOTO);
        cfg[j]->lock();
        rd->adicione(cfg[j]);
        slots[i][j] = cfg[j];
    }
    delete[] cfg;
    ReleaseSemaphore(semSlots,1,NULL);
    request[0] = CABECALHO;
    request[1] = FOTO;
    request[2] = formato;
    request[3] = qualidade;
    crc = updateCrc(request,4);
    request[4] = crc&0xFF;
    request[5] = (crc>>8)&0xFF;
    if(sock!=INVALID_SOCKET) enviaDados(sock,(char*)request,6);
    ReleaseSemaphore(semaforo,1,NULL);
    ReleaseSemaphore(semaforoFotos,1,NULL);
    return i;
}

int Itscam300::requisitaFotoId(int slot, int idFoto, unsigned char *buf, int *tempo)
{
    int ret;
    if( slot >= (int)(sizeof(slots)/(16*sizeof(RecebeDados *))) ) return -INVALID_VALUE;
    if(idFoto>=16) return -INVALID_VALUE;
    RecebeDados *cfg = slots[slot][idFoto];
    if(cfg==NULL) return -INVALID_VALUE;
    if(status!=CAMERA_OK) return -status;
    if(cfg->espere(10000)==WAIT_TIMEOUT){
        rd->remova(cfg);
        cfg->unlock();
        slots[slot][idFoto] = NULL;
        return -NOT_READY;
    }
    memcpy(buf,cfg->buffer()+sizeof(int),cfg->capacidade()-sizeof(int));
    ret = cfg->capacidade()-sizeof(int);
    if(tempo!=NULL) memcpy(tempo,cfg->buffer(),sizeof(int));
    rd->remova(cfg);
    cfg->unlock();
    slots[slot][idFoto] = NULL;
    return ret;
}

int Itscam300::requisita(unsigned char **buf, int formato, int qualidade, bool video, int nfotos, int *tamanhos)
{
    unsigned char request[6];
    unsigned char r;
    int crc,ret=1;
    if(status!=CAMERA_OK) return -status;
    r = video?VIDEO:FOTO;
    RecebeDados **cfg;
    WaitForSingleObject(semaforoFotos,INFINITE);
    if(!video && nfotos!=numeroFotos){
        int snf = setaNumeroFotos(nfotos);
        if(snf!=1){
            ReleaseSemaphore(semaforoFotos,1,NULL);
            return -INVALID_VALUE;
        }
        numeroFotos = nfotos;
    }
    WaitForSingleObject(semaforo,INFINITE);
    cfg = new RecebeDados*[nfotos];
    for(int i=0;i<nfotos;i++){
        cfg[i] = new RecebeDados(r);
        cfg[i]->lock();
        rd->adicione(cfg[i]);
    }
    request[0] = CABECALHO;
    request[1] = r;
    request[2] = formato;
    request[3] = qualidade;
    crc = updateCrc(request,4);
    request[4] = crc&0xFF;
    request[5] = (crc>>8)&0xFF;
    if(sock!=INVALID_SOCKET) enviaDados(sock,(char*)request,6);
    ReleaseSemaphore(semaforo,1,NULL);
    ReleaseSemaphore(semaforoFotos,1,NULL);
    for(int i=0;i<nfotos;i++){
        /* Timeout para a requisicao de 500ms */
        if(cfg[i]->espere(1*1000)==WAIT_TIMEOUT){
            rd->remova(cfg[i]);
            cfg[i]->unlock();
            tamanhos[i] = 0;
            ret = -TIMEOUT_EXCEEDED;
        }
        else{
            if(status==OBJECT_DESTROYED){
                delete[] cfg;
                return -OBJECT_DESTROYED;
            }
            memcpy(buf[i],cfg[i]->buffer()+sizeof(int),cfg[i]->capacidade()-sizeof(int));
            tamanhos[i] = cfg[i]->capacidade()-sizeof(int);
            rd->remova(cfg[i]);
            cfg[i]->unlock();
        }
    }
    delete[] cfg;
    return ret;
}

int Itscam300::requisitaMultiplasFotosIOSemEspera(int nfotos, int formato, int qualidade)
{
    RecebeDados *env;

    if(status!=CAMERA_OK) return -status;

    if(numeroFotosIO!=nfotos){
        int snf = setaNumeroFotosIO(nfotos);
        if(snf!=1){
            return -INVALID_VALUE;
        }
        numeroFotosIO = nfotos;
    }
    if(qualidadeFotoIO!=qualidade){
        int sq = setaQualidadeFotoIO(qualidade);
        if(sq!=1){
            return -INVALID_VALUE;
        }
        qualidadeFotoIO = qualidade;
    }
    if(formatoFotoIO!=formato){
        int sq = setaFormatoFotoIO(formato);
        if(sq!=1){
            return -INVALID_VALUE;
        }
        formatoFotoIO = formato;
    }

    //Procura um envelope
    DWORD ini = GetTickCount();
    do{
        env = rd->encontre(1000+nfotos);
        if(env==NULL) Sleep(1);
    } while(status==CAMERA_OK && env==NULL && diferencaTempo(GetTickCount(),ini)<=timeoutIO);
    if(status!=CAMERA_OK) return -status;

    if(env==NULL) return -TIMEOUT_EXCEEDED;

    //Procura um slot
    int i;
    RecebeDados *sl[16];
    memset(sl,0,sizeof(sl));
    WaitForSingleObject(semSlots,INFINITE);
    for(i=0;i<(int)(sizeof(slots)/sizeof(sl));i++)
        if(!memcmp(slots[i],sl,sizeof(sl))) break;
    if(i>=(int)(sizeof(slots)/sizeof(sl))){
        ReleaseSemaphore(semSlots,1,NULL);
        return -SLOT_FULL;
    }
    //Slot encontrado, requisite as fotos
    for(int j=0;j<nfotos;j++)
        memcpy(&slots[i][j],env->buffer()+sizeof(RecebeDados *)*j,sizeof(RecebeDados *));
    ReleaseSemaphore(semSlots,1,NULL);
    env->unlock();
    return i;
}

int Itscam300::requisitaFotoTriggerContinuo(unsigned char *buf, int, int)
{
    RecebeDados *env;
    int ret = 1;

    if(status!=CAMERA_OK) return -status;

    //Procura um envelope
    DWORD ini = GetTickCount();
    do{
        env = rd->encontre(1001);
        if(env==NULL) Sleep(1);
    } while(status==CAMERA_OK && env==NULL && diferencaTempo(GetTickCount(),ini)<=timeoutIO);
    if(status!=CAMERA_OK) return -status;
    if(env==NULL){
        return -TIMEOUT_EXCEEDED;
    }
    //Espera o timeout
    if(env->espere(timeoutIO)==WAIT_TIMEOUT){
        //Timeout, libere tudo e zere tamanhos
        RecebeDados *foto;
        memcpy(&foto,env->buffer(),sizeof(RecebeDados *));
        foto->unlock();
        ret = -TIMEOUT_EXCEEDED;
    }
    else{
        if(status==OBJECT_DESTROYED) return -OBJECT_DESTROYED;
        //Sem timeout, copie buffers
        RecebeDados *foto;
        memcpy(&foto,env->buffer(),sizeof(RecebeDados *));
        memcpy(buf,foto->buffer()+sizeof(int),foto->capacidade()-sizeof(int));
        ret = foto->capacidade()-sizeof(int);
        foto->unlock();
    }
    env->unlock();
    return ret;
}



int Itscam300::requisitaIO(unsigned char **buf, int formato, int qualidade, int nfotos, int *tamanhos)
{
    RecebeDados *env;
    int ret = 1;

    if(status!=CAMERA_OK) return -status;

    if(numeroFotosIO!=nfotos){
        int snf = setaNumeroFotosIO(nfotos);
        if(snf!=1){
            return -INVALID_VALUE;
        }
        numeroFotosIO = nfotos;
    }
    if(qualidadeFotoIO!=qualidade){
        int sq = setaQualidadeFotoIO(qualidade);
        if(sq!=1){
            return -INVALID_VALUE;
        }
        qualidadeFotoIO = qualidade;
    }
    if(formatoFotoIO!=formato){
        int sq = setaFormatoFotoIO(formato);
        if(sq!=1){
            return -INVALID_VALUE;
        }
        formatoFotoIO = formato;
    }

    //Procura um envelope
    DWORD ini = GetTickCount();
    do{
        env = rd->encontre(1000+nfotos);
        if(env==NULL) Sleep(1);
    } while(status==CAMERA_OK && env==NULL && diferencaTempo(GetTickCount(),ini)<=timeoutIO);
    if(status!=CAMERA_OK) return -status;
    if(env==NULL){
        for(int i=0;i<nfotos;i++)
            tamanhos[i] = 0;
        return -TIMEOUT_EXCEEDED;
    }
    //Espera o timeout
    if(env->espere(timeoutIO)==WAIT_TIMEOUT){
        //Timeout, libere tudo e zere tamanhos
        for(int i=0;i<nfotos;i++){
            RecebeDados *foto;
            memcpy(&foto,env->buffer()+(sizeof(RecebeDados *)*i),sizeof(RecebeDados *));
            foto->unlock();
            tamanhos[i] = 0;
        }
        ret = -TIMEOUT_EXCEEDED;
    }
    else{
        if(status==OBJECT_DESTROYED) return -OBJECT_DESTROYED;
        //Sem timeout, copie buffers
        for(int i=0;i<nfotos;i++){
            int t;
            RecebeDados *foto;
            memcpy(&foto,env->buffer()+(sizeof(RecebeDados *)*i),sizeof(RecebeDados *));
            memcpy(&t,foto->buffer(),sizeof(int));
            if(t>ret) ret = t;
            memcpy(buf[i],foto->buffer()+sizeof(int),foto->capacidade()-sizeof(int));
            tamanhos[i] = foto->capacidade()-sizeof(int);
            foto->unlock();
        }
    }
    env->unlock();
    return ret;
}

int Itscam300::requisitaFotoIO(unsigned char *buf, int formato, int qualidade)
{
    int tam = 0;
    int ret = requisitaIO(&buf,formato,qualidade,1,&tam);
    if(ret>=1) return tam;
    else return ret;
}

int Itscam300::requisitaMultiplasFotosIO(unsigned char **foto,int nFotos,int *tamFotos,int formato, int qualidade)
{
    return requisitaIO(foto,formato,qualidade,nFotos,tamFotos);
}

int Itscam300::salvarFotoIO(char *arquivo,int formato,int qualidade)
{
    unsigned char *buf;
    buf = new unsigned char[5*1024*1024];
    int tam = 0;
    int ret = requisitaIO(&buf,formato,qualidade,1,&tam);
    if(ret<=0){
        delete[] buf;
        return ret;
    }
    FILE *st;
    st = fopen(arquivo,"wb");
    if(st!=NULL){
        fwrite(buf,1,tam,st);
        fclose(st);
    }
    else{
        delete[] buf;
        return -NOT_SAVED;
    }
    delete[] buf;
    return tam;
}

int Itscam300::salvarFotoTriggerContinuo(char *arquivo,int formato,int qualidade)
{
    unsigned char *buf;
    buf = new unsigned char[5*1024*1024];
    int ret = requisitaFotoTriggerContinuo(buf,formato,qualidade);
    if(ret<=0){
        delete[] buf;
        return ret;
    }
    FILE *st;
    st = fopen(arquivo,"wb");
    if(st!=NULL){
        fwrite(buf,1,ret,st);
        fclose(st);
    }
    else{
        delete[] buf;
        return -NOT_SAVED;
    }
    delete[] buf;
    return ret;
}

int Itscam300::salvarMultiplasFotosIO(char **filename,int nFotos,int *tamFotos,int formato,int qualidade)
{
    unsigned char *buf[16];
    int tam[16];
    for(int i=0;i<nFotos;i++)
        buf[i] = new unsigned char[5*1024*1024];
    int ret = requisitaIO(buf,formato,qualidade,nFotos,tam);
    if(ret<=0){
        for(int i=0;i<nFotos;i++)
            delete[] buf[i];
        return ret;
    }
    for(int i=0;i<nFotos;i++){
        FILE *st;
        st = fopen(filename[i],"wb");
        tamFotos[i] = tam[i];
        if(st!=NULL){
            fwrite(buf[i],1,tam[i],st);
            fclose(st);
        }
        else ret = -NOT_SAVED;
        delete[] buf[i];
    }
    return ret;
}


int Itscam300::requisitaQuadroVideo(unsigned char *buf, int formato, int qualidade)
{
    int tam = 0;
    int ret = requisita(&buf,formato,qualidade,true,1,&tam);
    if(ret>=1) return tam;
    else return ret;
}

int Itscam300::requisitaFoto(unsigned char *buf, int formato, int qualidade)
{
    int tam = 0;
    int ret = requisita(&buf,formato,qualidade,false,1,&tam);
    if(ret>=1) return tam;
    else return ret;
}

int Itscam300::salvarFoto(char *arquivo, int formato, int qualidade)
{
    unsigned char *buf;
    buf = new unsigned char[5*1024*1024];
    int tam = 0;
    DEBUG(DBDEBUG, "entrou");
    int ret = requisita(&buf,formato,qualidade,false,1,&tam);
    DEBUG(DBDEBUG, "resultado de requisita %d", ret);

    if(ret<=0){
        delete[] buf;
        DEBUG(DBDEBUG, "delete buf e retorna %d", ret);
        return ret;
    }
    FILE *st;
    st = fopen(arquivo,"wb");
    if(st!=NULL){
        fwrite(buf,1,tam,st);
        fclose(st);
    }
    else{
        delete[] buf;
        return -NOT_SAVED;
    }
    delete[] buf;
    return tam;
}

int Itscam300::salvarMultiplasFotos(char **filename,int nFotos,int *tamFotos,int formato, int qualidade)
{
    unsigned char *buf[16];
    int tam[16];
    for(int i=0;i<nFotos;i++)
        buf[i] = new unsigned char[5*1024*1024];
    int ret = requisita(buf,formato,qualidade,false,nFotos,tam);
    if(ret<=0){
        for(int i=0;i<nFotos;i++)
            delete[] buf[i];
        return ret;
    }
    for(int i=0;i<nFotos;i++){
        FILE *st;
        tamFotos[i] = tam[i];
        st = fopen(filename[i],"wb");
        if(st!=NULL){
            fwrite(buf[i],1,tam[i],st);
            fclose(st);
        }
        else ret = -NOT_SAVED;
        delete[] buf[i];
    }
    return ret;

}

int Itscam300::requisitaMultiplasFotos(unsigned char **buf, int nfotos, int *tamanho, int formato, int qualidade)
{
    return requisita(buf,formato,qualidade,false,nfotos,tamanho);
}

int Itscam300::setaNumeroFotos(int nFotos)
{
    return setaConfig(SETA_NUMERO_FOTOS,nFotos,1,16);
}

int Itscam300::setaNumeroFotosIO(int nFotos)
{
    return setaConfig(SETA_NUMERO_FOTOS_IO,nFotos,1,16);
}

int Itscam300::setaTimeoutIO(int sec)
{
    if(status!=CAMERA_OK) return -status;
    if(sec<0) return -INVALID_VALUE;
    if(sec==0) timeoutIO = INFINITE;
    else timeoutIO = 1000*sec;
    return 1;
}

int Itscam300::salvarFotoOcrIO(char *diretorio, int qualidade)
{
    unsigned char *buf = new unsigned char[2*1024*1024];
    char placa[8];
    int tam = requisitaFotoIO(buf,1,qualidade);
    if(tam>0)
    {
        struct timeval tv;
        int ocr;
        ocr = ocrJPEG(buf,placa);
        gettimeofday(&tv,NULL);

        char arquivo[512];
        strcpy(arquivo,diretorio);
        if(ocr!=1) strcpy(placa,"PLACAND");
        if(arquivo[strlen(arquivo)-1]!='\\') strcat(arquivo,"\\");

        struct tm tm_time;
        localtime_r((time_t *)&tv.tv_sec,&tm_time);
        sprintf(arquivo+strlen(arquivo),"%02d%02d%02d_%02d%02d%02d%03d_%s.jpg",
            tm_time.tm_year%100,tm_time.tm_mon+1,tm_time.tm_mday,tm_time.tm_hour,tm_time.tm_min,tm_time.tm_sec,(int)(tv.tv_usec/1000),placa);

        FILE *st = fopen(arquivo,"wb");
        if(st!=NULL){
            fwrite(buf,1,tam,st);
            fclose(st);
        }
        else tam = -NOT_SAVED;
    }
    delete[] buf;
    return tam;
}

int Itscam300::salvarFotoOcrTriggerContinuo(char *diretorio, int qualidade)
{
    unsigned char *buf = new unsigned char[2*1024*1024];
    char placa[8];
    int tam = requisitaFotoTriggerContinuo(buf,1,qualidade);
    if(tam>0)
    {
        struct timeval tv;
        int ocr;
        ocr = ocrJPEG(buf,placa);
        gettimeofday(&tv,NULL);

        char arquivo[512];
        strcpy(arquivo,diretorio);
        if(ocr!=1) strcpy(placa,"PLACAND");
        if(arquivo[strlen(arquivo)-1]!='\\') strcat(arquivo,"\\");

        struct tm tm_time;
        localtime_r((time_t *)&tv.tv_sec,&tm_time);
        sprintf(arquivo+strlen(arquivo),"%02d%02d%02d_%02d%02d%02d%03d_%s.jpg",
            tm_time.tm_year%100,tm_time.tm_mon+1,tm_time.tm_mday,tm_time.tm_hour,tm_time.tm_min,tm_time.tm_sec,(int)(tv.tv_usec/1000),placa);

        FILE *st = fopen(arquivo,"wb");
        if(st!=NULL){
            fwrite(buf,1,tam,st);
            fclose(st);
        }
        else tam = -NOT_SAVED;
    }
    delete[] buf;
    return tam;
}


int Itscam300::salvarFotoOcr(char *diretorio, int qualidade)
{
    unsigned char *buf = new unsigned char[2*1024*1024];
    char placa[8];
    int tam = requisitaFoto(buf,1,qualidade);
    if(tam>0)
    {
        struct timeval tv;
        int ocr;
        ocr = ocrJPEG(buf,placa);
        gettimeofday(&tv,NULL);

        char arquivo[512];
        strcpy(arquivo,diretorio);
        if(ocr!=1) strcpy(placa,"PLACAND");
        if(arquivo[strlen(arquivo)-1]!='\\') strcat(arquivo,"\\");

        struct tm tm_time;
        localtime_r((time_t *)&tv.tv_sec,&tm_time);
        sprintf(arquivo+strlen(arquivo),"%02d%02d%02d_%02d%02d%02d%03d_%s.jpg",
            tm_time.tm_year%100,tm_time.tm_mon+1,tm_time.tm_mday,tm_time.tm_hour,tm_time.tm_min,tm_time.tm_sec,(int)(tv.tv_usec/1000),placa);

        FILE *st = fopen(arquivo,"wb");
        if(st!=NULL){
            fwrite(buf,1,tam,st);
            fclose(st);
        }
        else tam = -NOT_SAVED;
    }
    delete[] buf;
    return tam;
}

int Itscam300::salvarMultiplasFotosOcr(char *diretorio, int nFotos, int qualidade){
    unsigned char *buf[16];
    int tam[16];
    int i = 0;
    for(i=0;i<nFotos;i++){
        buf[i] = new unsigned char[5*1024*1024];
    }
    int ret = requisita(buf,1,qualidade,false,nFotos,tam);
    if(ret<=0){
        for(i=0;i<nFotos;i++)
            delete[] buf[i];
        return ret;
    }
    for( i=0;i<nFotos;i++)
    {
        char placa[8] = "";
        FILE *st;
        struct timeval tv;
        int ocr;
        ocr = ocrJPEG(buf[i],placa);
        gettimeofday(&tv,NULL);

        char arquivo[512];
        strcpy(arquivo,diretorio);
        if(ocr!=1) strcpy(placa,"PLACAND");
        if(arquivo[strlen(arquivo)-1]!='\\') strcat(arquivo,"\\");

        struct tm tm_time;
        localtime_r((time_t *)&tv.tv_sec,&tm_time);
        sprintf(arquivo+strlen(arquivo),"%02d%02d%02d_%02d%02d%02d%03d_%02d_%s.jpg",
            tm_time.tm_year%100,tm_time.tm_mon+1,tm_time.tm_mday,tm_time.tm_hour,tm_time.tm_min,tm_time.tm_sec,(int)(tv.tv_usec/1000),i+1,placa);

        st = fopen(arquivo,"wb");
        if(st!=NULL){
            fwrite(buf[i],1,tam[i],st);
            fclose(st);
        }
        else ret = -NOT_SAVED;
        delete[] buf[i];
    }
    return ret;
}

int Itscam300::salvarMultiplasFotosOcrIO(char *diretorio, int nFotos, int qualidade){
    unsigned char *buf[16];
    int tam[16];
    int i = 0;
    for(i=0;i<nFotos;i++){
        buf[i] = new unsigned char[5*1024*1024];
    }
    int ret = requisitaIO(buf,1,qualidade,nFotos,tam);
    if(ret<=0){
        for(i=0;i<nFotos;i++)
            delete[] buf[i];
        return ret;
    }
    for(i=0;i<nFotos;i++)
    {
        char placa[8] = "";
        FILE *st;
        struct timeval tv;
        int ocr;
        ocr = ocrJPEG(buf[i],placa);
        gettimeofday(&tv,NULL);

        char arquivo[512];
        strcpy(arquivo,diretorio);
        if(ocr!=1) strcpy(placa,"PLACAND");
        if(arquivo[strlen(arquivo)-1]!='\\') strcat(arquivo,"\\");

        struct tm tm_time;
        localtime_r((time_t *)&tv.tv_sec,&tm_time);
        sprintf(arquivo+strlen(arquivo),"%02d%02d%02d_%02d%02d%02d%03d_%02d_%s.jpg",
            tm_time.tm_year%100,tm_time.tm_mon+1,tm_time.tm_mday,tm_time.tm_hour,tm_time.tm_min,tm_time.tm_sec,(int)(tv.tv_usec/1000),i+1,placa);

        st = fopen(arquivo,"wb");
        if(st!=NULL){
            fwrite(buf[i],1,tam[i],st);
            fclose(st);
        }
        else ret = -NOT_SAVED;
        delete[] buf[i];
    }
    return ret;
}

int Itscam300::ocrJPEG(unsigned char *jpeg, char *placa){
    bool comment = false;
    if(jpeg==NULL || jpeg[0]!=0xFF || jpeg[1]!=0xD8) return -INVALID_VALUE;
    memset(placa,0,8);
    for(int i=0;;i++){
        if(jpeg[i]==0xFF && jpeg[i+1]==0xD9) return 0;
        if(jpeg[i]==0xFF && jpeg[i+1]==0xFE){
            comment = true;
            i += 4;
        }
        if(comment && !strncmp((char*)jpeg+i,"Placa=",6)){
            if(jpeg[i+6]==';') strcpy(placa,"0000000");
            else strncpy(placa,(char*)jpeg+i+6,7);
            placa[7] = '\0';
            return 1;
        }
    }
}

void Itscam300::processaDados(unsigned char *buf, int len)
{
    for(int i=0;i<len;i++){
        switch(estado){
            case 0:
                if(buf[i]==CABECALHO) estado = -1;
                break;
            case -1:
                if(buf[i]==MENSAGEM_TEXTO) estado = MENSAGEM_TEXTO;
                else if(buf[i]==TEMPO_LACO) estado = TEMPO_LACO;
                else if(buf[i]==FOTO){
                    if(tamanhoPacote<=0){
                        estado = FOTO;
                        tamanhoPacote = 0;
                    }
                    else estado = FOTO_IO;
                }
                else if(buf[i]==VIDEO || buf[i]==VIDEO_REDIMENSIONADO) estado = VIDEO;
                else if(buf[i]==INICIO_PACOTE_CRC) estado = INICIO_PACOTE_CRC;
                else{
                    config = rd->encontre(buf[i]);
                    if(config!=NULL){
                        estado = 100;
                    }
                    else estado = 0;
                }
                byteCount = 0;
                break;
            case TEMPO_LACO:
                if(byteCount==0) tempoLaco = 0;
                tempoLaco += ((int) buf[i])<<(8*byteCount);
                byteCount++;
                if(byteCount>=4) estado = 0;
                break;
            case INICIO_PACOTE_CRC:
                if(byteCount==0) auxPacote = buf[i];
                else if(byteCount==1){
                    auxCrc = buf[i];
                }
                else if(byteCount==2){
                    unsigned char pct[3];
                    int crc = 256*(int)buf[i];
                    crc += (int) auxCrc;
                    pct[0] = CABECALHO;
                    pct[1] = INICIO_PACOTE_CRC;
                    pct[2] = auxPacote;
                    if(tamanhoPacote<=0){
                        if(crc==updateCrc(pct,3)){
                            tamanhoPacote = auxPacote;
                            envelope = new RecebeDados(1000+tamanhoPacote,2*sizeof(RecebeDados *)*tamanhoPacote);
                            envelope->lock();
                            for(int y=0;y<tamanhoPacote;y++){
                                RecebeDados *cfg = new RecebeDados(FOTO_IO);
                                cfg->lock();
                                rd->adicione(cfg);
                                envelope->preenche((unsigned char *)&cfg,sizeof(RecebeDados *));
                            }
                            rd->adicione(envelope);
                        }
                        else tamanhoPacote = 0;
                    }
                    estado = 0;
                }
                byteCount++;
                break;
            case 100:
                if(config!=NULL){
                    int y = config->preenche(buf+i,len-i);
                    i += y - 1;
                    if(config->preenchido()){
                        estado = 0;
                        rd->remova(config);
                        config->unlock();
                        config = NULL;
                    }
                }
                break;
            case MENSAGEM_TEXTO:
                texto[byteCount++] = buf[i];
                if(byteCount>=62){
                    log(texto);
                    estado = 0;
                }
                break;
            case VIDEO:
            case FOTO:
            case FOTO_IO:
                if(byteCount==0){
                    tempInt = 0;
                }
                else if(byteCount<5){
                    tempInt += ((int) buf[i])<<(8*(byteCount-1));
                    if(byteCount==4){
                        config = rd->encontre(estado&0xFF);
                        if(config!=NULL){
                            config->redimensione(tempInt+sizeof(int));
                            config->preenche((unsigned char *)&tempoLaco,sizeof(int));
                            tempoLaco = 0;
                        }
                        else{
                            char logErro[1024];
                            imagem = new unsigned char[tempInt];
                            sprintf(logErro,"Recebendo imagem de %d bytes sem solicitar e fora de envelope!",tempInt);
                            log(logErro);
                        }
                    }
                }
                else{
                    if(config!=NULL){
                        int y = config->preenche(buf+i,len-i);
                        i += y-1;
                        byteCount += y-1;
                        if(config->preenchido()){
                            if(estado==FOTO_IO){
                                envelope->preenche((unsigned char *)&config,sizeof(RecebeDados *));
                                if(envelope->preenchido()) rd->adicioneEnvelope(envelope);
                                tamanhoPacote--;
                            }
                            rd->remova(config);
                            config->unlock();
                            config = NULL;
                            estado = 0;
                        }
                    }
                    else if(imagem!=NULL){
                        int y = len - i;
                        if(tempInt-(byteCount-5)<y) y = tempInt-(byteCount-5);
                        memcpy(imagem+byteCount-5,buf+i,y);
                        i += y - 1;
                        byteCount += y - 1;
                        if(byteCount-5+1>=tempInt){
                            estado = 0;
                            delete[] imagem;
                        }
                    }
                }
                byteCount++;
                break;
            default:
                estado = 0;
                break;
        }
    }
}

