/*
 ============================================================================
 Name        : main.c
 Author      : dialoghmari
 Version     : 1.0
 Description : Simulation de systeme de transport public
 ============================================================================
  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define pmax 50 //Nombre maximal des passager dans le fichier
#define busMax 5 //Capacité du bus
#define MetroMax 8 //Capacité du métro
#define NbrStations 9 // nombre total des stations
#define nomTube "myfifo" // Nom de fifo


typedef struct passager
{
	short id;
	short depart;
	short arrivee;
	short attente;
	short transfert;
	short attMax;
} passager;
int temps = 0;
passager ListeBus[busMax];
passager ListeMetro[MetroMax];
int NbPBus = 0; // nombre des passagers dans le bus
int NbPMetro = 0; //nombre des passagers dans le metro;
passager LStation[NbrStations][pmax]; // liste des passagers dans les stations
int nbs[NbrStations]; // Nombre de passagers dans les stations


/* Trier le tableau des passagers par date arrivée */
void trier (passager tab[], int n)
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			if (tab[j].attente < tab[i].attente)
			{
				passager tmp = tab[i];
				tab[i] = tab[j];
				tab[j] = tmp;
			}
		}
	}
}

void afficherPassager(passager p)
{
	printf("Passager n%d [%d -> %d] attente : %d transfert: %d max: %d\n", p.id, p.depart, p.arrivee, p.attente, p.transfert, p.attMax);
}

/* Verifier le bon format d'une ligne de fichier */
int verif(char* ligne)
{
	int charcount;
	charcount = 0;
	for (int m = 0; ligne[m]; m++) {
		if (ligne[m] == ' ') {
			charcount ++;
		}
	}
	if (charcount != 6 || ligne[0] != '#' || (strcmp(ligne, "") == 0))
	{
		return 0;
	}
	return 1;
}

/* Construire un passager à partir d'une ligne de fichier */
passager extractPassager(char * ligne)
{
	passager p;
	if (verif(ligne))
	{
		char str1[100];
		char newString[10][10];
		int i, j, ctr;

		strcpy(str1, ligne);

		j = 0; ctr = 0;
		for (i = 2; i <= (strlen(str1)); i++)
		{
			// if space or NULL found, assign NULL into newString[ctr]
			if (str1[i] == ' ' || str1[i] == '\0')
			{
				newString[ctr][j] = '\0';
				ctr++;  //for next word
				j = 0;  //for next word, init index to 0
			}
			else
			{
				newString[ctr][j] = str1[i];
				j++;
			}
		}

		p.id = atoi(newString[0]);
		p.depart = atoi(newString[1]);
		p.arrivee = atoi(newString[2]);
		p.attente = atoi(newString[3]);
		p.transfert = atoi(newString[4]);
		p.attMax = atoi(newString[5]);
	} else
	{
		/* perror("Ligne erronée !");
		exit(EXIT_FAILURE); */
	}

	return p;
}

/* Distribuer la liste des passagers vers les stations adéquats */
void distribuerPassagers(passager tab[], int n)
{
	for (int i = 0; i < n; ++i)
	{
		LStation[tab[i].depart][nbs[tab[i].depart]] = tab[i];
		nbs[tab[i].depart]++;
	}
}

void afficherStations()
{
	printf("NbPBus = %d \n", NbPBus);
	printf("NbPMetro = %d \n", NbPMetro);
	for (int i = 0; i < NbrStations; ++i)
	{
		printf("Station no: %d \n", i);
		for (int j = 0; j < nbs[i]; ++j)
		{
			afficherPassager(LStation[i][j]);
		}
		printf("_________ \n");
	}
}

/* Constuire un tableau de passagers trié par par temps d'arrivée */
void extractLignes(FILE * fichier)
{
	for (int i = 0; i < NbrStations; ++i)
	{
		nbs[i] = 0;
	}
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	//Tableau des passagers dans le fichier
	passager passagers[pmax];
	//nombre des passagers dans le fichier
	int nbp = 0;

	//fichier = fopen("file.txt", "r");
	if (fichier == NULL)
		exit(EXIT_FAILURE);

	while ((read = getline(&line, &len, fichier)) != -1) {
		//printf("Retrieved line of length %d :\n", len);
		strtok(line, "\n"); //enlever le retour à la ligne.
		if (verif(line))
		{
			passagers[nbp] = extractPassager(line);
			nbp ++;
		}
	}

	printf("nombre des passagers = %d \n \n \n", nbp);
	distribuerPassagers(passagers, nbp);
	afficherStations();
}

/* supprimer un element d'un tableau */
void supprimer(passager arr[], int pos, int* size)
{
	if (pos == (*size) + 1 || pos < 0)
	{
		/*
		exit(EXIT_FAILURE);
		perror("iciiiiiiiiiiiiiiiiiii");
		 */
	}
	else
	{
		for (int j = pos; j < (*size); j++)
		{
			arr[j] = arr[j + 1];
		}
		(*size)--;
	}
}

/* Embarquer des passagers dans le bus */
void embarquerBus(int numeroStation)
{
	int i = 0;
	while (i < nbs[numeroStation] && NbPBus < busMax )
	{
		if (LStation[numeroStation][i].depart == numeroStation)
		{
			printf("[bus] : embarque le passager {%d} \n", LStation[numeroStation][i].id);
			ListeBus[NbPBus] = LStation[numeroStation][i];
			NbPBus++;
			//Retirer le passager de sa station de depart
			supprimer(LStation[numeroStation], i, &nbs[numeroStation]);
		}
		i++;
	}
}

/* Debarquer des passagers du bus */
void debarquerBus(int numeroStation)
{
	if (NbPBus > 0)
	{
		int i = 0;
		if (numeroStation == 0)
		{
			i = 0;
			while (i < NbPBus)
			{
				if (ListeBus[i].arrivee > 4)
				{
					printf("[bus] : debarque le passager {%d} \n", ListeBus[i].id);
					ListeBus[i].depart = 5;
					LStation[5][nbs[5]] = ListeBus[i];
					nbs[5]++;
					supprimer(ListeBus, i, &NbPBus);
					i--;
				}
				i++;

			};
		}

		i = 0;
		while (i < NbPBus)
		{
			if (ListeBus[i].arrivee == numeroStation)
			{
				printf("[bus] : debarque le passager {%d} (arrivé)\n", ListeBus[i].id);
				supprimer(ListeBus, i, &NbPBus);
				i--;
			}
			i++;

		};

	}
}

/* Embarquer un passager */
void embarquerMetro(int numeroStation)
{
	int i = 0;
	while (i <= nbs[numeroStation] && NbPMetro < MetroMax)
	{
		if (LStation[numeroStation][i].depart == numeroStation)
		{
			printf("[metro] : embarque le passager {%d} \n", LStation[numeroStation][i].id);
			ListeMetro[NbPMetro] = LStation[numeroStation][i];
			NbPMetro++;
			//Retirer le passager de sa station de depart
			supprimer(LStation[numeroStation], i, &nbs[numeroStation]);
		}
		i++;
	}
}

/* Debarquer un passager */
void debarquerMetro(int numeroStation)
{
	if (NbPMetro > 0)
	{
		int i = 0;
		if (numeroStation == 5)
		{
			i = 0;
			while (i < NbPMetro)
			{
				if (ListeMetro[i].arrivee < 5)
				{
					printf("[metro] : debarque le passager {%d} \n", ListeMetro[i].id);
					ListeMetro[i].depart = 0;
					LStation[0][nbs[0]] = ListeMetro[i];
					nbs[0]++;
					supprimer(ListeMetro, i, &NbPMetro);
					i--;
				}
				i++;
			};
		}

		i = 0;
		while (i < NbPMetro)
		{
			if (ListeMetro[i].arrivee == numeroStation)
			{
				printf("[metro] : debarque le passager {%d} (arrivé) \n", ListeMetro[i].id);
				supprimer(ListeMetro, i, &NbPMetro);
				i--;
			}
			i++;

		};

	}
}

pthread_t pthread_id[4];
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock3 = PTHREAD_MUTEX_INITIALIZER;

void * roulerMetro() {
	int station = 5;
	short countup = 1;
	while (1)
	{
		pthread_mutex_lock(&lock1);

		printf("Metro : à la station %d \n", station);
		debarquerMetro(station);
		embarquerMetro(station);

		/* prochaine station */
		if (countup)
		{
			station++;
			if (station >= 7)
				countup = 0;
		}
		else
		{
			--station;
			if (station <= 5)
				countup = 1;
		}
		sleep(1);
	}
}

void * roulerBus() {
	int station = 0;
	while (1)
	{
		pthread_mutex_lock(&lock2);
		printf("Bus   : à la station %d \n", station);
		debarquerBus(station);
		embarquerBus(station);
		/* prochaine station */
		station++;
		if (station > 5)
		{
			station = 0;
		}
		pthread_mutex_unlock(&lock1);
		sleep(1);
	}
}

void * verificateur() {
	while (1)
	{
		//printf("Verificateur !!!\n");
		temps++;
		if (temps % 10 == 0)
		{
			afficherStations();
		}

		for (int i = 0; i < NbrStations; ++i)
		{
			for (int j = 0; j < nbs[i]; ++j)
			{
				LStation[i][j].attente++;
				if (LStation[i][j].attente > LStation[i][j].attMax)
				{
					//printf("Ce passager va prendre un taxi : \n");
					//afficherPassager(LStation[i][j]);

					int fd;
					char message[250];
					sprintf(message, "# %d %d %d %d %d %d", LStation[i][j].id, LStation[i][j].depart, LStation[i][j].arrivee, LStation[i][j].attente, LStation[i][j].transfert, LStation[i][j].attMax);
					if ((fd = open(nomTube, O_WRONLY)) == -1)
					{
						fprintf(stderr, "Impossible d'ouvrir l'entrée du tube nommé.\n");
						exit(EXIT_FAILURE);
					}
					else {
						//pthread_mutex_unlock(&lock3);
						write(fd, message, strlen(message));
					}
					close (fd) ;

					supprimer(LStation[i], i, &nbs[i]);
				}
			}
		}
		pthread_mutex_unlock(&lock2);
		sleep(2);
	}
}

void * roulerTaxi1() {
	while (1)
	{
		pthread_mutex_lock(&lock3);
		int sortieTube;

		char chaineALire[250];

		if ((sortieTube = open (nomTube, O_RDONLY)) == -1)
		{
			fprintf(stderr, "Impossible d'ouvrir la sortie du tube nommé.\n");
			exit(EXIT_FAILURE);
		}

		read(sortieTube, chaineALire, 250);
		passager p = extractPassager(chaineALire);
		printf("taxi#{1} : passager {%d} est rendu a la station {%d}\n", p.id, p.arrivee);

		pthread_mutex_unlock(&lock3);
		sleep(5);
	}
}


int main(int argc, char const *argv[])
{
	system("[ -e myfifo ] && rm  myfifo");
	FILE * fichier;
	if ( argc == 2 ) {
		fichier = fopen(argv[1], "r");
	} else
	{
		fichier = fopen("passagers.txt", "r");
	}
	extractLignes(fichier);
	fclose (fichier);


	/* Créer la tube nomee */
	if (mkfifo(nomTube, 0666 | O_NONBLOCK) == -1)
	{
		fprintf(stderr, "Erreur de création du tube");
		exit(EXIT_FAILURE);
	}

	if (pthread_create((pthread_id + 1), NULL, roulerMetro, NULL) == -1) {
		fprintf(stderr, "Erreur creation pthread de Metro\n");
		exit(2);
	}
	if (pthread_create((pthread_id + 0), NULL, roulerBus, NULL) == -1) {
		fprintf(stderr, "Erreur creation pthread du Bus\n" );
		exit(2);
	}

	if (pthread_create((pthread_id + 2), NULL, verificateur, NULL) == -1) {
		fprintf(stderr, "Erreur creation pthread du verificateur\n" );
		exit(2);
	}

	if (pthread_create((pthread_id + 3), NULL, roulerTaxi1, NULL) == -1) {
		fprintf(stderr, "Erreur creation pthread du taxi 1\n" );
		exit(2);
	}

	if (pthread_join(pthread_id[0], NULL)) {

		fprintf(stderr, "Erreur join du thread\n");
		return 2;
	}
	if (pthread_join(pthread_id[1], NULL)) {

		fprintf(stderr, "Erreur join du thread\n");
		return 2;
	}
	if (pthread_join(pthread_id[2], NULL)) {

		fprintf(stderr, "Erreur join du thread\n");
		return 2;
	}
	if (pthread_join(pthread_id[3], NULL)) {

		fprintf(stderr, "Erreur join du thread\n");
		return 2;
	}

	return 0;
}