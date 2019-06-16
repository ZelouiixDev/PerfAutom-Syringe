/*
	Fichier : seringue.c
	Auteur : Julien LAMMENS
	Version : 1.0
	Description : Cerveau d'une seringue auto pulsée. Ce programme reçoit des requêtes via une socket d'écoute sur le port 50000.
		Les requêtes sont au format suivant : "id_requete:id_seringue:dose".
		Le programme va ensuite envoyer des signaux aux GPIO de la Raspberry Pi pour augmenter ou baisser la dose, puis actualiser l'écran LCD affichant la dose et la quantité restante de produit.
		Ce programme gère également la quantité de produit restante dans la seringue via un thread timer qui calcule la quantité restante en fonction de la dose configurée (prorata). 
		Lorsque la seringue n'a plus de produit, cela allume la LED bleue du kit GrovePi+ en exécutant un fichier Python (grovePiBridge.py) avec les paramètres "led <numéro_port> <état (1 ou 0)>".
		L'écran LCD est actualisé via ce même fichier Python, avec les paramètres "screen <dose> <quantite_restante>".
	Todo: Envoyer une notification au serveur lorsque la seringue est vide.
*/

#include "seringue.h" // Toute les constantes principales sont définies dans ce fichier

sem_t reload_lcd;

int socketEcoute, socketClient, status, est_vide = 0;
struct sockaddr_in svc, clt;
char dose[6] = {'0', '0', '.', '0', '0', '\0'};
float quantite_seringue = QUANTITE_SERINGUE;

//Requete reçue "id_requete:id_seringue:dose"

// Fonction permettant de contrôler l'état des LED (on / off)
void led_control(int pin, int etat){
	pinMode(pin, OUTPUT);
	digitalWrite(pin, etat);
}

// Fonction permettant d'appeler le script python permettant de contrôler l'écran LCD, en changeant le texte avec la nouvelle dose passée en paramètre
void lcd_control(char *nouvelle_dose){
	char pythonCommande[256];
	char quantite[3];
	gcvt(quantite_seringue, 2, quantite);
	sem_wait(&reload_lcd);
	snprintf(pythonCommande, sizeof(pythonCommande), "python grovePiBridge.py screen %s %s", nouvelle_dose, quantite);
	system(pythonCommande);
	sem_post(&reload_lcd);
}

// Fonction qui copie la dose actuelle dans la nouvelle dose
void maj_nouvelle_dose(char *nouvelle_dose){
	nouvelle_dose[0] = dose_actuelle[0] + '0';
	nouvelle_dose[1] = dose_actuelle[1] + '0';
	nouvelle_dose[2] = '.';
	nouvelle_dose[3] = dose_actuelle[2] + '0';
	nouvelle_dose[4] = dose_actuelle[3] + '0';
	nouvelle_dose[5] = '\0';
	strcpy(dose, nouvelle_dose);
}

// Pour chaque digit, cette fonction additionne la dose actuelle avec la dose passée en paramètre, puis allume les LED de simulation
int augmenter_dose(char *dose){
	printf("Augmentation de la dose de %s...\n", dose);
	int dizaine = dose[0] - '0';
	int unite = dose[1] - '0';
	int dizieme = dose[3] - '0';
	int centieme = dose[4] - '0';
	int i = 0;
	char nouvelle_dose[6] = {dose_actuelle[0], dose_actuelle[1], '.', dose_actuelle[2], dose_actuelle[3]};
	
	// Pour chaque chiffre de la dose, on actualise la variable dose_actuelle (addition), on fait clignoter les LED, et on affiche la nouvelle dose sur l'écran LCD
	printf("Augmentation centième:\n");
	for (i=0; i<centieme; i++){
		if (dose_actuelle[3] < 9) dose_actuelle[3]++;
		else {
			dose_actuelle[3] = 0;
			if (dose_actuelle[2] < 9) dose_actuelle[2]++;
			else {
				dose_actuelle[2] = 0;
				if (dose_actuelle[1] < 9) dose_actuelle[1]++;
				else {
					if (dose_actuelle[0] < 9) {
						dose_actuelle[0]++;
						dose_actuelle[1] = 0;
					}
				}
			}
		}
		maj_nouvelle_dose(nouvelle_dose);
		printf("\tNouvelle dose = %s\n", nouvelle_dose);
		led_control(LED_CENTIEME_UP, 1);
		lcd_control(nouvelle_dose);
		delay(250);
		led_control(LED_CENTIEME_UP, 0);
		delay(250);
	}
	printf("Augmentation dixième:\n");
	for (i=0; i<dizieme; i++){
		if (dose_actuelle[2] < 9) dose_actuelle[2]++;
		else {
			dose_actuelle[2] = 0;
			if (dose_actuelle[1] < 9) dose_actuelle[1]++;
			else {
				if (dose_actuelle[0] < 9){
					dose_actuelle[0]++;
					dose_actuelle[1] = 0;
				}
			}
		}
		maj_nouvelle_dose(nouvelle_dose);
		printf("\tNouvelle dose = %s\n", nouvelle_dose);
		led_control(LED_DIXIEME_UP, 1);
		lcd_control(nouvelle_dose);
		delay(250);
		led_control(LED_DIXIEME_UP, 0);
		delay(250);
	}
	printf("Augmentation unité:\n");
	for (i=0; i<unite; i++){
		if (dose_actuelle[1] < 9) dose_actuelle[1]++;
		else {
			if (dose_actuelle[0] < 9) {
				dose_actuelle[0]++;
				dose_actuelle[1] = 0;
			}
		}
		maj_nouvelle_dose(nouvelle_dose);
		printf("\tNouvelle dose = %s\n", nouvelle_dose);
		led_control(LED_UNITE_UP, 1);
		lcd_control(nouvelle_dose);
		delay(250);
		led_control(LED_UNITE_UP, 0);
		delay(250);
	}
	printf("Augmentation dizaine:\n");
	for (i=0; i<dizaine; i++){
		if (dose_actuelle[0] < 9) dose_actuelle[0]++;
		maj_nouvelle_dose(nouvelle_dose);
		printf("\tNouvelle dose = %s\n", nouvelle_dose);
		led_control(LED_DIZAINE_UP, 1);
		lcd_control(nouvelle_dose);
		delay(250);
		led_control(LED_DIZAINE_UP, 0);
		delay(250);
	}
	
	return 1;
}

// Pour chaque digit, cette fonction soustrait la dose actuelle avec la dose passée en paramètre, puis allume les LED de simulation
int diminuer_dose(char *dose){
	printf("Diminution de la dose de %s...\n", dose);
	int dizaine = dose[0] - '0';
	int unite = dose[1] - '0';
	int dizieme = dose[3] - '0';
	int centieme = dose[4] - '0';
	int i = 0;
	char nouvelle_dose[6] = {dose_actuelle[0], dose_actuelle[1], '.', dose_actuelle[2], dose_actuelle[3]};
	
	// Pour chaque chiffre de la dose, on actualise la variable dose_actuelle (soustraction), on fait clignoter les LED, et on affiche la nouvelle dose sur l'écran LCD
	printf("Diminution centième:\n");
	for (i=0; i<centieme; i++){
		if (dose_actuelle[3] > 0) dose_actuelle[3]--;
		else {
			dose_actuelle[3] = 9;
			if (dose_actuelle[2] > 0) dose_actuelle[2]--;
			else {
				dose_actuelle[2] = 9;
				if (dose_actuelle[1] > 0) dose_actuelle[1]--;
				else {
					if (dose_actuelle[0] > 0) {
						dose_actuelle[0]--;
						dose_actuelle[1] = 9;
					}
				}
			}
		}
		maj_nouvelle_dose(nouvelle_dose);
		printf("\tNouvelle dose = %s\n", nouvelle_dose);
		led_control(LED_CENTIEME_DOWN, 1);
		lcd_control(nouvelle_dose);
		delay(250);
		led_control(LED_CENTIEME_DOWN, 0);
		delay(250);
	}
	printf("Diminution dixième:\n");
	for (i=0; i<dizieme; i++){
		if (dose_actuelle[2] > 0) dose_actuelle[2]--;
		else {
			dose_actuelle[2] = 9;
			if (dose_actuelle[1] > 0) dose_actuelle[1]--;
			else {
				if (dose_actuelle[0] > 0){
					dose_actuelle[0]--;
					dose_actuelle[1] = 9;
				}
			}
		}
		maj_nouvelle_dose(nouvelle_dose);
		printf("\tNouvelle dose = %s\n", nouvelle_dose);
		led_control(LED_DIXIEME_DOWN, 1);
		lcd_control(nouvelle_dose);
		delay(250);
		led_control(LED_DIXIEME_DOWN, 0);
		delay(250);
	}
	printf("Diminution unité:\n");
	for (i=0; i<unite; i++){
		if (dose_actuelle[1] > 0) dose_actuelle[1]--;
		else {
			if (dose_actuelle[0] > 0) {
				dose_actuelle[0]--;
				dose_actuelle[1] = 9;
			}
		}
		maj_nouvelle_dose(nouvelle_dose);
		printf("\tNouvelle dose = %s\n", nouvelle_dose);
		led_control(LED_UNITE_DOWN, 1);
		lcd_control(nouvelle_dose);
		delay(250);
		led_control(LED_UNITE_DOWN, 0);
		delay(250);
	}
	printf("Diminution dizaine:\n");
	for (i=0; i<dizaine; i++){
		if (dose_actuelle[0] > 0) dose_actuelle[0]--;
		maj_nouvelle_dose(nouvelle_dose);
		led_control(LED_DIZAINE_DOWN, 1);
		lcd_control(nouvelle_dose);
		delay(250);
		led_control(LED_DIZAINE_DOWN, 0);
		delay(250);
	}
	return 1;
}

// Fonction d'arrêt d'urgence de la seringue
int arreter_seringue(){
	int i = 0;
	char nouvelle_dose[6];
	printf("Arrêt de la seringue...\n");
	for (i = 0; i < 4 ; i++) dose_actuelle[i] = 0;
	maj_nouvelle_dose(nouvelle_dose);
	lcd_control(nouvelle_dose);
	printf("dose: %s\n", nouvelle_dose);
	return 1;
}

// Fonction qui envoie un acquitement via la socket
void envoyer_acquitement(requete_t reponse, int socketClient){
	char str_reponse[MAX_CARACTERES];
	sprintf(str_reponse, "%d:%d:%s", reponse.id_requete, reponse.id_seringue, "0");
	CHECK(write(socketClient, str_reponse, strlen(str_reponse)+1), "Erreur d'envoi de l'acquitement");
}

// Fonction de dialogue avec la socket d'un client
void *dialogue_client(void *args){
	requete_t requete, reponse;
	int nbCaracteres = 0;
	char buffer[MAX_CARACTERES];

	// Réception de la requête
	CHECK(nbCaracteres = read(socketClient, buffer, sizeof(buffer)), "Erreur de lecture de la requête");
	sscanf(buffer, "%d:%d:%s",&requete.id_requete, &requete.id_seringue, requete.dose);
	printf("Commande reçue :\n\tid_requete: %d,\n\tid_seringue: %d,\n\tdose: %s\n", requete.id_requete, requete.id_seringue, requete.dose);
	
	// Teste si la seringue est bien la notre et traite la commande
	if (requete.id_seringue == ID_SERINGUE){
		reponse.id_seringue = ID_SERINGUE;
		switch(requete.id_requete){
			// Augmentation de dose
			case 100:
				reponse.id_requete = 101;
				envoyer_acquitement(reponse, socketClient);
				augmenter_dose(requete.dose);
			break;
			
			// Diminution de dose
			case 200:
				envoyer_acquitement(reponse, socketClient);
			 	reponse.id_requete = 201;
				diminuer_dose(requete.dose);
			break;
			
			// Demande d'arrêt
			case 300:
				if (arreter_seringue() != 0) reponse.id_requete = 301;
				else reponse.id_requete = 1;
				envoyer_acquitement(reponse, socketClient);
			break;
			
			default:
				printf("Commande non reconnue...\n");
				reponse.id_requete = 1;
				envoyer_acquitement(reponse, socketClient);
			break;
		}
	}
}

// Thread permettant de traiter l'évènement de l'appuie sur le bouton RESET
// Remet la quantité de produit dans la seringue à 60 et éteint la LED
void *reset_event(void *args){
	while (1){
		if (digitalRead(BOUTON) == LOW){
			printf("Réinitialisation demandée !\n");
			quantite_seringue = QUANTITE_SERINGUE;
			system("python grovePiBridge.py led 8 0");
			est_vide = 0;
			sleep(3);
		}
	}
}

// Thread qui calcule la quantité restante de produit dans la seringue, en fonction de la dose configurée (ml/min pour la simulation)
void *timer_seringue(void *args){
	char pythonCommande[256];
	char quantite[3];
	while (1){
		if (!est_vide){
			quantite_seringue -= atof(dose)/60;
			gcvt(quantite_seringue, 2, quantite);
			printf("Quantité restante de produit : %s\n", quantite);
			sem_wait(&reload_lcd);
			snprintf(pythonCommande, sizeof(pythonCommande), "python grovePiBridge.py screen %s %s", dose, quantite);
			system(pythonCommande);
			sem_post(&reload_lcd);
		}
		if (quantite_seringue <= 0 && !est_vide){
			quantite_seringue = 0;
			system("python grovePiBridge.py led 8 1");
			sem_wait(&reload_lcd);
			snprintf(pythonCommande, sizeof(pythonCommande), "python grovePiBridge.py screen %s 0", dose);
			system(pythonCommande);
			sem_post(&reload_lcd);
			est_vide = 1;
		}
		sleep(1);
	}
}

int main(){
	pthread_t dialogue, timer, reset;
	socklen_t cltLen;
	sem_init(&reload_lcd, 1, 1);
	
	if (wiringPiSetup() == -1) return 0;
	pinMode(BOUTON, INPUT);
	system("python grovePiBridge.py screen 00.00, 60");
	system("python grovePiBridge.py led 8 0");
	
	// Création de la socket de réception d’écoute des appels 
	CHECK(socketEcoute=socket(PF_INET, SOCK_STREAM, 0), "Erreur de création de la socket d'écoute");

	printf("Socket d'écoute créée !\n");
	
	// Préparation de l’adressage du service
	svc.sin_family = PF_INET;
	svc.sin_port = htons (PORT_SVC);
	svc.sin_addr.s_addr = INADDR_ANY;
	memset(&svc.sin_zero, 0, 8);
	
	// Association de l’adressage préparé avec la socket d’écoute
	CHECK(bind(socketEcoute, (struct sockaddr *) &svc, sizeof(svc)) , "Erreur du bind de la socket d'écoute");
	
	// Mise en écoute de la socket
	CHECK(listen(socketEcoute, 5) , "Erreur du listen de la socket d'écoute");
	printf("Ecoute en cours sur le port %d !\n", htons(svc.sin_port));
	
	CHECK(pthread_create(&timer, NULL, timer_seringue, 0), "problème création thread de timer");
	CHECK(pthread_create(&reset, NULL, reset_event, 0), "problème création thread de reset");
	cltLen = sizeof(clt);
	
	// Boucle permanente de service
	while (1) {
		//  Attente d’un appel
		printf("Attente d'un appel...\n");
		socketClient = accept(socketEcoute, (struct sockaddr *)&clt, &cltLen); 
		
		fprintf(stderr, "Appel reçu du client [%s], sur le port [%d]\n", inet_ntoa(clt.sin_addr), ntohs(clt.sin_port));

		// Création d'un thread pour l'échange
		CHECK(pthread_create(&dialogue, NULL, dialogue_client, 0), "problème création thread de dialogue");
	}

	return 0;
}
