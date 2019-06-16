#define CHECK(sts, msg) if ((sts) == -1) {perror(msg);exit(-1);}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#include <wiringPi.h>

#define ID_SERINGUE 1		// ID de la seringue
#define PORT_SVC 50000		// Port d'écoute de la socket d'écoute
#define MAX_CARACTERES 128	// Maximum de caractères dans une requête
#define QUANTITE_SERINGUE 50	// Quantité initiale contenue dans la seringue

// Configuration des numéros de GPIO pour chaque LED et pour le bouton
#define LED_CENTIEME_UP 6
#define LED_CENTIEME_DOWN 5
#define LED_DIXIEME_UP 1
#define LED_DIXIEME_DOWN 4
#define LED_UNITE_UP 0
#define LED_UNITE_DOWN 3
#define LED_DIZAINE_UP 7
#define LED_DIZAINE_DOWN 2
#define BOUTON 21

/*
	ID_REQUETE			SIGNIFICATION
	1					Problème de fonctionnement
	100					Augmentation de dose
	101					Acquitement de l'augmentation de la dose
	200					Diminition de dose
	201					Acquitement de la diminution de la dose
	300					Arrêt de la seringue
	301					Acquitement de l'arrêt de la seringue
	400					Seringue terminée
*/

// Structure d'une requête
struct requete{
	int id_seringue;
	int id_requete;
	char dose[6];
};

typedef struct requete requete_t;

// Initialisation de la dose à 0
int dose_actuelle[4] = {0,0,0,0};
