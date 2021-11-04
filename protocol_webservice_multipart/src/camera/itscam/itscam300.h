#ifndef ITSCAM300_H
#define ITSCAM300_H

#include "camera/itscam/compat_pumatronix.h"
#include "camera/itscam/protocolo.h"
#include "camera/itscam/recebedados.h"

class Itscam300 {
private:

    static Itscam300 *ids[];
    int estado;
    int tamanhoPacote;
    int byteCount;
    int tempInt;
    int status;
    int crcTab[256];
    int updateCrc(unsigned char *c, int numBytes);
    void initCrcTab();
    SOCKET sock;
    FilaRecebeDados *rd;
    RecebeDados *envelope;
    pthread_t *thread1;
    pthread_t *thread2;

    HANDLE semaforo;
    HANDLE semaforoFotos;
    HANDLE semSlots;

    int numeroFotos;
    int numeroFotosIO;
    int qualidadeFotoIO;
    int formatoFotoIO;
    int tempoLaco;
    char texto[64];
    char arqLog[2048];
    unsigned char macItscam[6];
    unsigned int timeoutIO;
    unsigned char auxPacote;
    unsigned char auxCrc;
    RecebeDados *config;
    RecebeDados *slots[128][16];
    unsigned char *imagem;

    int setaConfig(unsigned char, int, int, int);
    void processaDados(unsigned char *, int);
    int reqConfig(unsigned char config,unsigned char *parametros, int tparametros, int offset,int len);
    int requisita(unsigned char **buf, int formato, int qualidade, bool video, int nfotos, int *tamanhos);
    int requisitaIO(unsigned char **buf, int formato, int qualidade, int nfotos, int *tamanhos);
    void enviaDados(SOCKET s, char *data, int size);
    void log(const char *texto);

public:

    Itscam300(const char *ip, int timeout, int port=50000);
    ~Itscam300();

    void itscamThread();
    void keepAlive();

    static int criarItscam300(const char *ip, int timeout);
    static Itscam300 *getObject(int i)
    {
        return ids[i];
    }

    int ocrJPEG(unsigned char *jpeg, char *placa);

    int leRealceBorda();
    int leVersaoFirmware();
    int leHdr();
    int leTipoShutter();
    int leShutterFixo();
    int leShutterMaximo();
    int leTipoGanho();
    int leGanhoFixo();
    int leGanhoMaximo();
    int leModoTeste();
    int leNivelDesejado();
    int leValorNivel();
    int leValorGanho();
    int leValorShutter();
    int leSituacaoDayNight();
    int leFormatoFotoIO();
    int leQualidadeFotoIO();
    int lePesos(int *pesos);
    int leValorEntrada();
    int leRotacao();
    int leAutoIris();
    int leModoDayNight();
    int leModoOCR();
    int leRevisaoFirmware();
    int leTipoFlash();
    int leTrigger();
    int leTipoSaida();
    int leValorSaida();
    int leDelay();
    int leStatus();
    int leIp(char *ip);
    int leMascaraRede(char *mascara);
    int leGateway(char *gateway);
    int leMac(char *mac);
    int lePorcentagemSegundoDisparo();
    int leNumeroFotos();
    int leNumeroFotosIO();
    int leSaturacao();
    int leBrilho();
    int leContraste();
    int leGamma();
    int leSombra();
    int leFocoInfraVermelho();
    int leModelo();
    int leBalancoBranco(int *bb);
    int leFotoColorida();
    int leGanhoAlternativo();
    int leValorGanhoAlternativo();
    int leGammaAlternativo();
    int leValorGammaAlternativo();
    int leBalancoBrancoAlternativo();
    int leValorBalancoBrancoAlternativo(int *bb);
    int leBalancoBrancoAtual(int *bb);
    int leDelayCapturaDay();
    int leDelayCapturaNight();
    int lePosicaoZoom();
    int lePosicaoFoco();
    int leAutoFoco();
    int leDataAtual();
    int leHoraAtual();
    int leGanhoSegundaFotoLuzVisivel();
    int leGanhoSegundaFotoLuzInfravermelha();
    int leTempoEntreTriggers();
    int setaNivelDesejado(int nivel);
    int setaRotacao(int rotacao180);
    int setaValorSaida(int saida);
    int setaHdr(int hdr);
    int setaDelay(int delay);
    int setaShutterFixo(int shutter);
    int setaShutterMaximo(int shutter);
    int setaGanhoFixo(int ganho);
    int setaGanhoMaximo(int ganho);
    int setaQualidadeFotoIO(int qualidade);
    int setaFlash(int flash);
    int setaTrigger(int trigger);
    int setaTipoSaida(int saida);
    int setaIp(char *ip);
    int setaMascaraRede(char *mascara);
    int setaGateway(char *gateway);
    int setaArquivoLog(const char *arquivo);
    int setaTipoShutter(int tipo);
    int setaTipoGanho(int automatico);
    int setaModoTeste(int teste);
    int setaFormatoFotoIO(int formato);
    int setaLenteAutoIris(int autoiris);
    int setaModoOCR(int ocr);
    int setaModoDayNight(int daynight);
    int setaPesos(int *pesos);
    int setaPadrao();
    int reinicializaItscam();
    int setaNumeroFotos(int num);
    int setaPorcentagemSegundoDisparo(int porcentagem);
    int setaNumeroFotosIO(int num);
    int setaSaturacao(int saturacao);
    int setaBrilho(int brilho);
    int setaContraste(int contraste);
    int setaGamma(int gamma);
    int setaBalancoBranco(int *bb);
    int setaRealceBorda(int borda);
    int setaFotoColorida(int cor);
    int setaGanhoAlternativo(int tipo);
    int setaValorGanhoAlternativo(int ganho);
    int setaGammaAlternativo(int tipo);
    int setaValorGammaAlternativo(int gamma);
    int setaBalancoBrancoAlternativo(int tipo);
    int setaValorBalancoBrancoAlternativo(int *bb);
    int setaGanhoSegundaFotoLuzVisivel(int ganho);
    int setaGanhoSegundaFotoLuzInfravermelha(int ganho);
    int setaTempoEntreTriggers(int tempo);
    int setaFoco(int foco);
    int setaAutoFoco(int foco);
    int setaSombra(int sombra);
    int setaZoom(int zoom);
    int setaDataAtual(int data);
    int setaHoraAtual(int hora);
    int setaTimeoutIO(int sec);
    int setaDelayCapturaDay(int delay);
    int setaDelayCapturaNight(int delay);
    int setaPosicaoZoom(int zoom);
    int setaPosicaoFoco(int foco);
    int setaFocoDayNight(int daynight);
    int setaMac(unsigned char *mac);
    int requisitaFoto(unsigned char *buf, int formato, int qualidade);
    int requisitaQuadroVideo(unsigned char *buf, int formato, int qualidade);
    int requisitaMultiplasFotos(unsigned char **buf, int nfotos, int *tamanho, int formato, int qualidade);
    int requisitaMultiplasFotosSemEspera(int nfotos, int formato, int qualidade);
    int requisitaFotoId(int slot, int idFoto, unsigned char *buf, int *tempo);
    int requisitaMultiplasFotosIOSemEspera(int nfotos, int formato, int qualidade);
    int salvarFoto(char *arquivo,int formato,int qualidade);
    int salvarMultiplasFotos(char **filename,int nFotos,int *tamFotos,int formato,int qualidade);
    int requisitaFotoIO(unsigned char *foto, int formato, int qualidade);
    int requisitaFotoTriggerContinuo(unsigned char *buf, int formato, int qualidade);
    int requisitaMultiplasFotosIO(unsigned char **foto,int nFotos,int *tamFotos,int formato,int qualidade);
    int salvarFotoIO(char *arquivo,int formato,int qualidade);
    int salvarMultiplasFotosIO(char **filename,int nFotos,int *tamFotos,int formato,int qualidade);
    int salvarFotoOcr(char *diretorio, int qualidade);
    int salvarFotoOcrTriggerContinuo(char *diretorio, int qualidade);
    int salvarFotoOcrIO(char *diretorio, int qualidade);
    int salvarMultiplasFotosOcr(char *diretorio, int nFotos, int qualidade);
    int salvarMultiplasFotosOcrIO(char *diretorio, int nFotos, int qualidade);
    int salvarFotoTriggerContinuo(char *arquivo,int formato,int qualidade);
    int atualizarFirmware(unsigned char *firmware, int tamanho);
    int atualizarFirmwareSenha(unsigned char *firmware, int tamanho, unsigned short senha);
};

#endif /* ITSCAM300_H */
